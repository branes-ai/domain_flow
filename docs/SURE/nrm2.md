# nrm2 as a System of Uniform Recurrence Equations

The BLAS Level-1 `nrm2` is the Euclidean norm:

$$
\lVert x \rVert_2 \;=\; \sqrt{\sum_{i=0}^{N-1} x_i^{\,2}} .
$$

It is [`dot`](dot.md) turned inward — the same sum-of-products reduction, with
`x` against itself — plus one new ingredient: a **pointwise epilogue** applied to
the reduced scalar. `nrm2` is the running example for the *fused output-face
epilogue*, the confluence pattern in which the value leaving a face is a function
of the accumulator, not the accumulator itself.

## The reduction, then the epilogue

The sum of squares is a reduction identical in shape to the dot product:

$$
s(i) = s(i-1) + x_i^{\,2}, \qquad s(-1) = 0,
$$

so $s(N-1) = \sum_i x_i^2$. The norm is then $\sqrt{s(N-1)}$. The square root is
**not** part of the recurrence — it is a single pointwise operation applied *once*,
where the result leaves the domain. Placing it on the output face keeps the
accumulation a clean uniform recurrence and models the epilogue exactly where the
spatial fabric would apply it: at the drain.

## Feeding the operand: a depth-1 array

As in [`dot`](dot.md), the operand $x_i$ is indexed by the reduction coordinate
`i`, so it enters on a **depth-1 feed axis** `j` — injected on the `j = -1` halo
and consumed once at its multiply-add cell:

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 1\,\}.
$$

`nrm2` needs only *one* operand (there is no second vector), and it reads it
twice — squared. In the recurrence the two reads are the same tap `x(i,j-1)` at
one constant offset, so the parser folds them to a single dependence.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 1) {
    x(i,j) = x(i,j-1);                         // operand X, fed from the j = -1 halo
    s(i,j) = s(i-1,j) + x(i,j-1) * x(i,j-1);   // accumulate X[i]^2 along +i
}

output R[1] ((i,j) | i = N-1, 0 <= j < 1) : R[0] = sqrt(s(i,j));   // sqrt epilogue
```

The multiply-add reads its operand at the constant offset `(0,-1)` and the
previous partial sum at `(-1,0)`; both are constant offsets, so `s` is a uniform
recurrence. The output declaration reads the terminal accumulator `s(N-1,0)` and
applies `sqrt` as it writes `R[0]`.

## The fused output-face epilogue

The output confluence is more than a read: its expression may apply a pointwise
function of the value(s) it taps. Here it is $\sqrt{\,\cdot\,}$; in a fused MATMUL
it is an activation `act(c + bias)`; in [`qr`](QR_decomposition.md) the diagonal of
`R` leaves as `Rdiag = sqrt(snorm)`. In each case the epilogue

- executes **once per output element**, at the drain — not at every interior point,
  so it never pollutes the uniform interior recurrence;
- fuses into the confluence, so no separate pass and no intermediate tensor for
  $s(N-1)$ is materialized.

This is why `nrm2` is the natural home for the pattern: it is the smallest operator
whose result is a *nonlinear function of a reduction*, so the epilogue is
unavoidable and stands alone.

## Confluences (oriented faces)

Faces are equality-pinned in the domain's own coordinates; the outward normal is
*derived* as the equality plane's outward normal with respect to the domain
(inputs are influx, `tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  X[N] ((i,j) | 0 <= i < N, j = -1)  : x(i,j) = X[i];         // feed X  -> normal (0,-1)
input       ((i,j) | i = -1, 0 <= j < 1)  : s(i,j) = 0;            // seed    -> normal (-1,0)
output R[1] ((i,j) | i = N-1, 0 <= j < 1) : R[0] = sqrt(s(i,j));   // result  -> normal (1, 0)
```

The operand fluxes in across the feed, the accumulator seed fluxes in at the
`i = -1` face, and the scalar norm fluxes out at the terminal `i = N-1` face.
Derived normals: `X → (0,-1)`, `seed → (-1,0)`, `R → (1,0)`.

## Schedule

Being a linear reduction chain, `nrm2` inherits `dot`'s schedule structure exactly:
the free and linear schedules coincide (a chain has no parallelism to recover),
$\tau = [1,1]$ is legal with every dependence vector at `tau·theta = 1`, and the
latency is $N$ (4 for $N = 4$). The sqrt epilogue adds no dependence edge in the
interior — it lives on the output face — so it does not change legality. Reversing
$\tau_i$, e.g. $\tau = [-1,1]$, turns the seed into an outflux and is rejected by
flux revalidation.

## Memory cardinality

Under $\tau = [1,1]$ the analysis reports `peakLiveValues = 2` — the $O(1)$
accumulator footprint of a reduction (producer and consumer overlap for one step),
latency 4, work 8, matching the eviction run. The operand is read once at the feed
halo and never held; the epilogue holds nothing extra. As with `dot`, the resident
set is independent of $N$.

## Numerical note: the scaled (robust) variant

Accumulating $\sum x_i^2$ can overflow (or underflow to zero) even when
$\lVert x \rVert_2$ is representable — the LAPACK `snrm2` avoids this by tracking a
running scale $\sigma = \max_i |x_i|$ and the scaled sum $\sum (x_i/\sigma)^2$,
yielding $\lVert x \rVert_2 = \sigma\sqrt{\text{ssq}}$. That is a **two-reduction**
variant (a max-reduction feeding a sum-reduction) with the same feed/epilogue
structure; it is a natural extension once the max-reduction (`iamax`/`amax`) entry
lands, and is noted here rather than implemented.

## Executable

The spec above is the executable `docs/SURE/nrm2.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/nrm2.sure
# free schedule; ||x||_2 = sqrt(1 + 4 + 4 + 16) = sqrt(25) = 5

dfactl --sure docs/SURE/nrm2.sure --tau -1,1
# ILLEGAL: reversed i-flux (seed/result), rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/nrm2_sure.cpp` (CTest `test_nrm2_sure`)
parses this document's spec, checks the derived face normals, verifies
$\lVert x \rVert_2$ *through the sqrt epilogue* against a direct reference, confirms
free/linear schedule legality, and asserts the $O(1)$ accumulator footprint.

## Watch the schedule

The sum-of-squares reduction sweeps as a single point down the chain; the `sqrt` epilogue fires once at the terminal cell. Shown at N = 16. Press play, or scrub; drag to orbit.

<div class="schedule-anim" data-src="schedules/nrm2-linear.json" data-height="340"></div>
