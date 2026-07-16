# syr — symmetric rank-1 update

`syr` computes **`A := A + αxxᵀ`** on the triangular domain `j ≤ i`. It is
[`ger`](ger.md) with `y = x` (a single vector on both axes) and the output restricted
to one triangle, since `A + αxxᵀ` is symmetric. It is the SPD-accumulation primitive
behind **Cholesky's trailing update**.

## The SURE

`ger`'s inject → add → drain (a depth-2 axis `k`, `α` injected once on the `k=-1`
halo) on `trmv`'s triangular domain. The single vector `x` drives both axes, fed per
cell on the `(i,j)` face (`x(i)` as `xx`, `x(j)` as `yy`).

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, j <= i, 0 <= k < 2) {
    alpha(i,j,k) = 0;                                                       // α only on the k=-1 halo
    xx(i,j,k)    = xx(i,j,k-1);                                             // x(i)
    yy(i,j,k)    = yy(i,j,k-1);                                             // x(j)
    r(i,j,k)     = r(i,j,k-1) + alpha(i,j,k-1)*xx(i,j,k-1)*yy(i,j,k-1);     // add αxxᵀ once, then drain
}
output Aout[i][j] = r(i,j,1);        // updated lower triangle
```

Executable spec: `docs/SURE/syr.sure`. For the feed/drain and triangular-domain
reasoning, see [deriving the matrix–vector operators](../matrix_vector_operators.md).

## Schedule

The result drains on the `k=1` face, so (unlike `trmv`) there is no `τ_j > τ_i`
constraint: the canonical **τ = [1,1,1]** is legal, as is the free schedule. The
wavefront sweeps the lower triangle.

<div class="schedule-anim" data-src="schedules/syr-linear.json" data-height="440"></div>

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `syr`'s four variables are the scalar `alpha`, the symmetric matrix
accumulator `r`, and the two reads `xx`, `yy` of the single vector `x` (the outer product
is `x xᵀ`):

| arc | θ | dependence |
| --- | --- | --- |
| `r → r` | `[0,0,1]ᵀ` | the rank-1 update applied once along the feed axis `k` |
| `alpha → r` | `[0,0,1]ᵀ` | α enters the product |
| `xx → r`, `yy → r` | `[0,0,1]ᵀ` | the two copies of `x` feed the outer product |
| `xx → xx`, `yy → yy` | `[0,0,1]ᵀ` | each copy held on the feed axis |

**Every** arc is the same feed-axis translation `[0,0,1]ᵀ` — `syr` is a fully parallel
operator whose whole rank-1 update inject → add → drains along the single depth axis `k`,
so `syr` is a genuine **SURE**.

<div class="rdg" data-src="rdg/syr.json" data-height="560"></div>
