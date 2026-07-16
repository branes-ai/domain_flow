# syrk — symmetric rank-k update

`syrk` computes **`C := βC + αAAᵀ`** — a symmetric rank-k update (the `AAᵀ` term is
positive *semi*definite), the kernel behind blocked Cholesky's trailing update. It is
[`gemm`](gemm.md) with `B = Aᵀ` (a single operand `A` feeding both taps) on a
**triangular output** `j ≤ i`, since `AAᵀ` is symmetric.

## The SURE

The 3-D reduction runs over `k`. As in `gemm`, `a` carries `A(i,k)` along `+j`; the
second tap is `A(j,k)`, so `b` carries `A(j,k)` along `+i`. Because the output
triangle's columns begin at the diagonal, `b` cannot enter on the plain `i=-1` face —
it enters on the **super-diagonal** halo `i-j = -1` (a clean halo here, since the
output leaves on `k=K-1`, not on a diagonal parallel to that feed). `α`/`β`/`C_in` are
pipelined as in `gemm`.

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, j <= i, 0 <= k < K) {
    a(i,j,k)     = a(i,j-1,k);                                          // A(i,k) along +j
    b(i,j,k)     = b(i-1,j,k);                                          // A(j,k) along +i (super-diagonal seed)
    alpha(i,j,k) = alpha(i,j,k-1);  beta(i,j,k) = beta(i,j,k-1);  cin(i,j,k) = cin(i,j,k-1);
    c(i,j,k)     = c(i,j,k-1) + alpha(i,j,k-1)*a(i,j-1,k)*b(i-1,j,k);   // Σ_k A(i,k)A(j,k)
}
output C[i][j] = c(i,j,K-1) + beta*cin;                                // αAAᵀ + βC (lower triangle)
```

Executable spec: `docs/SURE/syrk.sure`. For the reduction + epilogue see
[`gemm`](gemm.md); for the triangular-domain feed and schedule constraint see
[`trmv`](trmv.md) and [deriving the matrix–vector operators](../matrix_vector_operators.md).

## Schedule

The super-diagonal `b`-face has outward normal `(-1,1,0)`, so its influx needs
**`τ_i > τ_j`**: the canonical **τ = [2,1,1]** is legal, while the box-default
`τ = [1,1,1]` is *rejected*. This mirrors [`trmv`](trmv.md)'s diagonal constraint in
the opposite direction — the triangular geometry again shapes the schedule. The free
schedule is also legal; the wavefront sweeps the triangular prism.

<div class="schedule-anim" data-src="schedules/syrk-linear.json" data-height="480"></div>

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `syrk` (`C := αAAᵀ + βC`) has the same six-node reduction-cube structure as
[`gemm`](gemm.md) — the symmetry lives in *reading `A` twice* and updating one triangle,
not in the dependence graph:

| arc | θ | dependence |
| --- | --- | --- |
| `c → c` | `[0,0,1]ᵀ` | the reduction chains along `+k` |
| `a → c` | `[0,1,0]ᵀ` | `A(i,k)` enters the product |
| `b → c` | `[1,0,0]ᵀ` | the reflected `A(j,k)` enters the product |
| `alpha → c` | `[0,0,1]ᵀ` | α enters the product |
| `a → a` | `[0,1,0]ᵀ` | `A` pipelined along `+j` |
| `b → b` | `[1,0,0]ᵀ` | the reflected `A` pipelined along `+i` |
| `alpha → alpha`, `beta → beta`, `cin → cin` | `[0,0,1]ᵀ` | scalars / incoming `C` on the feed face |

Every arc is a translation vector, so `syrk` is a genuine **SURE**.

<div class="rdg" data-src="rdg/syrk.json" data-height="620"></div>
