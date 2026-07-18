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
import { mkdtempSync, mkdirSync, rmSync, existsSync } from 'node:fs';
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
const preview = spawn('npx', ['astro', 'preview'], { stdio: ['ignore', 'pipe', 'inherit'] });
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
  preview.kill('SIGTERM');
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
  const region = pageObj.locator(selector).first();
  const handle = await region.elementHandle();
  if (!handle) die(`selector "${selector}" matched nothing on /${page}`);

  // Grow the viewport if the capture region is taller than it (tall embeds), then scroll the
  // region into view with a raw DOM call. We measure a clip rect and screenshot the VIEWPORT
  // with that clip — element.screenshot() would wait for a "stable bounding box", which never
  // settles on a page full of continuous requestAnimationFrame WebGL loops (it times out).
  let box = await handle.boundingBox();
  if (!box) die('could not measure the capture region — is it visible / has non-zero size?');
  const needH = Math.ceil(box.height) + 80;
  if (needH > height) { await pageObj.setViewportSize({ width, height: needH }); await pageObj.waitForTimeout(200); }
  await handle.evaluate((n) => n.scrollIntoView({ block: 'center', inline: 'center' }));
  await pageObj.waitForTimeout(150);
  box = await handle.boundingBox();
  const vp = pageObj.viewportSize();
  const clip = {
    x: Math.max(0, Math.round(box.x)), y: Math.max(0, Math.round(box.y)),
    width: Math.min(Math.round(box.width), vp.width), height: Math.min(Math.round(box.height), vp.height),
  };

  console.log(`make-video: capturing ${frameCount} frame(s) × ${loops} loop(s) …`);
  let n = 0;
  for (let l = 0; l < loops; l++) {
    for (let f = 0; f < frameCount; f++) {
      // set the frame, then wait two animation frames so the render lands before the shot
      await pageObj.evaluate(({ i, frame }) => new Promise((res) => {
        window.__scheduleViewers[i].setFrame(frame);
        requestAnimationFrame(() => requestAnimationFrame(res));
      }), { i: index, frame: f });
      await pageObj.screenshot({ path: join(framesDir, `f${String(n++).padStart(5, '0')}.png`), clip });
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
