# Triangular solve

`trsv` solves a triangular system **`L x = b`** (lower-triangular `L`) by **forward
substitution** — the substitution engine that finishes every direct solver once
[LU](lu_decomposition.md), [Cholesky](cholesky_decomposition.md), or
[LDLᵀ](ldlt_decomposition.md) has produced the triangular factors. For the derivation
— why the solve is a SARE and why its schedule is free-only — see
[**Triangular solve**](trsolve.md) under *Theory*.

## The SURE

`trsv` is [`trmv`](trmv.md) run backwards: the reduction accumulates
`Σ_{j<i} L(i,j) x_j`, but `x_j` is now a **solved** value read via an affine tap onto
the diagonal `(j,j)`, and the diagonal cell divides it out.

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, j <= i, 0 <= k < 1) {
    Lm(i,j,k)  = Lm(i,j,k-1);                                  // L(i,j) enters on the (i,j) face
    bb(i,j,k)  = bb(i,j,k-1);                                  // b(i)   enters on the (i,j) face
    acc(i,j,k) = acc(i,j-1,k) + Lm(i,j,k-1) * x(j,j,k);        // reduce L(i,j)*x_j over +j (x_j on the diagonal)
    x(i,j,k)   = ( bb(i,j,k-1) - acc(i,j-1,k) ) / Lm(i,j,k-1); // solve x_i = (b_i - sum_{j<i}) / L(i,i)
}
input  L                              : Lm(i,j,-1) = L[i][j];  // lower triangle of L, on k=-1
input  Bin                            : bb(i,j,-1) = Bin[i];   // right-hand side b, on k=-1
input  (seed)  on  j = -1             : acc(i,j,k) = 0;        // reduction seed
output Xout    on  i - j = 0          : Xout[i] = x(i,j,k);    // solution on the diagonal j = i
```

Executable spec: `docs/SURE/trsolve.sure`. **Schedule — free only.** Forward
substitution fuses the off-diagonal reduction and the divide at the *same* diagonal
lattice point, a zero-slack self-dependence (`τ·0 = 0` for every linear `τ`), so no
linear schedule is legal — the flux-legal `τ = [1,2,1]` is rejected by the legality
check, not by flux. The **free** schedule is the sequential substitution wavefront:
row `i` fires only after every `x_j`, `j < i`, is resolved. (This mirrors
[`trmm`](trmm.md), which is also free-only — but there the obstruction is flux.)

For the lower-triangular `L` and `b = [2, 11, 38, 100]`:
`x = [1, 2, 3, 4]` — `x₀=2/2`, `x₁=(11−3)/4`, `x₂=(38−5−12)/7`, `x₃=(100−8−18−30)/11`.

`trsm` (multiple RHS, `L X = B`) is this replicated across the columns of `B` — an
independent, fully parallel RHS axis; the schedule is unchanged.

<div class="schedule-anim" data-src="schedules/trsolve-free.json" data-height="440" data-fps="4"></div>
