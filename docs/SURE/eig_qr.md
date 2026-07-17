# Symmetric eigensolver — tridiagonalization + QR iteration

The QR-iteration eigensolver — the workhorse behind `dsyev` — finds the eigenvalues of a
symmetric `A` in two phases:

1. **Householder tridiagonalization** reduces `A` to a symmetric *tridiagonal* `T` by
   `N-2` two-sided reflections, each zeroing a column below its subdiagonal;
2. **implicit-shift QR iteration** drives `T` to diagonal by bulge-chasing Givens
   rotations, with a Wilkinson shift for cubic convergence.

Both phases are orthogonal similarities, so the spectrum is preserved throughout. This
page derives phase 1's building block — **one Householder step** — as the executable
spec, and explains why the sweep and phase 2 are scheduled compositions rather than a
single recurrence.

## One Householder step

To tridiagonalize column 0, reflect the subcolumn `x = A[1:,0]` onto `±‖x‖ e_1`, so
every entry below the subdiagonal is zeroed. With `H = I - β v vᵀ`:

$$
\alpha = \lVert A[1{:},0] \rVert, \quad
v = \begin{bmatrix} 0 \\ A_{10} + \operatorname{sign}(A_{10})\,\alpha \\ A_{20} \\ \vdots \end{bmatrix}, \quad
\beta = \frac{2}{v^{\mathsf T} v},
$$

and the symmetric two-sided update `A' = HAH` is the rank-2 form

$$
w = \beta A v - \tfrac{\beta^2 (v^{\mathsf T}\! A v)}{2}\,v,
\qquad
A' = A - v\,w^{\mathsf T} - w\,v^{\mathsf T}.
$$

It zeros `A'[2:,0]`, leaves `A'[1][0] = -\operatorname{sign}(A_{10})\,\alpha` on the
subdiagonal, and (being an orthogonal similarity) preserves the eigenvalues.

## The SURE — three reductions and a rank-2 update

The step is a small domain-flow graph, structurally close to [CG](cg.md): **three
reductions over rows** — `‖A[1:,0]‖²`, `vᵀv`, and `vᵀ(Av)` — and **one over columns** —
`Av` — with the scalars `α`, `β`, `vᵀAv` broadcast back. The "row ≥ 1" and "row == 1"
masks are **fed** (the body cannot read the loop index); the pivot column `A[i][0]` and
the vectors `v`, `w` enter as **fixed-index affine taps**, so it is a **SARE**.

```text
nrm(i,j)  = nrm(i-1,j) + sub(i)*am(i,0)^2;                 // ||A[1:,0]||^2, reduce over i
v(i,j)    = sub(i)*am(i,0) + e1(i)*sgn*sqrt(nrm(N-1));     // Householder vector (fed masks)
av(i,j)   = av(i,j-1) + am(i,j)*v(j);                      // A v, reduce over j
vav(i,j)  = vav(i-1,j) + v(i)*av(i,N-1);                   // v^T A v, reduce over i
w(i,j)    = beta*av(i,N-1) - (beta^2*vav/2)*v(i);          // reflector's w
arot(i,j) = am(i,j) - v(i)*w(j) - w(i)*v(j);               // A' = A - v w^T - w v^T
```

Executable spec: `docs/SURE/eig_qr.sure`. For a symmetric `4×4` `A`, one step zeros
`A'[2][0]=A'[3][0]=0` and leaves `A'[1][0]=-√6`; `test_eig_qr_sure` checks `A' = HAH` to
machine precision (trace `18`, Frobenius² `110` invariant).

## Why the full algorithm is not one SURE

The **reduction sweep** repeats this step for column `p = 0,1,…,N-2`; each step reads the
pivot column `A[:,p]` at a **data-dependent** index `p`, which the confluence DSL cannot
address (the same obstruction as [partial pivoting](lu.md) and the
[Jacobi eigensolver](eig_jacobi.md)). **Phase 2** (implicit-shift QR) is worse still: it
*iterates*, and each sweep computes a shift from the trailing `2×2` of the current `T` and
chases a bulge — a data-dependent, convergence-controlled loop. So the executable artifact
is the single Householder step; the tridiagonalization sweep and the QR iteration are
scheduled compositions of these local orthogonal updates.

## Contrast with Jacobi

The [Jacobi eigensolver](eig_jacobi.md) applies *many* small two-sided rotations directly
to the dense `A`, converging linearly but with abundant parallelism (disjoint pivot pairs
rotate independently). QR iteration first *concentrates* the work into a tridiagonal `T`
(a finite, `O(N³)` reduction) and then iterates cheaply on `T` with cubic convergence —
fewer flops overall, but a longer serial dependence chain (the bulge chase). Same
spectrum, opposite ends of the parallelism/convergence trade — visible here as the
Householder step's wide rank-2 update versus Jacobi's local `2×2`.

## The RDG

Thirteen variables — the held `A`, the two fed masks, the three reduction accumulators
(`nrm`, `vv`, `vav`), the `Av` accumulator, the scalars (`alpha`, `sgn`, `bet`), the
reflector `v` and `w`, and the update `arot` — wired by fifteen **affine** arcs: the
fixed-index reads of column 0, the vector gathers, and the scalar broadcasts.

<div class="rdg" data-src="rdg/eig_qr.json" data-height="700"></div>

<div class="schedule-anim" data-src="schedules/eig_qr-free.json" data-height="380"></div>
