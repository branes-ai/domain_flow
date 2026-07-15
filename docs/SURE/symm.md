# symm — symmetric matrix–matrix product

`symm` computes **`C := βC + αAB`** for a symmetric `A = Aᵀ` stored as a single
triangle. Numerically it is [`gemm`](gemm.md); the interest is the
**symmetric-storage confluence** — [`symv`](../../sure-algorithms/blas-l2/symv/)'s
two-face feed, now on gemm's 3-D reduction cube.

## The SURE

The operand `a` carrying `A(i,k)` reads the stored triangle **two ways**: the lower
cells (`k ≤ i`) read the stored `A[i][k]`, the upper cells (`i < k`) read the
**reflected** `A[k][i]`. So cell `(i,j,k)` always sees `A_sym(i,k)` and the rest is
the ordinary gemm sweep (`B` along `+i`, `α`/`β` projected, `βC` epilogue).

```text
system ((i,j,k) | 0 <= i < M, 0 <= j < N, 0 <= k < K) {
    a(i,j,k)     = a(i,j-1,k);                                          // A_sym(i,k) along +j
    b(i,j,k)     = b(i-1,j,k);                                          // B(k,j) along +i
    alpha(i,j,k) = alpha(i,j,k-1);  beta(i,j,k) = beta(i-1,j,k);  cin(i,j,k) = cin(i,j,k-1);
    c(i,j,k)     = c(i,j,k-1) + alpha(i,j,k-1)*a(i,j-1,k)*b(i-1,j,k);
}
// A feeds on TWO faces of one stored triangle:
input a(i,j,k) = A[i][k]   on  k <= i    // lower: the stored element
input a(i,j,k) = A[k][i]   on  i <  k    // upper: the reflected element
output C[i][j] = c(i,j,K-1) + beta*cin;
```

Executable spec: `docs/SURE/symm.sure`. For the reduction + epilogue see
[`gemm`](gemm.md); for the symmetric-storage confluence see
[`symv`](../../sure-algorithms/blas-l2/symv/).

## Schedule

Box domain, so the canonical **τ = [1,1,1]** is legal, as is the free schedule. The
wavefront sweeps the `(i,j,k)` cube exactly as `gemm`.

<div class="schedule-anim" data-src="schedules/symm-linear.json" data-height="480"></div>
