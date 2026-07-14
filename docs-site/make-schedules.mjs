#!/usr/bin/env node
/**
 * make-schedules.mjs — regenerate the SURE schedule-animation JSON (issue #64).
 *
 * Runs the built `dfactl` over each catalog spec and writes the free + linear
 * schedules to docs/schedules/<op>-{free,linear}.json. These are COMMITTED (the
 * docs build / GitHub-Pages CI has no C++ toolchain); sync-content.mjs copies
 * them to docs-site/public/schedules/ at build time. Re-run this after changing
 * a .sure spec or the emitter:  npm run schedules
 *
 * Locates dfactl under a build tree's sim/ dir, or via the DFACTL env var.
 */
import { execFileSync } from 'child_process';
import { existsSync, mkdirSync, readdirSync } from 'fs';
import { join } from 'path';

const REPO = join(import.meta.dirname, '..');
const OUT = join(REPO, 'docs', 'schedules');
const SURE = join(REPO, 'docs', 'SURE');

// Operators to render, and which schedules to emit for each. `spec` (optional)
// overrides the source .sure name — matmul uses the 15x15x15 visualization-scale
// spec so the wavefront geometry materializes, while the output keeps the `op`
// name the docs page references.
const OPERATORS = [
  { op: 'matmul', spec: 'matmul15', schedules: ['free', 'linear'] },
];

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
for (const { op, spec: specName, schedules } of OPERATORS) {
  const spec = join(SURE, `${specName ?? op}.sure`);
  if (!existsSync(spec)) { console.error(`  MISSING spec: ${spec}`); continue; }
  for (const kind of schedules) {
    const out = join(OUT, `${op}-${kind}.json`);
    execFileSync(dfactl, ['--sure', spec, '--schedule', kind, '--emit-schedule', out], { stdio: 'pipe' });
    console.log(`  ${op} (${kind}, ${specName ?? op}.sure) → docs/schedules/${op}-${kind}.json`);
    ++n;
  }
}
console.log(`make-schedules: wrote ${n} schedule file(s) using ${dfactl}`);
