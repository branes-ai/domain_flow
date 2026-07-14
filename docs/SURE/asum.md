# asum as a System of Uniform Recurrence Equations

The BLAS Level-1 `asum` is the sum of absolute values (the $\ell_1$ norm):

$$
\operatorname{asum}(x) \;=\; \sum_{i=0}^{N-1} \lvert x_i \rvert .
$$

Like [`dot`](dot.md) and [`nrm2`](nrm2.md) it is a linear-chain reduction; what
makes it worth its own entry is *where the pointwise nonlinearity lives*. `nrm2`
applies `sqrt` to the **result**, on the output face. `asum` applies `abs` to each
**operand**, on the input face. `asum` is the running example for the
**input-face prologue** — the mirror image of the output-face epilogue.

## The reduction, with the map at the door

The accumulation is a pure sum:

$$
s(i) = s(i-1) + \lvert x_i \rvert, \qquad s(-1) = 0,
$$

so $s(N-1) = \sum_i |x_i|$. The absolute value is not part of the accumulation
recurrence — it is a pointwise map applied to the operand *as it enters the
domain*. Fusing it onto the input confluence keeps the interior a clean uniform
sum (identical to `dot`'s, minus the second operand) and models the `abs` exactly
where a spatial fabric would rectify the incoming stream: at the feed.

### Prologue and epilogue, side by side

The confluence carries an expression, and a pointwise op may ride it at either
end of the flow:

| operator | pointwise op | confluence | when it runs |
|----------|--------------|------------|--------------|
| `asum`   | `abs`        | **input** face `x = abs(X[i])` | as the operand **enters** |
| `nrm2`   | `sqrt`       | **output** face `R = sqrt(s)`  | as the result **leaves** |

In both the interior recurrence is a plain uniform sum; the nonlinearity is
pushed to the boundary where it executes once per element (prologue) or once for
the result (epilogue), never polluting the interior.

## Feeding the operand: a depth-1 array

As in [`dot`](dot.md), the operand $x_i$ is indexed by the reduction coordinate
`i`, so it enters on a **depth-1 feed axis** `j` — injected on the `j = -1` halo
and consumed once at its add cell:

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 1\,\}.
$$

The input face injects $\lvert X[i] \rvert$: the `abs` is evaluated on the halo
value as it is read.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 1) {
    x(i,j) = x(i,j-1);                         // operand |X|, fed from the j = -1 halo
    s(i,j) = s(i-1,j) + x(i,j-1);              // accumulate |X[i]| along +i
}

input X[N] ((i,j) | 0 <= i < N, j = -1) : x(i,j) = abs(X[i]);   // abs prologue
```

The add reads its operand at the constant offset `(0,-1)` and the previous partial
sum at `(-1,0)`; both are constant offsets, so `s` is a uniform recurrence. The
input confluence evaluates `abs(X[i])` as it seeds the feed.

## Confluences (oriented faces)

Faces are equality-pinned in the domain's own coordinates; the outward normal is
*derived* as the equality plane's outward normal with respect to the domain
(inputs are influx, `tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  X[N] ((i,j) | 0 <= i < N, j = -1)  : x(i,j) = abs(X[i]);   // feed |X|  -> normal (0,-1)
input       ((i,j) | i = -1, 0 <= j < 1)  : s(i,j) = 0;           // seed      -> normal (-1,0)
output S[1] ((i,j) | i = N-1, 0 <= j < 1) : S[0] = s(i,j);        // result    -> normal (1, 0)
```

The rectified operand fluxes in across the feed, the accumulator seed fluxes in at
the `i = -1` face, and the scalar sum fluxes out at the terminal `i = N-1` face.
Derived normals: `X → (0,-1)`, `seed → (-1,0)`, `S → (1,0)`.

## Schedule

Being a linear reduction chain, `asum` inherits the schedule structure of `dot`
exactly: the free and linear schedules coincide (no parallelism in a chain to
recover), $\tau = [1,1]$ is legal with every dependence vector at `tau·theta = 1`,
and the latency is $N$ (4 for $N = 4$). The `abs` prologue lives on the input face,
so it adds no interior dependence edge and does not affect legality. Reversing
$\tau_i$, e.g. $\tau = [-1,1]$, turns the seed into an outflux and is rejected by
flux revalidation.

## Memory cardinality

Under $\tau = [1,1]$ the analysis reports `peakLiveValues = 2` — the $O(1)$
accumulator footprint of a reduction (producer and consumer overlap for one step),
latency 4, work 8, matching the eviction run. The operand is read once at the feed
halo and never held; the prologue holds nothing extra. As with `dot` and `nrm2`,
the resident set is independent of $N$.

## Executable

The spec above is the executable `docs/SURE/asum.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/asum.sure
# free schedule; asum = |1| + |-2| + |3| + |-4| = 10

dfactl --sure docs/SURE/asum.sure --tau -1,1
# ILLEGAL: reversed i-flux (seed/result), rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/asum_sure.cpp` (CTest `test_asum_sure`)
parses this document's spec, checks the derived face normals, verifies
$\sum_i |x_i|$ *through the abs prologue* (signed input data) against a direct
reference, confirms free/linear schedule legality, and asserts the $O(1)$
accumulator footprint.
