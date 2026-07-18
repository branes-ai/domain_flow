#!/usr/bin/env node
/**
 * make-video.mjs — offline PNG→video for the SURE schedule animations (issue #142, Phase 3).
 *
 * The animations are WebGL and interactive; a landing page (or a README, or a talk) often
 * wants a plain looping clip instead. This renders one embedded viewer frame-by-frame in a
 * headless browser, screenshots each frame, and stitches them with ffmpeg — the same offline
 * path branes-ai/cortex uses. The output is COMMITTED (like docs/schedules/*.json), so the
 * GitHub-Pages build stays toolchain-free; only whoever regenerates a clip needs the tools.
 *
 * LOCAL-ONLY PREREQUISITES (not in package.json, to keep normal installs light):
 *   npm  i -D playwright         # the driver
 *   npx  playwright install chromium
 *   ffmpeg on PATH               # the encoder  (e.g. `apt install ffmpeg` / `brew install ffmpeg`)
 *
 * USAGE (build first so dist/ exists — the script serves it via `astro preview`):
 *   npm run build
 *   node make-video.mjs --page theory/matmul --index 0 --out public/videos/matmul-linear.mp4
 *
 * Then embed the committed clip in a page, e.g.:
 *   <video src="/domain_flow/videos/matmul-linear.mp4" autoplay loop muted playsinline></video>
 *
 * FLAGS (all optional except a target):
 *   --page <path>      page under the site base to open      (default: theory/matmul)
 *   --index <n>        which mounted viewer on that page      (default: 0; see data-sv-index)
 *   --selector <css>   region to screenshot each frame        (default: the viewer's <canvas>)
 *   --out <file>       output path; extension picks the codec (default: public/videos/schedule.mp4)
 *                      .mp4 → H.264, .webm → VP9, .gif → palette GIF
 *   --fps <n>          output frame rate                        (default: 6)
 *   --width/--height   viewport size in px                      (default: 960 × 600)
 *   --loops <n>        repeat the sweep n times in the file     (default: 1)
 *   --settle <ms>      wait after mount before capturing        (default: 800)
 *   --keep-frames      leave the temp PNG dir in place (debug)
 */
import { spawn, execFileSync } from 'node:child_process';
import { mkdtempSync, mkdirSync, rmSync, existsSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname, extname } from 'node:path';

// ── tiny arg parser ─────────────────────────────────────────────────────────
const argv = process.argv.slice(2);
const arg = (name, def) => {
  const i = argv.indexOf(`--${name}`);
  return i >= 0 && i + 1 < argv.length ? argv[i + 1] : def;
};
const flag = (name) => argv.includes(`--${name}`);

const page = arg('page', 'theory/matmul').replace(/^\/|\/$/g, '');
const index = Number(arg('index', '0'));
const out = arg('out', 'public/videos/schedule.mp4');
const fps = Number(arg('fps', '6'));
const width = Number(arg('width', '960'));
const height = Number(arg('height', '600'));
const loops = Math.max(1, Number(arg('loops', '1')));
const settle = Number(arg('settle', '800'));
const selector = arg('selector', `[data-sv-index="${index}"] canvas`);
const keepFrames = flag('keep-frames');
const ext = extname(out).toLowerCase();

const die = (msg) => { console.error(`make-video: ${msg}`); process.exit(1); };

// ── preflight: ffmpeg + playwright must be present locally ───────────────────
try { execFileSync('ffmpeg', ['-version'], { stdio: 'ignore' }); }
catch { die('ffmpeg not found on PATH. Install it (apt install ffmpeg / brew install ffmpeg).'); }

let chromium;
try { ({ chromium } = await import('playwright')); }
catch { die('playwright not installed. Run: npm i -D playwright && npx playwright install chromium'); }

if (!existsSync('dist')) die('dist/ not found — run `npm run build` first (this script serves dist via astro preview).');

const { BASE } = await import('./base.mjs');
const framesDir = mkdtempSync(join(tmpdir(), 'sched-frames-'));
mkdirSync(dirname(out), { recursive: true });

// ── serve the built site (astro preview honours the /domain_flow base) ───────
console.log('make-video: starting astro preview …');
// detached ⇒ its own process group, so cleanup can kill the WHOLE tree (npx → astro → the
// server child); otherwise the orphaned server keeps the port and Node's event loop alive.
const preview = spawn('npx', ['astro', 'preview'], { stdio: ['ignore', 'pipe', 'inherit'], detached: true });
const baseUrl = await new Promise((resolve, reject) => {
  const to = setTimeout(() => reject(new Error('astro preview did not report a URL within 30s')), 30000);
  let buf = '';
  preview.stdout.on('data', (d) => {
    buf += d.toString();
    const m = buf.match(/http:\/\/localhost:\d+\S*/);
    if (m) { clearTimeout(to); resolve(m[0].replace(/\/$/, '')); }
  });
  preview.on('exit', (c) => { clearTimeout(to); reject(new Error(`astro preview exited early (${c})`)); });
});

