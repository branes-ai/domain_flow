# LDLᵀ decomposition

`ldlt` factors a symmetric (possibly **indefinite**) `A` as **`A = L·D·Lᵀ`** with `L`
unit-lower-triangular and `D` diagonal — the **square-root-free** sibling of
[Cholesky](cholesky_decomposition.md). For the derivation — why the `√`-free form
handles indefinite `A`, and the two-face extraction — see
[**LDLᵀ factorization**](ldlt.md) under *Theory*.

## The SURE

The elimination is identical to Cholesky's symmetric Schur update; only the extraction
differs — `D` keeps the signed pivot, `L` the unit-lower multiplier, with no `√`.

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, j <= i, 0 <= k < N, k <= j-1) {
    a(i,j,k) = a(i,j,k-1) - a(i,k,k-1) * a(j,k,k-1) / a(k,k,k-1);       // symmetric Schur update
}
input  A                              : a(i,j,-1) = A[i][j];       // lower triangle of A
output L  on  j-k = 1, j < i          : L[i][j] = a(i,j,k) / a(j,j,k);  // unit-lower multipliers
output D  on  i-k = 1, i = j          : D[i]    = a(i,j,k);             // diagonal pivots (signed)
```

Executable spec: `docs/SURE/ldlt.sure`. **Schedule:** a SARE with a benign forward
affine dependence (like Cholesky/LU), so the canonical linear **τ = [1,1,2]** is legal,
as is the free schedule. The wavefront sweeps the triangular trailing domain.

For the **indefinite** `A = [[2,2,2],[2,-1,-1],[2,-1,3]]`:
`L = [[1,0,0],[1,1,0],[1,1,1]]`, `D = diag(2,-3,4)` (mixed signs), and `L·D·Lᵀ = A`.

<div class="schedule-anim" data-src="schedules/ldlt-free.json" data-height="440" data-fps="4"></div>
