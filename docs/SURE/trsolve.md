# Triangular solve as a recurrence system

Triangular solve — **forward substitution** for `L x = b` (lower-triangular `L`) and
its mirror **back substitution** for `U x = b` — is the substitution engine behind
*every* direct solver. Once [LU](lu.md), [Cholesky](cholesky.md), or [LDLᵀ](ldlt.md)
has produced triangular factors, the actual system is finished by two triangular
solves. BLAS calls the vector form `trsv` and the multiple-right-hand-side form
`trsm`.

## The recurrence

For a lower-triangular `L`, forward substitution resolves the unknowns top to bottom:

$$
x(i) \;=\; \frac{1}{L(i,i)}\left( b(i) \;-\; \sum_{j<i} L(i,j)\, x(j) \right),
\qquad 0 \le i < N .
$$

Row `i` needs **all earlier** `x(j)`, `j < i`, so the solve is a *sequentially
coupled* triangular recurrence — the classic wavefront over a triangular domain.

## trmv run backwards — and why it is a SARE

Structurally this is [`trmv`](trmv.md) (the lower-triangular mat-vec) turned inside
out. `trmv` **feeds** `x(j)` on the face and reduces `L(i,j)·x(j)` to a product;
`trsv` must **solve** for `x(j)` and feed the result back into the rows below. That
feedback is the whole story:

- The reduction `acc(i,j,k) = acc(i,j-1,k) + L(i,j)·x(j)` accumulates
  `Σ_{j'≤j} L(i,j') x_{j'}` along `+j`, exactly as `trmv` does.
- But `x(j)` is now a **computed** value, produced at the diagonal cell `(j,j)` and
  consumed by every later row `i > j`. Reading it at cell `(i,j)` is an **affine
  tap** onto the diagonal `(j,j)` — precisely the projection [LU](lu.md) uses to read
  its pivot `a(k,k,k-1)`.

So `trsv` is a **SARE**, not a pure SURE: the solved `x` is broadcast down the
columns. The dependence is *forward and benign* — the source row `j` is strictly
above the consumer row `i` — so the data-flow is well-founded (`x_0` before `x_1`
before …).

## The solve on the diagonal

`acc(i,i-1)` is the off-diagonal sum `Σ_{j<i} L(i,j) x_j`, so the solve reads the
accumulator one column back and divides:

$$
x(i) \;=\; \frac{b(i) - acc(i,i-1)}{L(i,i)} ,
$$

leaving on the diagonal face `i - j = 0` (the same output geometry as `trmv`, whose
completed sum also exits on the diagonal). The right-hand side `b(i)` and the row
`L(i,·)` enter on the `(i,j)` feed face `k = -1`, just as `trmv` feeds `T` and `x`.

## Why the schedule is free-only

Forward substitution **fuses a reduction and a division at one lattice point**. On
row `i` the off-diagonal sum completes *and* `x_i` is divided out at the very same
diagonal cell `(i,i)`. A linear schedule assigns a single time to a lattice point, so
it cannot order "finish the sum" before "do the divide" there. Concretely, with all
equations sharing the system domain (per-equation domains are a follow-up), the
diagonal also carries a **dead** accumulator cell `acc(i,i)` that reads the same-point
solve `x(i,i)`. Both are **zero-slack self-dependences**: `τ · 0 = 0` for *every*
linear `τ`, so no linear schedule is legal.

This is the same conclusion as [`trmm`](trmm.md), but for a different reason —
`trmm` fails on **flux** (a feed face and an output face are parallel with opposite
sign), whereas `trsv` is flux-legal (`τ_j > τ_i` orients the diagonal outflux, as in
`trmv`) and fails only on the **same-point reduce-then-divide fusion**. The
data-flow-earliest **free** schedule resolves it: it is exactly the sequential
substitution wavefront — row `i` fires only after every `x_j`, `j < i`, is resolved.

## trsv and trsm

`trsm` (multiple right-hand sides, `L X = B`) is `trsv` replicated across the columns
of `B`: add an independent RHS axis and the same triangular recurrence solves each
column. Nothing about the schedule changes — the extra axis is fully parallel — so the
vector spec here captures the essential geometry.

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence. `trsv`'s four variables — the row `Lm`,
the right-hand side `bb`, the reduction `acc`, and the solution `x` — wire almost entirely
with translation vectors, with one telling exception:

| arc | dependence map | reads |
| --- | --- | --- |
| `Lm → Lm` | `[0,0,1]ᵀ` | `L` held on the feed axis |
| `bb → bb` | `[0,0,1]ᵀ` | `b` held on the feed axis |
| `acc → acc` | `[0,1,0]ᵀ` | the reduction chains along `+j` |
| `Lm → acc` | `[0,0,1]ᵀ` | `L(i,j)` enters the product |
| `x → acc` | affine `(i,j,k) ↦ (j,j,k)` | the solved `x(j)`, from the diagonal `(j,j)` |
| `bb → x` | `[0,0,1]ᵀ` | `b(i)` feeds the solve |
| `acc → x` | `[0,1,0]ᵀ` | the off-diagonal sum feeds the solve |
| `Lm → x` | `[0,0,1]ᵀ` | the pivot `L(i,i)` divides |

Seven arcs are translation vectors; the single **affine** arc `x → acc` — the map
`(i,j,k) ↦ (j,j,k)` reading the solved `x` off the diagonal — is the broadcast that makes
`trsv` a **SARE**. Where LU and Cholesky carry three affine pivot arcs, `trsv` carries
just one, but a single matrix arc is enough to make the whole system affine. It renders
dashed in the graph; the seven translations render solid.

<div class="rdg" data-src="rdg/trsolve.json" data-height="560"></div>

---

The executable spec, its free schedule, and the animation are on the reference page:
[**Triangular solve**](triangular_solve.md) under *SURE Algorithms → Linear Algebra*.
