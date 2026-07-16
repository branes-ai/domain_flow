# ger — general rank-1 update

`ger` computes **`A := A + αxyᵀ`** — a rank-1 outer product added to a matrix. It is
the outer-product counterpart to [`gemv`](gemv.md) (expand two vectors into a matrix
rather than contract a matrix into a vector), and the building block reused by LU's
Schur update and by `syr`/`syr2`.

## The SURE

A **fully-parallel** update — no reduction. `x` broadcasts along `+j`, `y` along `+i`.
`A` is twice-indexed and both enters and exits, so it uses a **depth-2 feed/drain**
axis `k`: `A` seeds the accumulator on `k=-1`, the rank-1 term is added at `k=0`, and
the updated matrix drains on the `k=1` face. `α` is injected only on the `k=-1` halo,
so the term is added **exactly once**.

```text
system ((i,j,k) | 0 <= i < M, 0 <= j < N, 0 <= k < 2) {
    alpha(i,j,k) = 0;                                                       // α only on the k=-1 halo
    xx(i,j,k)    = xx(i,j-1,k);                                             // x(i) broadcast +j
    yy(i,j,k)    = yy(i-1,j,k);                                             // y(j) broadcast +i
    r(i,j,k)     = r(i,j,k-1) + alpha(i,j,k-1)*xx(i,j-1,k)*yy(i-1,j,k);     // add αxyᵀ once, then drain
}
output Aout[i][j] = r(i,j,1);
```

Executable spec: `docs/SURE/ger.sure`. For the depth-2 feed/drain and one-shot
injection, see [deriving the matrix–vector operators](../matrix_vector_operators.md).

## Schedule

Canonical linear schedule **τ = [1,1,1]**; also legal free. The wavefront sweeps the
`(i,j)` plane; the two `k` layers are the add (`k=0`) and drain (`k=1`).

<div class="schedule-anim" data-src="schedules/ger-linear.json" data-height="440"></div>

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `ger`'s four variables — the scalar `alpha`, the matrix accumulator `r`,
and the two operands `xx`, `yy` — connect with constant offsets only:

| arc | θ | dependence |
| --- | --- | --- |
| `r → r` | `[0,0,1]ᵀ` | the rank-1 update applied once along the feed axis `k` |
| `alpha → r` | `[0,0,1]ᵀ` | α enters the outer product |
| `xx → r` | `[0,1,0]ᵀ` | `x` feeds the outer product |
| `yy → r` | `[1,0,0]ᵀ` | `y` feeds the outer product |
| `xx → xx` | `[0,1,0]ᵀ` | `x` pipelined along `+j` |
| `yy → yy` | `[1,0,0]ᵀ` | `y` pipelined along `+i` |

Every arc is a translation vector, so `ger` is a genuine **SURE**: the outer product
`A += α x yᵀ` injects once on the `k = -1` halo and drains along the feed axis, all by
nearest-neighbour hops.

<div class="rdg" data-src="rdg/ger.json" data-height="560"></div>
