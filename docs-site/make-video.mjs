#!/usr/bin/env node
/**
 * make-video.mjs — offline PNG→video for the SURE schedule animations (issue #142, Phase 3).
 *
 * The animations are WebGL and interactive; a landing page (or a README, or a talk) often
 * wants a plain looping clip instead. This renders one embedded viewer frame-by-frame in a
 * headless browser, reads each frame's pixels, and stitches them with ffmpeg — the same
 * offline path branes-ai/cortex uses. The output is COMMITTED (like docs/schedules/*.json),
 * so the GitHub-Pages build stays toolchain-free; only whoever regenerates a clip needs the
 * tools.
 *
 * LOCAL-ONLY PREREQUISITES. `playwright` is a devDependency, but its browser binaries and
 * ffmpeg are large / out-of-band, so they are NOT pulled by a normal `npm install`:
 *   npx  playwright install chromium   # ~180 MB browser, one-time
 *   ffmpeg on PATH                     # the encoder  (e.g. `apt install ffmpeg` / `brew install ffmpeg`)
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
 *   --selector <css>   region to capture each frame           (default: the viewer's container,
 *                      `[data-sv-index="<n>"]` — every <canvas> inside it is composited, so a
 *                      side-by-side .schedule-compare captures BOTH panes)
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
const loops = Number(arg('loops', '1'));
const settle = Number(arg('settle', '800'));
const selector = arg('selector', `[data-sv-index="${index}"]`);
const keepFrames = flag('keep-frames');
const ext = extname(out).toLowerCase();

// die THROWS (never process.exit) so the finally block always runs its cleanup.
const die = (msg) => { throw new Error(msg); };

// ── validate the numeric flags + output codec up front (bad values would otherwise loop
//    forever on --loops Infinity, round a fractional count up, or silently mismatch codec) ─
const SUPPORTED = new Set(['.mp4', '.webm', '.gif']);
function validate() {
  if (!Number.isSafeInteger(index) || index < 0) die('--index must be a non-negative integer');
  if (!Number.isFinite(fps) || fps <= 0) die('--fps must be a positive number');
  if (!Number.isSafeInteger(width) || width < 1) die('--width must be a positive integer');
  if (!Number.isSafeInteger(height) || height < 1) die('--height must be a positive integer');
  if (!Number.isSafeInteger(loops) || loops < 1) die('--loops must be a positive integer');
  if (!Number.isFinite(settle) || settle < 0) die('--settle must be a non-negative number');
  if (!SUPPORTED.has(ext)) die(`--out must end in ${[...SUPPORTED].join(' / ')}`);
}

let preview, browser, framesDir;
try {
  validate();

  // ── preflight: ffmpeg + playwright + a built dist/ must be present ──────────
  try { execFileSync('ffmpeg', ['-version'], { stdio: 'ignore' }); }
  catch { die('ffmpeg not found on PATH. Install it (apt install ffmpeg / brew install ffmpeg).'); }
  let chromium;
  try { ({ chromium } = await import('playwright')); }
  catch { die('playwright not installed. Run: npm i -D playwright && npx playwright install chromium'); }
  if (!existsSync('dist')) die('dist/ not found — run `npm run build` first (this script serves dist via astro preview).');

  const { BASE } = await import('./base.mjs');
  framesDir = mkdtempSync(join(tmpdir(), 'sched-frames-'));
  mkdirSync(dirname(out), { recursive: true });

  // ── serve the built site (astro preview honours the /domain_flow base) ──────
  console.log('make-video: starting astro preview …');
  // detached ⇒ its own process group, so cleanup can kill the WHOLE tree (npx → astro → the
  // server child); otherwise the orphaned server keeps the port and Node's event loop alive.
  preview = spawn('npx', ['astro', 'preview'], { stdio: ['ignore', 'pipe', 'inherit'], detached: true });
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

  // the preview URL already includes the base; ensure exactly one base segment
  const origin = baseUrl.replace(new RegExp(`${BASE}$`), '');
  const url = `${origin}${BASE}/${page}/`;
  console.log(`make-video: launching headless chromium → ${url}`);
  browser = await chromium.launch({
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

  // the selector must contain at least one <canvas> to read pixels from
  const nCanvas = await pageObj.evaluate((sel) => {
    const root = document.querySelector(sel);
    if (!root) return -1;
    return root.tagName === 'CANVAS' ? 1 : root.querySelectorAll('canvas').length;
  }, selector);
  if (nCanvas < 0) die(`selector "${selector}" matched nothing on /${page}`);
  if (nCanvas === 0) die(`selector "${selector}" contains no <canvas> on /${page}`);

  // Read the canvas pixels directly with canvas.toDataURL instead of a Playwright screenshot:
  // both element- and page-level screenshots deadlock against the page's continuous
  // requestAnimationFrame WebGL loops (they wait for a "stable"/committed frame that never
  // comes). toDataURL is frame-exact and captures the CANVAS ONLY — no HUD or controls. It
  // needs the renderer's drawing buffer preserved (WebGLRenderer preserveDrawingBuffer:true,
  // set in schedule-anim.js). Multiple canvases (a side-by-side compare) are composited.
  console.log(`make-video: capturing ${frameCount} frame(s) × ${loops} loop(s)` +
    (nCanvas > 1 ? `, ${nCanvas} canvases composited` : '') + ' …');
  let n = 0;
  for (let l = 0; l < loops; l++) {
    for (let f = 0; f < frameCount; f++) {
      // set the frame, render two animation frames so it lands, then grab the pixels
      const dataUrl = await pageObj.evaluate(({ i, frame, sel }) => new Promise((res) => {
        window.__scheduleViewers[i].setFrame(frame);
        requestAnimationFrame(() => requestAnimationFrame(() => {
          const root = document.querySelector(sel);
          const canvases = !root ? []
            : (root.tagName === 'CANVAS' ? [root] : [...root.querySelectorAll('canvas')]);
          if (!canvases.length) return res(null);
          if (canvases.length === 1) return res(canvases[0].toDataURL('image/png'));
          // composite the panes side-by-side in DOM order (both compare panes in one frame)
          const gap = 12;
          const w = canvases.reduce((a, c) => a + c.width, 0) + gap * (canvases.length - 1);
          const h = Math.max(...canvases.map((c) => c.height));
          const off = document.createElement('canvas');
          off.width = w; off.height = h;
          const ctx = off.getContext('2d');
          let x = 0;
          for (const c of canvases) { ctx.drawImage(c, x, 0); x += c.width + gap; }
          res(off.toDataURL('image/png'));
        }));
      }), { i: index, frame: f, sel: selector });
      if (!dataUrl || !dataUrl.startsWith('data:image/png')) die(`empty capture at frame ${f} — is preserveDrawingBuffer enabled?`);
      writeFileSync(join(framesDir, `f${String(n++).padStart(5, '0')}.png`), Buffer.from(dataUrl.slice(dataUrl.indexOf(',') + 1), 'base64'));
    }
  }

  // ── encode with ffmpeg ─────────────────────────────────────────────────────
  const input = ['-y', '-framerate', String(fps), '-i', join(framesDir, 'f%05d.png')];
  let codec;
  if (ext === '.webm') codec = ['-c:v', 'libvpx-vp9', '-b:v', '0', '-crf', '32', '-pix_fmt', 'yuv420p'];
  else if (ext === '.gif') codec = ['-vf', `fps=${fps},split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse`];
  // H.264 needs even dimensions
  else codec = ['-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-movflags', '+faststart',
    '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2'];
  console.log(`make-video: encoding → ${out}`);
  execFileSync('ffmpeg', [...input, ...codec, out], { stdio: 'inherit' });
  console.log(`make-video: done — ${out} (${frameCount * loops} frames @ ${fps}fps)`);
} catch (err) {
  console.error(`make-video: ${err.message}`);
  process.exitCode = 1;
} finally {
  // every failure path lands here — close the browser and kill the preview group so nothing
  // is left running, and drop the temp frames unless asked to keep them.
  if (browser) await browser.close().catch(() => {});
  if (preview?.pid) { try { process.kill(-preview.pid, 'SIGTERM'); } catch { /* already gone */ } }
  if (framesDir && !keepFrames) rmSync(framesDir, { recursive: true, force: true });
}
// force exit: even after the group is signalled, a lingering child pipe can keep the event
// loop alive; the work is done and flushed, so don't wait on it.
process.exit(process.exitCode ?? 0);
