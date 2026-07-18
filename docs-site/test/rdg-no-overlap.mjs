#!/usr/bin/env node
// rdg-no-overlap.mjs — validate that the RDG viewer assigns every arc between a node
// pair its OWN channel, so no two arcs draw on top of each other (the lstsq a↔r bug:
// four arcs collapsing onto two curves). Mounts the real viewer under jsdom on the
// committed RDG JSON and asserts the rendered arcs are geometrically distinct.
//
// Overlap test: a non-self arc is a quadratic Bézier `M s Q c e`. Its CHANNEL is fixed
// by the control point c. Two arcs trace the SAME curve iff they run between the same
// node PAIR and share c. We resolve each endpoint to its nearest node (from the rendered
// circles), key on the unordered node pair + quantized c, and flag any reuse. Keying on
// raw endpoint coordinates would MISS reciprocal overlaps: a→r and r→a on one channel
// have their start/end trimmed at different radii (NR vs NR+4), so their coordinates
// differ by a few px even though the arcs coincide.
//
// Run: node test/rdg-no-overlap.mjs   (exits non-zero on any overlap)
import { JSDOM } from 'jsdom';
import { readFileSync } from 'fs';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const here = dirname(fileURLToPath(import.meta.url));
const dom = new JSDOM('<!doctype html><html><body></body></html>');
globalThis.document = dom.window.document;
globalThis.window = dom.window;

const { createRdgViewer } = await import('../src/components/rdg.js');

// round to a coarse grid so floating-point noise never masks / fabricates a collision
const q = (n) => Math.round(n * 10) / 10;
// parse "M sx sy Q cx cy ex ey" → { s:[..], c:[..], e:[..] } (non-self arcs only)
function parseQuad(d) {
  const m = d.match(/^M\s+([-\d.]+)\s+([-\d.]+)\s+Q\s+([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)/);
  if (!m) return null;
  const n = m.slice(1).map(Number);
  return { s: [n[0], n[1]], c: [n[2], n[3]], e: [n[4], n[5]] };
}

// Which ops to validate. lstsq (a↔r, 4 arcs) is the reported case; eig_qr has several
// multi-arc pairs; the rest guards against regressions across the whole catalog.
const OPS = process.argv.slice(2).length
  ? process.argv.slice(2)
  : ['lstsq', 'eig_qr', 'cg', 'lu', 'gemm', 'trmm', 'svd', 'eig_jacobi', 'ldlt', 'cholesky'];

let failures = 0;
for (const op of OPS) {
  const jsonPath = join(here, '..', 'public', 'rdg', `${op}.json`);
  let data;
  try { data = JSON.parse(readFileSync(jsonPath, 'utf8')); }
  catch { console.log(`  skip ${op}: no ${op}.json`); continue; }

  const container = dom.window.document.createElement('div');
  createRdgViewer(container, data);

  // node positions (+ names) from the rendered circles, to resolve each arc's endpoints
  const nodes = [...container.querySelectorAll('g.rdg-node')].map((g) => {
    const c = g.querySelector('circle');
    const t = g.querySelector('text');
    return { name: t?.textContent ?? '?', x: Number(c.getAttribute('cx')), y: Number(c.getAttribute('cy')) };
  });
  const nearest = (pt) => nodes.reduce((best, n) => {
    const dd = (n.x - pt[0]) ** 2 + (n.y - pt[1]) ** 2;
    return dd < best.dd ? { name: n.name, dd } : best;
  }, { name: '?', dd: Infinity }).name;

  const byName = new Map(nodes.map((n) => [n.name, n]));
  const paths = [...container.querySelectorAll('path.rdg-edge')];
  // per-arc record: pair, kind (from the path class), control point c, and |off| =
  // distance from the pair midpoint to c along the normal = how far out the channel bows
  const arcs = paths.map((p) => {
    const g = parseQuad(p.getAttribute('d'));
    if (!g) return null;
    const a = nearest(g.s), b = nearest(g.e);
    const pair = [a, b].sort().join('↔');
    const na = byName.get(a), nb = byName.get(b);
    const mid = [(na.x + nb.x) / 2, (na.y + nb.y) / 2];
    const off = Math.hypot(g.c[0] - mid[0], g.c[1] - mid[1]);
    const kind = p.classList.contains('rdg-affine') ? 'affine' : 'uniform';
    return { pair, kind, c: g.c, off };
  }).filter(Boolean);

  // (1) key: unordered node pair + control point ⇒ one physical channel
  const seen = new Map();
  const collisions = [];
  for (const g of arcs) {
    const key = `${g.pair}@${q(g.c[0])},${q(g.c[1])}`;
    if (seen.has(key)) collisions.push(key); else seen.set(key, true);
  }

  // (2) affine arcs must sit on the OUTERMOST channels of their pair, so the multi-line
  // matrix map has open peripheral space: within a pair, every affine arc's |off| must
  // exceed every uniform arc's |off|.
  const pairs = new Map();
  for (const g of arcs) {
    if (!pairs.has(g.pair)) pairs.set(g.pair, { aff: [], uni: [] });
    (g.kind === 'affine' ? pairs.get(g.pair).aff : pairs.get(g.pair).uni).push(g.off);
  }
  const misplaced = [];
  for (const [pair, { aff, uni }] of pairs) {
    if (!aff.length || !uni.length) continue;
    if (Math.min(...aff) <= Math.max(...uni) + 1e-6) misplaced.push(pair);
  }

  const nonSelf = arcs.length;
  if (collisions.length || misplaced.length) {
    failures++;
    if (collisions.length) {
      console.log(`✗ ${op}: ${collisions.length} overlapping arc(s) of ${nonSelf} pair-arcs`);
      for (const c of collisions) console.log(`    channel reused: ${c}`);
    }
    for (const p of misplaced) console.log(`✗ ${op}: affine arc not on an outer channel for pair ${p}`);
  } else {
    console.log(`✓ ${op}: ${nonSelf} pair-arcs, distinct channels, affine arcs outermost`);
  }
}

if (failures) { console.error(`\nFAILED: ${failures} operator(s) have RDG layout problems`); process.exit(1); }
console.log('\nAll checked RDGs render with no overlapping arcs and affine maps on outer channels.');
