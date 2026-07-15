# trmm — triangular matrix–matrix product

`trmm` computes **`B := αTB`** in place, for a (lower) triangular `T`. It is
[`gemm`](gemm.md) whose reduction over `k` has a **triangular extent** `k ≤ i` — a
non-box 3-D iteration domain — so it exercises triangular face pinning at Level-3
scale.

## The SURE

`T(i,k)` propagates along `+j` (reused across output columns); `B(k,j)` along `+i`
(reused across rows), entering on the **super-diagonal** halo `i-k = -1` since the
triangle's rows begin at the diagonal. The reduction for row `i` completes at `k = i`,
so the result leaves on the **diagonal** face `i-k = 0`.

```text
system ((i,j,k) | 0 <= i < M, 0 <= j < N, 0 <= k < M, k <= i) {
    a(i,j,k)     = a(i,j-1,k);                                          // T(i,k) along +j
    b(i,j,k)     = b(i-1,j,k);                                          // B(k,j) along +i (super-diagonal seed)
    alpha(i,j,k) = alpha(i,j,k-1);
    acc(i,j,k)   = acc(i,j,k-1) + alpha(i,j,k-1)*a(i,j-1,k)*b(i-1,j,k); // Σ_{k<=i} T(i,k)B(k,j)
}
output Bout[i][j] = acc(i,j,i);                                         // result on the diagonal k=i
```

Executable spec: `docs/SURE/trmm.sure`. For the reduction skeleton see
[`gemm`](gemm.md); for the triangular-domain feed see [`syrk`](syrk.md).

## Schedule — free only

`trmm` is **free-schedule-only**. The super-diagonal `B`-feed (`i-k = -1`) and the
diagonal output (`i-k = 0`) are **parallel** faces with the same outward normal
`(-1,0,1)` but opposite flux: the feed needs `τ_k < τ_i` (influx) while the output
needs `τ_k > τ_i` (outflux). **No single linear `τ` satisfies both**, so there is no
legal linear wavefront — the data-flow-earliest **free** schedule is required, as for
the Modified-Gram-Schmidt [QR](QR_decomposition.md). This is the triangular *in-place* geometry
obstructing a global wavefront (see [uniformization](../scaling/uniformization.md));
the animation below shows the free schedule sweeping the triangular prism.

<div class="schedule-anim" data-src="schedules/trmm-free.json" data-height="480"></div>
