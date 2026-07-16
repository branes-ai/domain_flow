# Cholesky factorization as a recurrence system

The Cholesky factorization of a symmetric positive-definite (SPD) matrix is
`A = L·Lᵀ` with `L` lower-triangular. It is [LU](lu.md) specialized to `A = Aᵀ`: there
is only **one** triangular factor, and — because `A` is SPD — **no pivoting is ever
needed** (every pivot is positive). It is the workhorse solver for SPD systems (normal
equations, covariance, the trailing update in blocked factorizations).

## The symmetric Schur elimination

Let `a(i,j,k)` be entry `(i,j)` after eliminating columns `0..k`. Because `A` is
symmetric only the lower triangle `j ≤ i` is stored and updated; step `k` applies the
symmetric rank-1 Schur complement:

$$
a(i,j,k) \;=\; a(i,j,k-1) \;-\; \frac{a(i,k,k-1)\,a(j,k,k-1)}{a(k,k,k-1)} .
$$

Where LU has a distinct multiplier column `a(i,k)` and pivot row `a(k,j)`, Cholesky's
symmetry collapses both to **column `k`**: `a(i,k,k-1)` and `a(j,k,k-1)`. Cell `(i,j)`
is updated for `k = 0 .. j-1` over the trailing lower triangle `j ≤ i`, `k ≤ j-1`.

## A SARE with a linear schedule

As in [LU](lu.md), the taps `a(i,k,k-1)`, `a(j,k,k-1)` and the pivot `a(k,k,k-1)` are
**affine** (they project onto column `k`), so Cholesky is a **SARE**. And as in LU the
dependence is *benign*: every consumer sits strictly below-and-right of the pivot, so
`τ = [1,1,2]` is a legal linear schedule (as is the free schedule).

## One output face for all of L

The elegant part: a **single** output expression yields the whole factor. Column `j`
finalizes at `k = j-1`, and

$$
L(i,j) \;=\; \frac{a(i,j,j-1)}{\sqrt{a(j,j,j-1)}}
$$

covers **both** cases at once — on the diagonal (`i = j`) it reduces to
`√a(j,j,j-1) = L(j,j)`, and below it (`i > j`) it is the multiplier. So `L` leaves on
the single super-diagonal face `j-k = 1` (`j ≤ i`); the first column (`j = 0`) is
`A[:,0]/√A[0,0]`, read from the `k=-1` seed. Contrast LU, which needs two faces (`i-k=1`
for `U`, `j-k=1` for `L`) because its factors are distinct.

## Relation to `syrk`

The Schur term `a(i,k)·a(j,k)/a(k,k)` is a scaled symmetric rank-1 update — the same
`xxᵀ` shape as [`syrk`](syrk.md), applied here *in place* down the elimination. Blocked
Cholesky iterates over diagonal blocks: **factor** the diagonal block (a small Cholesky,
`potrf`), **solve** the off-diagonal panel against it (`trsm`), then apply the **`syrk`**
trailing update — and this rank-1 shape is the kernel of that trailing update.

## No pivoting — and why that matters

Cholesky needs **no pivoting**: SPD guarantees `a(k,k,k-1) > 0` at every step, so the
`√` and the division are always well-defined and numerically stable. This is why
Cholesky is a *clean* SARE — unlike [LU](lu.md), where the honest factorization would
need partial pivoting (a data-dependent permutation that a static SURE cannot express).
Cholesky sidesteps that obstruction entirely.

---

The executable spec, its schedule, and the animation are on the reference page:
[**Cholesky decomposition**](cholesky_decomposition.md) under *SURE Algorithms →
Linear Algebra*.
