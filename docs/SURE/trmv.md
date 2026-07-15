# trmv — triangular matrix–vector product

`trmv` computes **`x := Tx`** for a triangular `T` (here lower). It is [`gemv`](gemv.md)
on a **triangular domain** `j ≤ i` — the catalog's first non-box index space.

## The SURE

Each row's reduction runs over the variable extent `j = 0..i`, so the result leaves
on the **diagonal face** `i-j = 0`. `T` and the input vector both feed on the `(i,j)`
face of a depth-1 axis `k` (feeding `x` here, rather than pipelining it along `+i`,
avoids a diagonal seed face on the triangular domain).

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, j <= i, 0 <= k < 1) {
    t(i,j,k)   = t(i,j,k-1);                                // T(i,j)
    xx(i,j,k)  = xx(i,j,k-1);                               // x(j)
    acc(i,j,k) = acc(i,j-1,k) + t(i,j,k-1)*xx(i,j,k-1);     // reduce over +j (j = 0..i)
}
output Xout[i] = acc(i,i,0);                                // result on the diagonal j = i
```

Executable spec: `docs/SURE/trmv.sure`. For the triangular-domain operand entry and
the diagonal-output schedule constraint, see
[deriving the matrix–vector operators](../matrix_vector_operators.md).

## Schedule

The diagonal output has outward normal `(-1,1,0)`, so its outflux needs **`τ_j > τ_i`**:
the canonical **τ = [1,2,1]** is legal, while the box-default `τ = [1,1,1]` is
*rejected* (zero diagonal flux). The free schedule is also legal. The wavefront sweeps
the lower triangle, each row completing on the diagonal.

<div class="schedule-anim" data-src="schedules/trmv-linear.json" data-height="440"></div>
