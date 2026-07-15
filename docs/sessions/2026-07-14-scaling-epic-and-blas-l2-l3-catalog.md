# Session: Scaling & Distribution epic, and the BLAS L2/L3 operator catalog

**Date:** 2026-07-14 through 2026-07-15

## Objective

Continue the SURE catalog and docs work from where the DSL session left off:
finish the **Givens-QR flagship**, build out the whole **Scaling & Distribution**
docs section (epic #68), then derive and verify every remaining **BLAS Level-2 and
Level-3** operator as an executable SURE — restructuring the docs-site so a designer
can read the reference (algorithm + SURE + animation) without wading through the
derivation prose.

## Summary

**19 PRs merged (#77–#86 and #88–#96 — #87 is the restructure *issue*)**; epic #68
and 19 issues closed. The Scaling &
Distribution section is a complete narrative arc (tiling → halo/collective →
uniformization → hierarchy/DMM → composition), anchored by an executable Givens-QR;
the schedule animator gained a `data-tile` overlay that *shows* halo vs collective
traffic. The operator catalog grew from BLAS L1 to a full **L1 + L2 + L3** set (20
operators), each with an executable spec, a dual-compiler regression test, a
derivation, and a schedule animation. Along the way the docs-site was reorganized so
**Theory** holds the derivation methodology and **SURE Algorithms** holds lean
reference pages, and three long-standing math-rendering failures in the geometry docs
were repaired.

## Work Items

### 1. Givens-QR flagship (issue #72, PR #77)

The Gentleman–Kung Givens-rotation QR as an executable recurrence system
(`qr_givens.sure` + a tall `qr_givens_tall.sure`), verified to the exact 3×3 factor
and the sign-robust `RᵀR = AᵀA` on over-determined inputs. The honest finding: it is
still a *SARE* (the diagonal taps `r(i-1,p,p)`, `a(i,p-1,p)` read the pivot column),
**but** unlike MGS's `srp(M-1,…)` reduce-then-broadcast, those are pure diagonal
broadcasts with **no reduction** — nearest-neighbour along `+q`, so they tile with
local communication, one internal-confluence extension away from a true SURE.

### 2. Scaling & Distribution epic (#68 → #69–#75, PRs #78–#82)

- **#70 Tiling** — blocked matmul as a *partition of the same index space* (uniformity
  makes the recurrence scale-invariant), with the `data-tile` animation overlay.
- **#71 Halo vs collective** — the cross-tile dependency dichotomy, grounded in
  `conv2d`'s stencil (halo) vs `dot`'s reduction (collective), with the cost argument.
- **#73 Hierarchy & DMM** — PE → tile → KPU → SoC → cluster, the energy–delay–distance
  cost, and the Distributed Memory Machine's collective primitives. Reconciled the
  acronym collision honestly: **EDDO** = *Explicit Decoupled Data Orchestration*, kept
  distinct from the energy–delay–distance cost model.
- **#74 Composition** — the section capstone: the DFG edge as the site of
  inter-operator collectives, fuse-vs-materialize, and the coupled tiling+placement
  mapping problem.
- **#75 Tiled animations** — the emitter now emits each variable's dependence taps
  (`A·p + b`); the viewer replays them to draw cross-tile edges **green** (halo,
  `|Δtile|=1`) vs **red** (collective). matmul draws 0 collective edges; the Givens
  QR lights up ~640 red edges on its affine diagonal taps.
- **#69** landing page — reconciled as already satisfied.

### 3. BLAS Level-2 catalog (issues #31–#36, PRs #83–#89)

Six operators, each a pure SURE with the recurring patterns collected in a new Theory
doc (`matrix_vector_operators.md`):

- **gemv** `y = αAx + βy` — depth-1 feed for `A`; α/β project-and-pipeline; βy epilogue.
- **ger** `A += αxyᵀ` — fully-parallel outer product; depth-2 feed/drain.
- **trmv** `x := Tx` — the first **non-box** (triangular) domain; diagonal output ⇒
  `τ_j > τ_i` (box-default `τ=[1,1,1]` rejected).
- **symv** — the **two-face symmetric feed** (lower `A[i][j]`, reflected upper `A[j][i]`).
- **syr** `A += αxxᵀ` / **syr2** `A += α(xyᵀ+yxᵀ)` — ger + triangular domain.

Also added a **BLAS L2** catalog-nav subsection and a **"Reading BLAS names"**
reference page decoding the `[type][structure][operation]` shorthand.

### 4. Docs-site restructure (issue #87, PR #90)

Separated *derivation* from *reference*. **Theory** became an explicit, ordered
methodology track (tensor structure → matmul → alignment → geometric transformation →
pipeline schedules → conv2d → QR → matrix-vector derivations). **SURE Algorithms /
BLAS L2** got dedicated **lean** pages (intro + SURE + animation), moved out of
Theory. Crucially, **schedule animations were generated for all six L2 operators**
(they had none) — the degenerate depth-1/2 feed axis renders fine, as the L1 ops
already animate equally thin domains.

### 5. Math-rendering repair in the geometry docs (PR #94)

The "Aligning convex hulls" and "Geometric Transformation" Theory pages showed raw
LaTeX from three compounding issues in the original brain-dump markdown:
(1) **concatenated** display blocks (`$$…$$$$…$$`) that remark-math can't parse;
(2) the **single-line** `$$…$$` form that this site renders inline, not as display;
(3) deeply **indented nested bullets** parsed as *code blocks*, so their inline `$…$`
rendered as literal text. Split the blocks, converted to the multi-line form, and
flattened the list nesting — while preserving the fenced Python code block.

### 6. BLAS Level-3 catalog (issues #37–#41, PRs #91–#96)

- **gemm** `C = αAB + βC` — the 3-D reduction cube; α folded in, βC epilogue.
- **syrk** `C = αAAᵀ + βC` — triangular output; single `A` feeds both taps; the
  `A(j,k)` tap enters on the **super-diagonal** halo `i-j=-1` ⇒ `τ=[2,1,1]`.
- **syr2k** `C = α(ABᵀ+BAᵀ) + βC` — two operands, four operand streams, fused.
- **trmm** `B = αTB` — triangular k-extent; **free-schedule-only**: the super-diagonal
  `B`-feed and the diagonal output are parallel faces (same normal `(-1,0,1)`) with
  opposite flux, so no linear `τ` exists (like MGS-QR). An honest, instructive entry.
- **symm** `C = αAB + βC` — symv's two-face symmetric feed on the 3-D cube.

## Design Notes Worth Remembering

- **The feed-axis toolkit.** A matrix operand indexed by both domain axes and read once
  needs a **depth-1 feed axis** to enter on a face (`gemv`/`gemm`); a fully-parallel
  operator whose *result* is also a matrix needs a **depth-2 feed/drain** axis
  (inject → add → drain, `ger`/`syr`/`syr2`), the same shape as `scal`/`axpy`.
- **Triangular geometry constrains the schedule.** A diagonal output (`trmv`) forces
  `τ_j > τ_i`; a super-diagonal operand feed (`syrk`) forces `τ_i > τ_j`; and when a
  near-diagonal feed is *parallel* to a diagonal output with opposite flux (`trmm`),
  **no linear schedule exists at all** — the free schedule is required. These are read
  directly off the derived face normals, before any tiling.
- **Symmetric storage is a confluence, not a recurrence.** `symv`/`symm` read one
  stored triangle via a two-face feed (lower `A[i][k]`, reflected upper `A[k][i]`); the
  recurrence stays a pure SURE.
- **`data-tile` makes tileability visible.** Colouring by tile block + replaying the
  emitted taps to draw halo (green) vs collective (red) cross-tile edges turns the
  abstract uniform-vs-affine claim into something a viewer can see sweep.
- **Honesty over acronym reuse.** EDDO (Explicit Decoupled Data Orchestration) is the
  architecture class; energy–delay–distance is the cost model — kept distinct.
- **The docs-site renders display math only from the multi-line `$$` block form**, and
  4-space indentation is an indented-code-block trap — both bit the geometry docs.

## Outcome

- **PRs merged:** #77–#86 and #88–#96 (19; #87 is the restructure issue, landed via
  #90); **issues closed:** #68 (epic) plus 19 issues — #69–#75, #31–#41, #87.
- **Catalog:** BLAS **L1 (9) + L2 (6) + L3 (5)** = 20 executable operators, each with a
  spec, a dual-compiler test, a derivation, and a schedule animation.
- **Scaling & Distribution** section complete (7 pages) with an executable Givens-QR
  flagship and tile/edge animations.
- **Docs-site** reorganized (Theory = derivation, SURE Algorithms = lean reference);
  geometry-doc math rendering repaired.
- **Sim test suite:** 26 test files, zero warnings on gcc and clang; every
  `dfactl_sure_*` emit test now genuinely exercises `--emit-schedule`.
- Every PR merged green with all CodeRabbit review threads resolved.
