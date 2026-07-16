# syr2k — symmetric rank-2k update

`syr2k` computes **`C := βC + α(ABᵀ + BAᵀ)`** on the triangular output `j ≤ i` (the
sum is symmetric). It is [`syrk`](syrk.md) with **two** operands — two fused
gemm-like reductions — and appears in blocked eigen/SVD trailing updates.

## The SURE

Four operand streams feed the two-term product: `A(i,k)`/`B(i,k)` propagate along
`+j`, and `A(j,k)`/`B(j,k)` along `+i` (on the **super-diagonal** halo `i-j = -1`, as
in [`syrk`](syrk.md), since the triangle's columns begin at the diagonal). `α`/`β`/`C_in`
pipeline along `+k` as in `gemm`.

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, j <= i, 0 <= k < K) {
    ai(i,j,k) = ai(i,j-1,k);   bi(i,j,k) = bi(i,j-1,k);                 // A(i,k), B(i,k) along +j
    aj(i,j,k) = aj(i-1,j,k);   bj(i,j,k) = bj(i-1,j,k);                 // A(j,k), B(j,k) along +i (super-diag)
    alpha(i,j,k) = alpha(i,j,k-1);  beta(i,j,k) = beta(i,j,k-1);  cin(i,j,k) = cin(i,j,k-1);
    c(i,j,k)  = c(i,j,k-1) + alpha(i,j,k-1)*(ai(i,j-1,k)*bj(i-1,j,k) + bi(i,j-1,k)*aj(i-1,j,k));
}
output C[i][j] = c(i,j,K-1) + beta*cin;                                // α(ABᵀ+BAᵀ) + βC (lower)
```

Executable spec: `docs/SURE/syr2k.sure`. For the single-operand case and the
super-diagonal feed see [`syrk`](syrk.md); for the reduction + epilogue see
[`gemm`](gemm.md).

## Schedule

As in [`syrk`](syrk.md), the super-diagonal feeds have outward normal `(-1,1,0)`, so
the canonical **τ = [2,1,1]** (`τ_i > τ_j`) is legal and the box-default `τ = [1,1,1]`
is *rejected*. The free schedule is also legal; the wavefront sweeps the triangular
prism.

<div class="schedule-anim" data-src="schedules/syr2k-linear.json" data-height="480"></div>

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `syr2k` (`C := α(ABᵀ + BAᵀ) + βC`) reads `A` and `B` each on two faces
(`ai`,`aj` and `bi`,`bj`) to form the two products; all eight variables wire with
constant offsets (grouped below; thirteen arcs in all):

| arc(s) | θ | dependence |
| --- | --- | --- |
| `c → c` | `[0,0,1]ᵀ` | the reduction chains along `+k` |
| `ai → c`, `bi → c` | `[0,1,0]ᵀ` | the `+j` operands feed the two products |
| `bj → c`, `aj → c` | `[1,0,0]ᵀ` | the `+i` operands feed the two products |
| `alpha → c` | `[0,0,1]ᵀ` | α enters the products |
| `ai → ai`, `bi → bi` | `[0,1,0]ᵀ` | the `+j` operands pipelined |
| `aj → aj`, `bj → bj` | `[1,0,0]ᵀ` | the `+i` operands pipelined |
| `alpha → alpha`, `beta → beta`, `cin → cin` | `[0,0,1]ᵀ` | scalars / incoming `C` on the feed face |

Every arc is a translation vector, so `syr2k` is a genuine **SURE** — two `gemm`-shaped
reduction cubes sharing the accumulator `c`.

<div class="rdg" data-src="rdg/syr2k.json" data-height="660"></div>
