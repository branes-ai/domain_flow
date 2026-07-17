# Singular Value Decomposition

The SVD `A = UΣVᵀ` factors any `A` into orthogonal `U`, `V` and diagonal singular values
`Σ`. Two routes reach it; the catalog derives both and takes the SURE-native one as the
executable spec.

- **(a) One-sided Jacobi** — sweep column *pairs*, rotating each pair to orthogonality.
  When the columns are mutually orthogonal, their norms are the singular values, their
  directions `U`, and the accumulated rotations `V`. Rotation-native, the most systolic
  formulation — this page's spec.
- **(b) Golub–Kahan** — Householder **bidiagonalization** (two-sided reflectors reduce
  `A` to bidiagonal `B`), then implicit-shift **QR iteration** on `B`. Fewer flops for
  large `A`, but a longer serial dependence chain (as in the [QR eigensolver](eig_qr.md)).

## One-sided Jacobi — one column rotation

To orthogonalize columns `a₀, a₁`, form their `2×2` Gram matrix and rotate it to diagonal
— exactly the [symmetric-eigenvalue angle](eig_jacobi.md), now on the Gram entries:

$$
\alpha = a_0^{\mathsf T} a_0,\quad
\beta  = a_1^{\mathsf T} a_1,\quad
\gamma = a_0^{\mathsf T} a_1,
\qquad
\zeta = \frac{\beta - \alpha}{2\gamma},\quad
t = \frac{\operatorname{sign}\zeta}{|\zeta| + \sqrt{\zeta^2+1}},\quad
c = \tfrac{1}{\sqrt{t^2+1}},\ s = tc.
$$

The update is **one-sided** — right-multiply by `J`, so only the two columns change:

$$
a_0' = c\,a_0 - s\,a_1, \qquad a_1' = s\,a_0 + c\,a_1, \qquad a_0'^{\mathsf T} a_1' = 0.
$$

Right-multiplying by an orthogonal `J` leaves the singular values invariant, so one
rotation makes columns `0,1` orthogonal while preserving `Σ`.

## The SURE

Three **dot reductions over rows** give the Gram entries `α, β, γ`; the scalar angle
`c, s` broadcasts back; and the update rotates columns `0,1`. The "is this column `p` or
`q`?" test reads **fed indicators**, and the pivot columns enter as **fixed-index affine
taps** `a0(i,0)`, `a0(i,1)` — so it is a **SARE** (simpler than the two-sided
[eigenvalue rotation](eig_jacobi.md): no left-multiply).

```text
alpha(i,j) = alpha(i-1,j) + a0(i,0)^2;                    // a0.a0, reduce over rows
gamma(i,j) = gamma(i-1,j) + a0(i,0)*a0(i,1);              // a0.a1
zeta       = (beta(M-1) - alpha(M-1)) / (2 gamma(M-1));   // Gram-diagonalizing angle
arot(i,j)  = select(cp, cs*a0(i,0) - sn*a0(i,1),          // one-sided: rotate columns p,q
             select(cq, sn*a0(i,0) + cs*a0(i,1), a0(i,j)));
```

Executable spec: `docs/SURE/svd.sure`. For a tall `4×3` `A` whose columns `0,1` have inner
product `4`, one rotation drives `a₀'·a₁'` to zero and leaves the Frobenius norm (hence
`Σ`) invariant — `test_svd_sure` checks `A' = AJ` to machine precision.

## Why neither full route is one SURE

The Jacobi **sweep** cycles the pair `(p,q)` over all columns and repeats to convergence;
each rotation reads columns `p,q` at **data-dependent** indices, which the DSL cannot
address (the [partial-pivoting](lu.md) obstruction, shared with both eigensolvers). The
Golub–Kahan route inherits the [Householder](eig_qr.md) reduction sweep and an iterative
QR phase. So the executable artifact is the single column rotation; the full SVD is a
scheduled composition of these local orthogonal updates — a fixed program of rotations
streamed over the array.

## The RDG

The Gram reductions (`alpha`, `beta`, `gamma`), the angle chain (`zeta → t → cs, sn`), the
two fed indicators, and the one-sided update `arot`, wired by the **affine** taps that read
the pivot columns and broadcast the angle — nine of them, marking it a SARE.

<div class="rdg" data-src="rdg/svd.json" data-height="640"></div>

<div class="schedule-anim" data-src="schedules/svd-free.json" data-height="360"></div>
