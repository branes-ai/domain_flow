#!/usr/bin/env node
// hull-rank.mjs — validate the affine-rank classifier that decides whether a domain of
// computation can form a 3-D convex hull (schedule-anim.js). ConvexGeometry (QuickHull) needs
// FOUR affinely-independent points; on a coplanar/collinear set it throws or yields a degenerate
// sliver, so the hull builder must classify rank up front and only build a solid for rank 3,
// outlining lower-rank domains with a bounding box. Many shipped schedules have 1-D/2-D domains
// (dot/asum = lines, gemv/axpy/trmv = planes), so this path is common — a wrong classification
// there would render garbage. This test pins the classifier on two-point, collinear, planar, and
// full-3-D fixtures.
//
// Run: node test/hull-rank.mjs   (exits non-zero on any mismatch)
import { affineRank } from '../src/components/schedule-anim.js';

const cases = [
  // [label, points, expectedRank]
  ['empty',                     [],                                              0],
  ['single point',              [[2, 3, 4]],                                     0],
  ['two coincident points',     [[1, 1, 1], [1, 1, 1]],                          0],
  ['two distinct points (line)',[[0, 0, 0], [3, 0, 0]],                          1],
  ['three collinear',           [[0, 0, 0], [1, 0, 0], [2, 0, 0]],               1],
  ['collinear off-axis',        [[0, 0, 0], [1, 1, 1], [2, 2, 2], [5, 5, 5]],    1],
  ['triangle in z=0 (plane)',   [[0, 0, 0], [1, 0, 0], [0, 1, 0]],               2],
  ['coplanar square (z=0)',     [[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]],    2],
  ['tilted plane',              [[0, 0, 0], [1, 0, 1], [0, 1, 1], [1, 1, 2]],    2],
  ['tetrahedron (full 3-D)',    [[0, 0, 0], [1, 0, 0], [0, 1, 0], [0, 0, 1]],    3],
  ['unit cube corners',
    [[0,0,0],[1,0,0],[0,1,0],[0,0,1],[1,1,0],[1,0,1],[0,1,1],[1,1,1]],           3],
];

let failures = 0;
for (const [label, pts, want] of cases) {
  const got = affineRank(pts);
  const ok = got === want;
  if (!ok) failures++;
  console.log(`  ${ok ? 'ok  ' : 'FAIL'} rank=${got} (want ${want})  ${label}`);
}

// the classifier's contract, restated as the two boundaries the hull builder relies on:
//   rank <  3  ⇒ bounding-box fallback   (planar face / line)
//   rank == 3  ⇒ solid ConvexGeometry
const solidCases = cases.filter(([, , r]) => r === 3);
const boxCases   = cases.filter(([, , r]) => r < 3);
console.log(`\n  ${solidCases.length} full-dimensional fixtures → solid hull; `
  + `${boxCases.length} rank<3 fixtures → bounding-box outline`);

if (failures) { console.error(`\nhull-rank: ${failures} FAILED`); process.exit(1); }
console.log('\nhull-rank: all passed');
