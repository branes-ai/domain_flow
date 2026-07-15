#!/usr/bin/env node
/**
 * make-schedules.mjs — regenerate the SURE schedule-animation JSON (issue #64).
 *
 * Runs the built `dfactl` over each catalog spec and writes the schedule(s) to
 * docs/schedules/<op>-<kind>.json. These are COMMITTED (the docs / GitHub-Pages
 * CI has no C++ toolchain); sync-content.mjs copies them to public/schedules/ at
 * build time. Re-run after changing a spec or the emitter:  npm run schedules
 *
 * VISUALIZATION SCALE. The catalog specs are tiny (N=4, so the numerics stay
 * checkable), but the wavefront geometry only reads at a larger size. Because the
 * schedule is a function of the DOMAIN, not the operand values, we scale a copy of
 * the spec for the animation: bump the size parameter(s) and scalar-fill the data
 * bindings (unused by the emit). The committed specs are never touched.
 *
 * Locates dfactl under a build tree's sim/ dir, or via the DFACTL env var.
 */
import { execFileSync } from 'child_process';
import { existsSync, mkdirSync, mkdtempSync, readdirSync, readFileSync, writeFileSync, rmSync } from 'fs';
import { join } from 'path';
import { tmpdir } from 'os';

const REPO = join(import.meta.dirname, '..');
const OUT = join(REPO, 'docs', 'schedules');
const SURE = join(REPO, 'docs', 'SURE');

// Per operator: the size parameter(s) to scale for visualization, and which
// schedules to emit. matmul (a 3-D volume) needs 15^3 for the wavefront plane to
// materialize and shows free vs linear; the L1 ops are 1–2-D (a chain or a strip),
// so a modest size and their declared linear schedule suffice.
const OPERATORS = [
  { op: 'matmul', scale: { M: 15, N: 15, K: 15 }, schedules: ['free', 'linear'] },
  // QR (Modified Gram-Schmidt): a SARE with affine/broadcast taps over a shared
  // triangular domain — no global linear tau exists, so only the free schedule.
  { op: 'qr', scale: { M: 8, N: 8 }, schedules: ['free'] },
  // N=12 so a T=4 tiling gives 3 blocks/axis — enough for the affine diagonal tap
  // r(i-1,p,p) to span >1 tile and show up as a long-range (collective) edge (#75).
  { op: 'qr_givens', scale: { M: 12, N: 12 }, schedules: ['free'] },
  // BLAS L1 reductions — a wavefront point sweeping the accumulation chain
  { op: 'dot',   scale: { N: 16 }, schedules: ['linear'] },
  { op: 'nrm2',  scale: { N: 16 }, schedules: ['linear'] },
  { op: 'asum',  scale: { N: 16 }, schedules: ['linear'] },
  { op: 'iamax', scale: { N: 16 }, schedules: ['linear'] },
  // BLAS L1 elementwise maps — a wavefront sweeping the systolic strip
  { op: 'axpy',  scale: { N: 12 }, schedules: ['linear'] },
  { op: 'scal',  scale: { N: 12 }, schedules: ['linear'] },
  { op: 'copy',  scale: { N: 12 }, schedules: ['linear'] },
  { op: 'swap',  scale: { N: 12 }, schedules: ['linear'] },
  { op: 'rot',   scale: { N: 12 }, schedules: ['linear'] },
  // BLAS L2 — matrix-vector over an (i,j) plane with a thin feed axis k. The
  // wavefront sweeps the plane (matvec reductions) or the triangle (trmv/syr*).
  { op: 'gemv',  scale: { M: 12, N: 12 }, schedules: ['linear'] },
  { op: 'ger',   scale: { M: 12, N: 12 }, schedules: ['linear'] },
  { op: 'trmv',  scale: { N: 12 },        schedules: ['linear'] },
  { op: 'symv',  scale: { N: 12 },        schedules: ['linear'] },
  { op: 'syr',   scale: { N: 12 },        schedules: ['linear'] },
  { op: 'syr2',  scale: { N: 12 },        schedules: ['linear'] },
  // BLAS L3 — matrix-matrix, the full 3-D reduction cube (like matmul)
  { op: 'gemm',  scale: { M: 12, N: 12, K: 12 }, schedules: ['linear'] },
  { op: 'syrk',  scale: { N: 12, K: 12 },       schedules: ['linear'] },
  { op: 'syr2k', scale: { N: 12, K: 12 },       schedules: ['linear'] },
  // triangular in-place matmul — free-schedule only (no linear tau; cf. qr)
  { op: 'trmm',  scale: { M: 12, N: 12 },       schedules: ['free'] },
  { op: 'symm',  scale: { M: 12, N: 12, K: 12 }, schedules: ['linear'] },
];

// Scale the size parameter(s) and scalar-fill the braced data bindings. The values
// are irrelevant to the schedule (which depends only on the domain).
function scaleSpec(text, scale, op) {
  let s = text;
  for (const [p, v] of Object.entries(scale || {})) {
    const re = new RegExp('\\b' + p + '\\s*=\\s*\\d+;', 'g');
    // a scale key that matches no declared parameter would silently leave the
    // spec at its (tiny) checked size — warn rather than emit a wrong animation
    if (!re.test(s)) console.warn(`  WARNING: ${op}: scale param '${p}' matches no parameter in the spec`);
    s = s.replace(re, `${p} = ${v};`);
  }
  return s.replace(/data\s+(\w+)\s*=\s*\{[\s\S]*?\};/g, 'data $1 = 1;');
}

function findDfactl() {
  if (process.env.DFACTL && existsSync(process.env.DFACTL)) return process.env.DFACTL;
  const buildRoot = join(REPO, 'build');
  if (existsSync(buildRoot)) {
    for (const d of readdirSync(buildRoot)) {
      const p = join(buildRoot, d, 'sim', 'dfactl');
      if (existsSync(p)) return p;
    }
  }
  return null;
}

const dfactl = findDfactl();
if (!dfactl) {
  console.error('make-schedules: could not find dfactl. Build it first:');
  console.error('  cmake --build build/<preset> --target dfactl');
  console.error('or set DFACTL=/path/to/dfactl');
  process.exit(1);
}

mkdirSync(OUT, { recursive: true });
let n = 0;
for (const { op, scale, schedules } of OPERATORS) {
  const src = join(SURE, `${op}.sure`);
  if (!existsSync(src)) { console.error(`  MISSING spec: ${src}`); continue; }
  // scale a temp copy named <op>.sure (in a private dir) so dfactl reports the
  // right operator label without a predictable path in the shared tmpdir
  const specDir = mkdtempSync(join(tmpdir(), 'make-schedules-'));
  const spec = join(specDir, `${op}.sure`);
  writeFileSync(spec, scaleSpec(readFileSync(src, 'utf8'), scale, op));
  try {
    for (const kind of schedules) {
      const out = join(OUT, `${op}-${kind}.json`);
      execFileSync(dfactl, ['--sure', spec, '--schedule', kind, '--emit-schedule', out], { stdio: 'pipe' });
      const tag = scale ? ` [${Object.entries(scale).map(([k, v]) => `${k}=${v}`).join(',')}]` : '';
      console.log(`  ${op} (${kind})${tag} → docs/schedules/${op}-${kind}.json`);
      ++n;
    }
  } finally {
    rmSync(specDir, { recursive: true, force: true });
  }
}
console.log(`make-schedules: wrote ${n} schedule file(s) using ${dfactl}`);
