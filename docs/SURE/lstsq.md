# Least squares via QR — the first pipeline

Overdetermined least squares — `min_x ‖Ax − b‖₂` for a tall `A` (`M ≥ N`) — is the
first entry in the catalog built by **composing** operators. The classic recipe is
`A = QR`, then solve `R x = Qᵀb`. The point of this page is that the three stages —
**QR → apply `Qᵀ` → back-substitution** — *chain as domain flows without materializing
the intermediates*, and that the first two collapse into a single pass.

## Never form Q — augment b

Least squares needs `Qᵀb`, never `Q` itself. So instead of computing `Q` and then a
`gemv`/`rot` to apply it, **augment `b` as an extra column of `A`** and run the
[Givens QR](QR_decomposition.md) on `[A | b]`. The rotations chosen to zero `A`'s sub-diagonal are
applied to *every* column to their right — including the augmented one — so the same
sweep that produces `R` transforms `b` into `Qᵀb`:

$$
G\,[\,A \mid b\,] \;=\; [\,R \mid Q^{\mathsf T} b\,].
$$

One QR pass yields both `R` (the `N×N` upper-triangular factor) and `c = Qᵀb` (its first
`N` components). The "QR factorization" and "apply `Qᵀ`" stages are **fused into one
domain flow** — no explicit `Q`, no intermediate matrix.

## The SURE

The Givens QR's index space `(i, p, q)` extends by one column: `i` is
the row of `A` being merged (`0..M-1`), `p` the pivot (`0..N-1`), and `q` the column
updated — now `0..N`, where `q = N` is the augmented `b` column. `r(i,p,q)` is `R[p][q]`
for `q < N`, and `c[p] = (Qᵀb)[p]` for `q = N`:

```text
system ((i,p,q) | 0 <= i < M, 0 <= p < N, 0 <= q < N+1, p <= q) {
    r(i,p,q) = (r(i-1,p,p)*r(i-1,p,q) + a(i,p-1,p)*a(i,p-1,q)) / sqrt(...);   // c*R + s*a
    a(i,p,q) = (r(i-1,p,p)*a(i,p-1,q) - a(i,p-1,p)*r(i-1,p,q)) / sqrt(...);   // -s*R + c*a
}
input  A on p=-1, q<N     : a(i,p,q) = A[i][q];   // rows of A stream in
input  B on p=-1, q=N     : a(i,p,q) = B[i];      // b augmented as column N
output R on i=M-1, q<N     : R[p][q] = r(i,p,q);   // the R factor
output C on i=M-1, q=N     : C[p]    = r(i,p,q);   // c = Qᵀb
```

Executable spec: `docs/SURE/lstsq.sure`. Like `qr_givens` this is a **SARE** — the
diagonal taps `r(i-1,p,p)`, `a(i,p-1,p)` are affine (the rotation broadcast along the
row). But the dependence is *benign and forward* (the diagonal `q = p` is resolved
before the cells `q ≥ p` that read it), so the canonical linear schedule **`τ = [1,1,1]`**
is legal, as is the free schedule.

## The third stage — back-substitution

`R x = c` is an upper-triangular solve — the mirror of the forward
[triangular solve](trsolve.md): sweep `p` from `N-1` down to `0`,
`x(p) = (c(p) − Σ_{q>p} R[p][q]·x(q)) / R[p][p]`. It composes onto the QR output face:
`c` and `R` leave the QR on `i = M-1` and feed straight into the solve — the pipeline
chains face-to-face. (The reference `test_lstsq_sure` performs this stage to recover `x`.)

## Verifying without Q

Because `Q` is never formed, the whole pipeline is checked by **sign-robust invariants**
that never mention it:

$$
R^{\mathsf T} R = A^{\mathsf T} A,
\qquad
R^{\mathsf T} c = A^{\mathsf T} b,
$$

the second because `Rᵀc = Rᵀ Qᵀ b = (QR)ᵀ b = Aᵀb`. Together they give
`RᵀR\,x = Rᵀc`, i.e. `AᵀA\,x = Aᵀb` — the **normal equations**, so the back-substituted
`x` is the least-squares solution. For the bundled tall Vandermonde `A` and `b =
[2,3,5,9]`, `x = [2.75, −1.45, 0.75]` satisfies `AᵀA x = Aᵀb` exactly.

## The RDG

The reduced dependency graph is `qr_givens`'s, one column wider: two variables (`a`, the
working row, and `r`, carrying `R` and `c`) with the affine rotation-broadcast arcs that
mark it a SARE — the same diagonal projections as the [QR page](reduced_dependency_graph.md),
now also carrying `Qᵀb`.

<div class="rdg" data-src="rdg/lstsq.json" data-height="560"></div>

<div class="schedule-anim" data-src="schedules/lstsq-linear.json" data-height="380"></div>
