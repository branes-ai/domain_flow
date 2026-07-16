# Cholesky decomposition

`cholesky` factors a symmetric positive-definite `A` as **`A = L·Lᵀ`** with `L`
lower-triangular — [LU](lu_decomposition.md) specialized to `A = Aᵀ` (one factor, no
pivoting). For the derivation — the symmetric Schur update, why one output face
suffices, and the relation to `syrk` — see [**Cholesky factorization**](cholesky.md)
under *Theory*.

## The SURE

The symmetric rank-1 Schur update over the trailing lower triangle `j ≤ i`, `k ≤ j-1`.
A single output face gives all of `L`: `a(i,j,k)/√a(j,j,k)` yields `√`(pivot) on the
diagonal and the multiplier below.

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, j <= i, 0 <= k < N, k <= j-1) {
    a(i,j,k) = a(i,j,k-1) - a(i,k,k-1) * a(j,k,k-1) / a(k,k,k-1);       // symmetric Schur update
}
input  A                      : a(i,j,-1) = A[i][j];       // lower triangle of A
output L  on  j-k = 1, j<=i   : L[i][j] = a(i,j,k) / sqrt(a(j,j,k));
```

Executable spec: `docs/SURE/cholesky.sure`. **Schedule:** a SARE with a benign forward
affine dependence (like LU), so the canonical linear **τ = [1,1,2]** is legal, as is the
free schedule. The wavefront sweeps the triangular trailing domain.

For SPD `A = [[4,2,2],[2,5,3],[2,3,6]]`: `L = [[2,0,0],[1,2,0],[1,1,2]]`, and
`L·Lᵀ = A`.

<div class="schedule-anim" data-src="schedules/cholesky-free.json" data-height="440" data-fps="4"></div>
