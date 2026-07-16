# gemv — general matrix–vector product

`gemv` computes **`y := αAx + βy`** — a matrix reduced against a vector, scaled and
accumulated into `y`. It is the workhorse of Level-2 BLAS.

## The SURE

A reduction over the columns `j` (rows `i` are the independent, parallel direction).
`A` feeds on the `(i,j)` face of a depth-1 axis `k`; `x` is reused across rows
(pipelined `+i`); the scalars `α`, `β` are projected onto faces; and the `βy` term
joins the completed sum in a terminal-face epilogue.

```text
system ((i,j,k) | 0 <= i < M, 0 <= j < N, 0 <= k < 1) {
    a(i,j,k)     = a(i,j,k-1);                                              // A(i,j)
    xx(i,j,k)    = xx(i-1,j,k);                                             // x(j), reused across rows
    alpha(i,j,k) = alpha(i,j,k-1);
    beta(i,j,k)  = beta(i-1,j,k);
    yin(i,j,k)   = yin(i,j-1,k);
    acc(i,j,k)   = acc(i,j-1,k) + alpha(i,j,k-1)*a(i,j,k-1)*xx(i-1,j,k);    // reduce αAx over +j
}
output Y[i] = acc(i,N-1,0) + beta*yin;                                      // αAx + βy epilogue
```

Executable spec: `docs/SURE/gemv.sure`. For the confluence reasoning — the feed axis,
project-and-pipeline, the epilogue — see
[deriving the matrix–vector operators](../matrix_vector_operators.md).

## Schedule

Canonical linear schedule **τ = [1,1,1]**; also legal under the free
(data-flow-earliest) schedule. The wavefront sweeps the `(i,j)` plane, each row's
reduction advancing along `+j`.

<div class="schedule-anim" data-src="schedules/gemv-linear.json" data-height="440"></div>

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `gemv`'s six variables wire up with constant offsets only:

| arc | θ | dependence |
| --- | --- | --- |
| `acc → acc` | `[0,1,0]ᵀ` | the reduction chains along `+j` |
| `a → acc` | `[0,0,1]ᵀ` | `A(i,j)` (fed on the depth-1 axis `k`) enters the product |
| `alpha → acc` | `[0,0,1]ᵀ` | α enters the product |
| `xx → acc` | `[1,0,0]ᵀ` | `x` enters the product |
| `a → a` | `[0,0,1]ᵀ` | `A` held on the feed axis |
| `xx → xx` | `[1,0,0]ᵀ` | `x` pipelined (reused) across rows `+i` |
| `alpha → alpha` | `[0,0,1]ᵀ` | α projected on the feed face |
| `beta → beta` | `[1,0,0]ᵀ` | β projected on the `i = -1` face |
| `yin → yin` | `[0,1,0]ᵀ` | the incoming `y` streamed along `+j` |

Every arc is a translation vector, so `gemv` is a genuine **SURE** — this is the
all-uniform worked example on the [RDG page](reduced_dependency_graph.md).

<div class="rdg" data-src="rdg/gemv.json" data-height="620"></div>
