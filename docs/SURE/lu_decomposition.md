# LU decomposition

`lu` factors **`A = L·U`** by right-looking Gaussian elimination (no pivoting), `L`
unit-lower-triangular and `U` upper-triangular. For the derivation — why it is a SARE,
why a linear schedule exists, and why partial pivoting is not expressible — see
[**LU factorization**](lu.md) under *Theory*.

## The SURE

The rank-1 Schur update over the strict trailing domain `k ≤ i-1, k ≤ j-1`; `L` and `U`
leave on the super-diagonal faces (the first row/column come from the seed).

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, 0 <= k < N, k <= i-1, k <= j-1) {
    a(i,j,k) = a(i,j,k-1) - ( a(i,k,k-1) / a(k,k,k-1) ) * a(k,j,k-1);   // Schur update
}
input  A                     : a(i,j,-1) = A[i][j];
output U  on  i-k = 1, i<=j  : U[i][j] = a(i,j,k);
output L  on  j-k = 1, j< i  : L[i][j] = a(i,j,k) / a(j,j,k);
```

Executable spec: `docs/SURE/lu.sure`. **Schedule:** a SARE with a benign forward affine
dependence, so the canonical linear **τ = [1,1,2]** is legal, as is the free schedule.
The wavefront sweeps the tetrahedral trailing domain.

<div class="schedule-anim" data-src="schedules/lu-free.json" data-height="440" data-fps="4"></div>

## Pivot propagation (a pure SURE)

For a **single** elimination step, the pivot broadcast can be *uniformized* into a
nearest-neighbour propagation (the pivot row and multiplier column come from the input
`A`): `pr` propagates the pivot row down `+i`, `mc` the multiplier column across `+j`,
and the rank-1 term is applied once via [`ger`](ger.md)'s
inject → add → drain — a **pure SURE** (`τ = [1,1,1]`).

```text
pr(i,j,k) = pr(i-1,j,k);   mc(i,j,k) = mc(i,j-1,k);                     // propagate pivot row / mult. col
r(i,j,k)  = r(i,j,k-1) - g*( mc(i,j-1,k)/pv(i-1,j,k) )*pr(i-1,j,k);     // one Schur step, once
```

Executable spec: `docs/SURE/lu_propagate.sure`. Its **free schedule shows the
propagation wavefront** — cell `(i,j)` fires at `t ≈ max(i,j)`, as the pivot and
multiplier hop across the array (a broadcast would fire all at once). The full
multi-step version needs the internal-confluence DSL extension (see
[the derivation](lu.md)).

<div class="schedule-anim" data-src="schedules/lu_propagate-free.json" data-height="440" data-fps="4"></div>
