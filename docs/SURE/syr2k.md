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
