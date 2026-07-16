# gemm — general matrix–matrix product

`gemm` computes **`C := βC + αAB`** — the compute-bound heart of dense linear
algebra (`O(n³)` work on `O(n²)` data, so each element is reused `O(n)` times). It
generalizes the canonical systolic [matmul](matmul.md) (`C = AB`) with `α`, `β`,
and an incoming `C`.

## The SURE

The 3-D reduction cube `(i,j,k)`: the sum runs over `k`, `A(i,k)` propagates along
`+j`, and `B(k,j)` along `+i` (the classic systolic flows). The generalization reuses
the catalog's matrix–vector patterns — `α` is projected onto a face and folded into
the reduction, and `βC_in` joins the completed sum in a terminal-face epilogue on the
output face `k = K-1`.

```text
system ((i,j,k) | 0 <= i < M, 0 <= j < N, 0 <= k < K) {
    a(i,j,k)     = a(i,j-1,k);                                          // A(i,k) along +j
    b(i,j,k)     = b(i-1,j,k);                                          // B(k,j) along +i
    alpha(i,j,k) = alpha(i,j,k-1);                                      // scalar α
    beta(i,j,k)  = beta(i-1,j,k);                                       // scalar β
    cin(i,j,k)   = cin(i,j,k-1);                                        // incoming C(i,j)
    c(i,j,k)     = c(i,j,k-1) + alpha(i,j,k-1)*a(i,j-1,k)*b(i-1,j,k);   // accumulate αAB over +k
}
output C[i][j] = c(i,j,K-1) + beta*cin;                                // αAB + βC epilogue
```

Executable spec: `docs/SURE/gemm.sure`. For the reduction skeleton see
[Matmul and Tensormul](matmul.md); for the `α`/`β` project-and-pipeline and the
epilogue see [deriving the matrix–vector operators](../matrix_vector_operators.md).

## Schedule

Canonical linear schedule **τ = [1,1,1]**; also legal under the free schedule. The
wavefront sweeps the `(i,j,k)` cube diagonally — `k` is the reduction depth, `i`/`j`
the output plane. This is the richest wavefront in the catalog and the archetype for
tiling (see [tiling the index space](../scaling/tiling.md)).

<div class="schedule-anim" data-src="schedules/gemm-linear.json" data-height="480"></div>

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `gemm`'s six variables wire the full 3-D reduction cube with constant
offsets only:

| arc | θ | dependence |
| --- | --- | --- |
| `c → c` | `[0,0,1]ᵀ` | the reduction chains along the contraction axis `+k` |
| `a → c` | `[0,1,0]ᵀ` | `A(i,k)` enters the product |
| `b → c` | `[1,0,0]ᵀ` | `B(k,j)` enters the product |
| `alpha → c` | `[0,0,1]ᵀ` | α enters the product |
| `a → a` | `[0,1,0]ᵀ` | `A` pipelined (reused) along `+j` |
| `b → b` | `[1,0,0]ᵀ` | `B` pipelined (reused) along `+i` |
| `alpha → alpha` | `[0,0,1]ᵀ` | α projected on the feed face |
| `beta → beta` | `[1,0,0]ᵀ` | β projected on its face |
| `cin → cin` | `[0,0,1]ᵀ` | the incoming `C` held for the `βC` epilogue |

Every arc is a translation vector, so `gemm` is a genuine **SURE** — the canonical
3-D reduction cube, schedulable by a single linear `τ`.

<div class="rdg" data-src="rdg/gemm.json" data-height="620"></div>
