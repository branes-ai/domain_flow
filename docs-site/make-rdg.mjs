#!/usr/bin/env node
/**
 * make-rdg.mjs — regenerate the Reduced Dependency Graph JSON (issue #103).
 *
 * Runs the built `dfactl` over each catalog spec and writes its RDG to
 * docs/rdg/<op>.json. These are COMMITTED (the docs / GitHub-Pages CI has no C++
 * toolchain); sync-content.mjs copies docs/rdg/ to public/rdg/ at build time, and
 * the rdg.js viewer fetches them into <div class="rdg" data-src="rdg/<op>.json">.
 * Re-run after changing a spec or the RDG emitter:  npm run rdg
 *
 * Unlike the schedule animations, the RDG is a function only of the recurrence
 * STRUCTURE — one node per variable, one arc per dependence — so it is invariant to
 * the domain size and the operand data. We therefore run `--emit-rdg` directly on
 * the committed specs (no scaling, no data fill).
 *
 * Locates dfactl under a build tree's sim/ dir, or via the DFACTL env var.
 */
import { execFileSync } from 'child_process';
import { existsSync, mkdirSync, readdirSync } from 'fs';
import { join } from 'path';

const REPO = join(import.meta.dirname, '..');
const OUT = join(REPO, 'docs', 'rdg');
const SURE = join(REPO, 'docs', 'SURE');

// The catalog operators the RDG epic (#102) targets — one graph per operator.
const OPERATORS = [
  'axpy', 'dot', 'nrm2', 'rot',                  // BLAS L1
  'gemv', 'ger', 'trmv', 'symv', 'syr', 'syr2',  // BLAS L2
  'gemm', 'syrk', 'syr2k', 'trmm', 'symm',       // BLAS L3
  'lu', 'cholesky', 'ldlt', 'trsolve',           // factorizations & solvers
  'lu_neighbor',                                 // neighbour pivoting — the uniformized (SURE) pivot compare-exchange
  'lstsq',                                       // least squares via QR — the augmented-column pipeline
  'stationary',                                  // Jacobi stationary iteration (solvers)
  'cg',                                         // conjugate gradient (solvers)
  'eig_jacobi',                                 // symmetric eigensolver — one Jacobi rotation
  'eig_qr',                                     // symmetric eigensolver — one Householder tridiag step
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
  console.error('make-rdg: could not find dfactl. Build it first:');
  console.error('  cmake --build build/<preset> --target dfactl');
  console.error('or set DFACTL=/path/to/dfactl');
  process.exit(1);
}

mkdirSync(OUT, { recursive: true });
let n = 0;
for (const op of OPERATORS) {
  const src = join(SURE, `${op}.sure`);
  if (!existsSync(src)) { console.error(`  MISSING spec: ${src}`); continue; }
  const out = join(OUT, `${op}.json`);
  execFileSync(dfactl, ['--sure', src, '--emit-rdg', out], { stdio: 'pipe' });
  console.log(`  ${op} → docs/rdg/${op}.json`);
  ++n;
}
console.log(`make-rdg: wrote ${n} RDG file(s) using ${dfactl}`);
