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

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `syr2`'s six variables are the scalar `alpha`, the symmetric accumulator
`r`, and the two face-reads each of `x` (`xi`, `xj`) and `y` (`yi`, `yj`) — the two outer
products of `A += α(x yᵀ + y xᵀ)`:

| arc | θ | dependence |
| --- | --- | --- |
| `r → r` | `[0,0,1]ᵀ` | the rank-2 update applied once along the feed axis `k` |
| `alpha → r` | `[0,0,1]ᵀ` | α enters the products |
| `xi → r`, `yj → r` | `[0,0,1]ᵀ` | the `x yᵀ` outer product |
| `yi → r`, `xj → r` | `[0,0,1]ᵀ` | the `y xᵀ` outer product |
| `xi → xi`, `xj → xj`, `yi → yi`, `yj → yj` | `[0,0,1]ᵀ` | each face-read held on the feed axis |

**Every** arc is the same feed-axis translation `[0,0,1]ᵀ`: like `syr`, `syr2` is a fully
parallel operator whose rank-2 update inject → add → drains along the single depth axis
`k` — a genuine **SURE**.

<div class="rdg" data-src="rdg/syr2.json" data-height="620"></div>
