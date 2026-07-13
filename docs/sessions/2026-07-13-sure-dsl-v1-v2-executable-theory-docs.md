# Session: SURE DSL — from Documentation Notation to Executable Confluence Language

**Date:** 2026-07-12 through 2026-07-13

## Objective

Answer "can we execute the DSL recorded in docs/SURE with our simulator?" —
and follow the answer through a v1 implementation, a design review that
reshaped it around Domain Flow's spatial semantics, and the migration of all
theory documents to executable, machine-verified kernels.

## Summary

Three PRs merged (#16, #18, #19), closing issues #15 and #17. The
`system((i,j,k) | constraints) { equations }` notation used throughout
`docs/SURE/` is now a real language: parsed by a header-only front-end,
executed by `dfactl --sure`, and — after the v2 redesign — articulating
exactly the Confluence model of the C++ IR (tensor bound to an oriented face
of the domain). All three theory documents (matmul, QR, conv2d) carry
runnable kernels verified against references.

## Work Items

### 1. DSL v1: the text front-end (issue #15, PR #16)

Gap analysis: the docs notation was pseudocode; the simulator executed only
C++ spec builders and `.dfg` imports — yet its execution model mirrored the
DSL one-to-one. Implemented `include/dfa/sim/sure_parser.hpp` (~650 lines):
tokenizer, recursive-descent parser, affine-index extraction into
`AffineDependency` taps, constraint chains/relations/negative bounds, an
expression AST (+ - * /, sqrt/exp/abs), and line-numbered diagnostics.
`dfactl --sure docs/SURE/matmul.sure` computed `C = A*B` and reproduced the
doc's illegal-schedule argument (`--tau 1,1,0`) numerically. Review round:
parse-time rejection of arrays in equation bodies, divide-by-zero errors,
`--dfg`/`--sure` mutual exclusion, and the test loading the canonical
`.sure` file instead of duplicating it.

### 2. Design review -> DSL v2: oriented confluences (issue #17, PR #18)

The user's design review found v1 captured evaluation semantics but not
spatial semantics: `boundary` was an extent-less, orientation-less value
oracle; `output` identified a face only by accident through a projected
read set; the vocabulary split what the IR treats as one concept. v2
replaced those statements with symmetric `input`/`output` confluence
declarations: equality-pinned face regions in the domain's own coordinates,
extent from the inequalities, location from exactly one equality, and the
orientation *derived* as the domain's outward normal (flow sense from the
keyword: influx/outflux). Input faces sit on the one-step halo where
boundary values are actually read; `data` bindings separate the test bench
from the kernel; a variable can carry both faces (accumulator seed at
k = -1, result at k = K-1).

New machine-checked contracts: static tap-image coverage over declared
input faces with face-dispatched boundary evaluation (uncovered accesses
are hard errors, not fabricated data); face well-formedness; flux
consistency (`tau.n < 0` in, `> 0` out) — a legality class orthogonal to
`tau.theta >= 1`; element range; data binding. Review round hardened the
orientation test into a true supporting-hyperplane check (classifying every
domain point; the centroid test missed splitting planes), bound input
metadata to the tensor actually read, rejected undeclared tap sources at
assembly, and added `validateSureFlux` so CLI `--tau` overrides revalidate
flow directions.

### 3. Executable theory documents (PR #19)

- **QR (the careful refinement)**: the document's case-split equations,
  partial-domain LHSs, and mixed-rank outputs were uniformized into four
  total recurrences over one shared triangular domain ((i,j,k) | k <= j):
  base cases as halo input confluences (with the codim-2 corner escape
  covered by an inequality-pinned face); the s_r/s_norm reductions as
  first-class variables read via affine broadcast taps; q embedded as
  `qhat` normalized everywhere but tapped only on the diagonal where it
  equals q exactly (uniformization slack, documented); R leaving through
  two oriented output confluences — off-diagonal via the i = M-1 face,
  diagonal via the non-axis-aligned k = j face shared with Q. `qr.sure`
  runs the classic 3x3 example: R exact, Q^T*Q = I and Q*R = A to ~1e-15.
- **Conv2D**: the doc's conditional cascade (not actually uniform) and
  pointwise `I_padded` oracle became four total recurrences: the image
  *flows* along the value-preserving anti-diagonal shift (-1,0,+1,0),
  seeded from two disjoint halo faces of one tensor; kernel broadcast;
  separable row/column reduction; padding as explicit halo data.
  `conv2d.sure` verifies Sobel-x over a 4x4 ramp against a direct
  reference. The C-channel section was updated to the same style (and a
  review round caught the stale prose still describing the old case split).
- **Parser refinement**: the conv2d image flow needs one variable seeded
  from several faces of the same tensor; the PR #18 binding rule was
  loosened precisely (declaring faces must read their tensor; tensorless
  faces may read one previously declared tensor, metadata bound to what is
  read; two different tensors in one face remains an error).

## Design Notes Worth Remembering

- Faces as equality-pinned constraint regions generalize to non-axis-
  aligned geometry for free: the QR diagonal output face (k = j) and the
  conv2d anti-diagonal input halo (j - k = -1 in the prefix-sum test) both
  fall out of the same construct that handles box faces.
- The one-equality rule (codimension 1) composes with inequality-pinning
  to cover corner escapes: a codim-2 point set is a face region whose
  second coordinate is confined by `-1 <= k < 0` rather than an equality.
- Deriving orientation (rather than declaring it) makes inconsistent
  normals unrepresentable — but the derivation must be a supporting-
  hyperplane test over the actual domain points, not a centroid heuristic.

## Outcome

- PRs merged: #16 (DSL v1), #18 (DSL v2 confluences), #19 (theory-doc
  migration + executable QR/conv2d); issues #15 and #17 closed
- docs/SURE: three documents rewritten, three verified runnable kernels
  (matmul.sure, qr.sure, conv2d.sure) with ctest coverage
- Sim test suite: 71 -> 73 tests, zero warnings on gcc and clang
- Review rounds: 4 + 2 + 5 + 4 + 3 + 1 CodeRabbit findings resolved across
  the three PRs, all with replies and resolved threads
