# scal as a System of Uniform Recurrence Equations

The BLAS Level-1 `scal` scales a vector in place by a scalar:

$$
x_i \;\leftarrow\; \alpha\,x_i, \qquad 0 \le i < N.
$$

It is the minimal scaled map, and the cleanest illustration of a compositional
fact: **`scal` is [`axpy`](axpy.md) with the addend set to the additive
identity.** Writing $\alpha x = \alpha x + 0$ makes it literally the `axpy`
recurrence with the incoming vector $y \equiv 0$. Everything `axpy` established —
the projected-and-pipelined scalar, the one-shot operand injection, the two-cell
systolic map — carries over unchanged; only the seed of the result stream changes.

## Same map, one operand lighter

Like `axpy`, `scal` produces a length-$N$ **vector**, which must leave through an
oriented output face. A 1-D domain has no such face (its only faces are the two
endpoints), so the vector rides the `i` axis and a short pipeline axis `j` turns
the elementwise scale into a two-cell systolic cell:

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 2\,\}.
$$

The result stream `r` accumulates the single scaled contribution and drains to the
terminal face:

$$
r(i,j) = r(i,j-1) + a(i-1,j)\,x(i,j-1), \qquad r(i,-1) = 0 .
$$

The only difference from `axpy` is that seed: `axpy` seeds `r` (its `y`) with the
incoming vector $Y_i$; `scal` has no addend, so it seeds with the **additive
identity** $0$. At the front cell $r(i,0) = 0 + \alpha X_i$, and the drain cell
carries it unchanged to $j = 1$.

## The three uniform flows

Exactly as in `axpy`, every operand is a uniform flow — nothing is a broadcast
constant:

- **`a` (the scalar $\alpha$)** is a 0-D operand, so it is **projected** into the
  index space: it enters on the `i = -1` edge and is pipelined across the lanes by
  the uniform shift `a(i,j) = a(i-1,j)`. (An inlined literal $\alpha$ would be a
  broadcast — an affine, non-uniform dependence; see [`axpy`](axpy.md), section
  *Uniformity*.)
- **`x` (the vector)** is **injected** on the `j = -1` halo and consumed once: its
  in-domain equation `x(i,j) = 0` is the additive identity, so the contribution
  $\alpha x_i$ is added exactly once at the front cell (a *one-shot injection*).
- **`r` (the result)** streams along `+j` and leaves at the terminal face.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 2) {
    a(i,j) = a(i-1,j);                         // scalar alpha pipelined across lanes
    x(i,j) = 0;                                // one-shot injection (additive identity in-domain)
    r(i,j) = r(i,j-1) + a(i-1,j) * x(i,j-1);   // scaled result, computed once then drained
}
```

The product `a(i-1,j) * x(i,j-1)` reads both operands at constant offsets, so `r`
is a uniform recurrence.

## Confluences (oriented faces)

Faces are equality-pinned in the domain's own coordinates; the outward normal is
*derived* (inputs are influx, `tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  Alpha[1] ((i,j) | i = -1, 0 <= j < 2) : a(i,j) = Alpha[0];  // project alpha -> normal (-1, 0)
input  X[N]     ((i,j) | 0 <= i < N, j = -1) : x(i,j) = X[i];       // inject x      -> normal (0,-1)
input           ((i,j) | 0 <= i < N, j = -1) : r(i,j) = 0;          // result seed   -> normal (0,-1)
output R[N]     ((i,j) | 0 <= i < N, j = 1)  : R[i] = r(i,j);       // scaled vector -> normal (0, 1)
```

The scalar fluxes in across the lanes, `x` and the result seed flux in along the
pipeline, and the scaled vector fluxes out at the terminal face. Derived normals:
`Alpha → (-1,0)`, `X → (0,-1)`, `seed → (0,-1)`, `R → (0,1)`. (In place: the output
tensor `R` overwrites `X`; the DSL names them separately because a confluence binds
one tensor to one oriented face.)

## Schedule and memory

`scal` inherits `axpy`'s schedule exactly. The canonical $\tau = [1,1]$ is legal —
every dependence vector (the `+j` carries, the `+i` `alpha` pipeline) has
`tau·theta = 1` — and so is the free schedule. Flux consistency needs
$\tau_i, \tau_j > 0$ so `alpha` fluxes in across the lanes and the vectors/result
along the pipeline; a backward component such as $\tau = [1,-1]$ is rejected by
flux revalidation.

The footprint matches `axpy`: under $\tau = [1,1]$ the analysis reports
`peakLiveValues = 10` for $N = 4$ (per variable `a = 5, x = 4, r = 2`), and the
eviction run realizes the same. The scaled map holds the `alpha` pipeline plus the
elementwise `x`/`r` wavefronts.

## Executable

The spec above is the executable `docs/SURE/scal.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/scal.sure
# free schedule; R = 3*{1,2,3,4} = {3,6,9,12}

dfactl --sure docs/SURE/scal.sure --tau 1,-1
# ILLEGAL: backward j-flux, rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/scal_sure.cpp` (CTest `test_scal_sure`)
parses this document's spec, checks the derived face normals, verifies
$R = \alpha X$ against a direct reference, and confirms free/linear schedule
legality and the memory analysis.

## Watch the schedule

The scaled map sweeps diagonally across the lanes under `τ = [1,1]`. Shown at N = 12. Press play, or scrub; drag to orbit.

<div class="schedule-anim" data-src="schedules/scal-linear.json" data-height="340"></div>
