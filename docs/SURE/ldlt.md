# LDLᵀ factorization as a recurrence system

The LDLᵀ factorization of a symmetric matrix is `A = L·D·Lᵀ` with `L` unit-lower-
triangular and `D` diagonal. It is the **square-root-free** sibling of
[Cholesky](cholesky.md): factoring out the diagonal `D` avoids the `√`, so LDLᵀ works
for symmetric **indefinite** `A` (where `D` may have mixed signs) — exactly the case a
Cholesky `√` of a negative pivot would reject.

## Same elimination as Cholesky

The elimination is *identical* to Cholesky's — the same symmetric Schur update over the
trailing lower triangle `j ≤ i`, `k ≤ j-1`:

$$
a(i,j,k) \;=\; a(i,j,k-1) \;-\; \frac{a(i,k,k-1)\,a(j,k,k-1)}{a(k,k,k-1)} .
$$

So LDLᵀ is likewise a **SARE** with a benign forward affine dependence, and the same
linear schedule `τ = [1,1,2]` (and the free schedule). What differs is only the
*extraction*.

## Extraction — a unit L and a diagonal D

Column `j` finalizes at `k = j-1`, and the factors read off directly, with **no square
root**:

$$
D(j) \;=\; a(j,j,j-1),
\qquad
L(i,j) \;=\; \frac{a(i,j,j-1)}{a(j,j,j-1)}\ \ (i > j),\quad L(j,j)=1 .
$$

Where Cholesky folds the pivot's `√` into a single output face, LDLᵀ keeps `D`
separate, so it uses **two** output faces: the unit-lower multipliers `L` on the
super-diagonal `j-k = 1` (`i > j`), and the diagonal pivots `D` on the diagonal `i = j`
at the same `k = j-1`. The first column of `L` and `D(0)` come from the `k=-1` seed.

## Why square-root-free matters

Cholesky's `L(i,j) = a(i,j)/√a(j,j)` requires every pivot `a(j,j,j-1) > 0` — i.e. `A`
SPD. LDLᵀ replaces the `√` with the **signed** pivot `D(j) = a(j,j,j-1)` and a plain
division, so it factors any symmetric `A` whose leading minors are non-singular,
**including indefinite ones**. For the bundled `A = [[2,2,2],[2,-1,-1],[2,-1,3]]` the
pivots are `D = diag(2,-3,4)` — mixed signs, an indefinite matrix Cholesky cannot touch.

(True symmetric-indefinite robustness uses **Bunch–Kaufman** `2×2` pivoting to avoid a
tiny/zero pivot; like [LU's partial pivoting](lu.md#partial-pivoting-is-not-a-sure), that
is a data-dependent choice a static SURE cannot express. The spec here is the
unpivoted LDLᵀ, which needs non-singular leading minors.)

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence. LDLᵀ shares Cholesky's elimination, so
it shares Cholesky's RDG exactly — a **single** variable `a` with four self-loops:

| arc | dependence map | reads |
| --- | --- | --- |
| `a → a` | translation `[0,0,1]ᵀ` | `a(i,j,k-1)`, the previous Schur value |
| `a → a` | affine `(i,j,k) ↦ (i,k,k-1)` | column `k`, row `i`: `a(i,k,k-1)` |
| `a → a` | affine `(i,j,k) ↦ (j,k,k-1)` | column `k`, row `j`: `a(j,k,k-1)` |
| `a → a` | affine `(i,j,k) ↦ (k,k,k-1)` | the pivot `a(k,k,k-1)` |

One **translation vector** and three **matrix** maps projecting onto column/pivot `k`, so
LDLᵀ is a **SARE**. The `√`-free extraction changes only the *output faces*, not the
elimination's dependence structure — which is why the RDG is identical to
[Cholesky](cholesky.md)'s. The affine arcs render dashed, carrying their matrix; the
uniform Schur arc renders solid.

<div class="rdg" data-src="rdg/ldlt.json" data-height="560"></div>

---

The executable spec, its schedule, and the animation are on the reference page:
[**LDLᵀ decomposition**](ldlt_decomposition.md) under *SURE Algorithms → Linear Algebra*.
