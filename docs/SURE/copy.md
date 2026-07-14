# copy as a System of Uniform Recurrence Equations

The BLAS Level-1 `copy` duplicates a vector:

$$
y_i \;\leftarrow\; x_i, \qquad 0 \le i < N.
$$

It is the **identity flow** — the simplest operator in the catalog, and the
elementary building block from which every value-preserving movement is assembled.
A single stream carries each element from the input face to the output face,
unchanged; there is no arithmetic and no second operand.

## One flow, one face pair

The operand is carried by a single value-preserving recurrence:

$$
x(i,j) = x(i,j-1),
$$

seeded from `X` on the `j = -1` halo. The stream reaches the terminal face
unchanged and leaves as `Y`:

$$
Y[i] = x(i,\text{term}).
$$

That is the whole operator: **one input confluence, one output confluence**, a
value-preserving flow between them. `copy` is to `swap` what one lane is to the
crossed pair — [`swap`](swap.md) is two `copy` flows with the output bindings
exchanged — and it is the axis-aligned special case of `conv2d`'s value-preserving
image flow.

## Two-cell pipeline for a vector output

The result is a length-$N$ **vector**, which must leave through an oriented face; a
1-D domain has none. So the vector rides the `i` axis and a two-cell pipeline axis
`j` gives the output a proper terminal face:

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 2\,\}.
$$

The flow `x(i,j) = x(i,j-1)` is value-preserving, so `X[i]` arrives unchanged at
`j = 1` where it is read out as `Y[i]`.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 2) {
    x(i,j) = x(i,j-1);                         // value-preserving flow carrying X
}

input  X[N] ((i,j) | 0 <= i < N, j = -1) : x(i,j) = X[i];
output Y[N] ((i,j) | 0 <= i < N, j = 1)  : Y[i] = x(i,j);
```

The single tap `x(i,j-1)` is a constant offset, so `x` is a uniform recurrence.

## Confluences (oriented faces)

Faces are equality-pinned in the domain's own coordinates; the outward normal is
*derived* (inputs are influx, `tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  X[N] ((i,j) | 0 <= i < N, j = -1) : x(i,j) = X[i];   // inject X  -> normal (0,-1)
output Y[N] ((i,j) | 0 <= i < N, j = 1)  : Y[i] = x(i,j);   // drain  Y  -> normal (0, 1)
```

The operand fluxes in along the pipeline and the result fluxes out at the terminal
face. Derived normals: `X → (0,-1)`, `Y → (0,1)`. (In place, `Y` overwrites `X`; the
DSL names them separately because a confluence binds one tensor to one oriented
face.)

## Schedule and memory

`copy` is embarrassingly parallel across `i` with only the one-step `j` carry.
$\tau = [1,1]$ is legal (the single dependence vector at `tau·theta = 1`) and so is
the free schedule, at latency 2. Flux consistency needs $\tau_j > 0$ so the operand
fluxes in and the result out; a backward $\tau = [1,-1]$ is rejected by flux
revalidation.

Under $\tau = [1,1]$ the analysis reports `peakLiveValues = 4` for $N = 4$ (the
single elementwise wavefront `x = 4`), work 8, matching the eviction run — the
minimal footprint of a value-preserving flow, exactly half of `swap`'s two streams.

## Executable

The spec above is the executable `docs/SURE/copy.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/copy.sure
# free schedule; Y = X = {1,2,3,4}

dfactl --sure docs/SURE/copy.sure --tau 1,-1
# ILLEGAL: backward j-flux, rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/copy_sure.cpp` (CTest `test_copy_sure`)
parses this document's spec, checks the input and output face normals, verifies the
identity result ($Y = X$), and confirms free/linear schedule legality and the memory
analysis.