const cleanup = () => {
  // negative pid → signal the whole process group (astro preview + its children)
  try { if (preview.pid) process.kill(-preview.pid, 'SIGTERM'); } catch { /* already gone */ }
  if (!keepFrames) rmSync(framesDir, { recursive: true, force: true });
};

try {
  // the preview URL already includes the base; ensure exactly one base segment
  const origin = baseUrl.replace(new RegExp(`${BASE}$`), '');
  const url = `${origin}${BASE}/${page}/`;
  console.log(`make-video: launching headless chromium → ${url}`);

  const browser = await chromium.launch({
    args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'],
  });
  const pageObj = await browser.newPage({ viewport: { width, height }, deviceScaleFactor: 2 });
  await pageObj.goto(url, { waitUntil: 'networkidle' });

  // wait for the target viewer to mount, then read its frame count
  await pageObj.waitForFunction(
    (i) => Array.isArray(window.__scheduleViewers) && window.__scheduleViewers[i], index, { timeout: 15000 });
  await pageObj.waitForTimeout(settle);
  const frameCount = await pageObj.evaluate((i) => window.__scheduleViewers[i].frameCount, index);
  if (!frameCount || frameCount < 1) die(`viewer #${index} on /${page} reports frameCount=${frameCount}`);
  // the selector must resolve to the <canvas> we read pixels from
  const isCanvas = await pageObj.evaluate((sel) => {
    const c = document.querySelector(sel);
    return !!(c && typeof c.toDataURL === 'function');
  }, selector);
  if (!isCanvas) die(`selector "${selector}" did not match a <canvas> on /${page}`);

  // Read the canvas pixels directly with canvas.toDataURL instead of a Playwright screenshot:
  // both element- and page-level screenshots deadlock against the page's continuous
  // requestAnimationFrame WebGL loops (they wait for a "stable"/committed frame that never
  // comes). toDataURL is frame-exact and, as a bonus, captures the CANVAS ONLY — no HUD or
  // controls — which is what a clean loop wants. It needs the renderer's drawing buffer to be
  // preserved (WebGLRenderer preserveDrawingBuffer:true, set in schedule-anim.js).
  console.log(`make-video: capturing ${frameCount} frame(s) × ${loops} loop(s) …`);
  let n = 0;
  for (let l = 0; l < loops; l++) {
    for (let f = 0; f < frameCount; f++) {
      // set the frame, render two animation frames so it lands, then grab the pixels
      const dataUrl = await pageObj.evaluate(({ i, frame, sel }) => new Promise((res) => {
        window.__scheduleViewers[i].setFrame(frame);
        requestAnimationFrame(() => requestAnimationFrame(() => {
          const c = document.querySelector(sel);
          res(c ? c.toDataURL('image/png') : null);
        }));
      }), { i: index, frame: f, sel: selector });
      if (!dataUrl || !dataUrl.startsWith('data:image/png')) die(`empty capture at frame ${f} — is preserveDrawingBuffer enabled?`);
      writeFileSync(join(framesDir, `f${String(n++).padStart(5, '0')}.png`), Buffer.from(dataUrl.slice(dataUrl.indexOf(',') + 1), 'base64'));
    }
  }
  await browser.close();

  // ── encode with ffmpeg ─────────────────────────────────────────────────────
  const input = ['-y', '-framerate', String(fps), '-i', join(framesDir, 'f%05d.png')];
  let codec;
  if (ext === '.webm') codec = ['-c:v', 'libvpx-vp9', '-b:v', '0', '-crf', '32', '-pix_fmt', 'yuv420p'];
  else if (ext === '.gif') codec = ['-vf', `fps=${fps},split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse`];
  else codec = ['-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-movflags', '+faststart',
    // H.264 needs even dimensions
    '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2'];
  console.log(`make-video: encoding → ${out}`);
  execFileSync('ffmpeg', [...input, ...codec, out], { stdio: 'inherit' });
  console.log(`make-video: done — ${out} (${frameCount * loops} frames @ ${fps}fps)`);
} finally {
  cleanup();
}
// force exit: even after the group is signalled, a lingering child pipe can keep the event
// loop alive; the work is done and flushed, so don't wait on it.
process.exit(0);
