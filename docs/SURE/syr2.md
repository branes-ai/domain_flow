# syr2 — symmetric rank-2 update

`syr2` computes **`A := A + α(xyᵀ + yxᵀ)`** on the triangular domain `j ≤ i`. It is
[`syr`](syr.md) with two vectors — the two rank-1 outer products fused into one
symmetric update — the kernel behind `syr2k` and the LDLᵀ / eigensolver trailing
updates.

## The SURE

The same structure as `syr` (a depth-2 feed/drain axis over the triangle, `α`
injected once), but the rank-2 combination needs **four** per-cell feeds — `x(i)`,
`x(j)`, `y(i)`, `y(j)` — from the two input vectors, and forms `α(x(i)y(j) +
y(i)x(j))` from uniform taps.

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, j <= i, 0 <= k < 2) {
    alpha(i,j,k) = 0;                                                       // α only on the k=-1 halo
    xi(i,j,k) = xi(i,j,k-1);   xj(i,j,k) = xj(i,j,k-1);                     // x(i), x(j)
    yi(i,j,k) = yi(i,j,k-1);   yj(i,j,k) = yj(i,j,k-1);                     // y(i), y(j)
    r(i,j,k)  = r(i,j,k-1) + alpha(i,j,k-1)*(xi*yj + yi*xj);                // add α(xyᵀ+yxᵀ) once, then drain
}
output Aout[i][j] = r(i,j,1);        // updated lower triangle
```

Executable spec: `docs/SURE/syr2.sure`. For the shared structure with `syr`, see
[deriving the matrix–vector operators](../matrix_vector_operators.md).

## Schedule

As with `syr`, the result drains on the `k=1` face, so the canonical **τ = [1,1,1]**
is legal, as is the free schedule. The wavefront sweeps the lower triangle.

<div class="schedule-anim" data-src="schedules/syr2-linear.json" data-height="440"></div>
