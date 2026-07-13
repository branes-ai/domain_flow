# axpy as a System of Uniform Recurrence Equations

`axpy` — **a·x plus y** — is the archetypal BLAS Level-1 kernel:

$$
y_i \;\leftarrow\; \alpha\,x_i + y_i, \qquad 0 \le i < N,
$$

for a scalar $\alpha$ and length-$N$ vectors $x$ and $y$. It is the smallest
useful catalog entry and the running example for how a *fully-parallel* vector
operation is expressed in the domain-flow / confluence formalism.

## The modelling problem: a map has no recurrence depth

Unlike `matmul` (a reduction along `k`) or `dot` (a running sum), `axpy` has **no
inter-index dependence**: every output element $y_i$ is an independent
multiply-add. As a system of recurrence equations its natural domain is the
one-dimensional index set $\{\,i \mid 0 \le i < N\,\}$ with a single equation and
no taps into other index points.

That is faithful, but it does not fit the *confluence* vocabulary the domain-flow
representation uses to place data on the spatial fabric. In that vocabulary a
tensor enters or leaves through an **oriented face** of the domain — a
codimension-1 region pinned by one equality, with the flow direction given by the
face's outward normal. A length-$N$ result must leave through a face that spans
all $N$ elements, and an oriented face needs the domain to have extent on *both*
sides of it. A 1-D domain cannot supply that: its only faces are the two endpoints.

So we give the vector a **flow axis**. The length-$N$ vector rides the `i` axis
(the face extent), and a short pipeline axis `j` turns the elementwise map into a
two-cell systolic cell whose result exits through a proper terminal face.

## Domain

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 2\,\}
$$

- `i` indexes the $N$ vector elements — the spatial extent of every face.
- `j` is a two-step pipeline: **inject** at the front, **drain** at the back.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 2) {
    x(i,j) = 0;                                // injected value is spent in-domain
    y(i,j) = y(i,j-1) + alpha * x(i,j-1);      // stream y along +j, pick up alpha*x once
}
```

- `x(i,j)` is the scaled operand's carrier. Its *in-domain* value is `0`; the
  actual data $x_i$ is **injected on the halo** `j = -1` (see the confluences
  below). Because in-domain `x` is zero, the contribution $\alpha\,x_i$ is added
  exactly once — at the front cell `j = 0` — and never again as `y` drains.
- `y(i,j)` streams along `+j`. It is seeded on the halo `j = -1` with the incoming
  vector $y_i$, accumulates $\alpha\,x_i$ at `j = 0`, and passes through unchanged
  to `j = 1`.

Tracing a fixed `i` (with $x_i$ on the `j=-1` halo of `x` and $y_i$ on the `j=-1`
halo of `y`):

| step | value |
|------|-------|
| `y(i,0) = y(i,-1) + α·x(i,-1)` | $y_i + \alpha x_i$ |
| `y(i,1) = y(i,0)  + α·x(i,0)`  | $(y_i + \alpha x_i) + \alpha\cdot 0 = y_i + \alpha x_i$ |

The result $R_i = \alpha x_i + y_i$ is present at the terminal face `j = 1`.

## Confluences (oriented faces)

Faces are equality-pinned in the domain's own coordinates; the outward normal is
*derived* as the equality plane's outward normal with respect to the domain
(inputs are influx, `tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  X[N] ((i,j) | 0 <= i < N, j = -1) : x(i,j) = X[i];   // inject x  -> normal (0,-1)
input  Y[N] ((i,j) | 0 <= i < N, j = -1) : y(i,j) = Y[i];   // seed y    -> normal (0,-1)
output R[N] ((i,j) | 0 <= i < N, j = 1)  : R[i] = y(i,j);   // drain R   -> normal (0, 1)
```

Both inputs sit on the one-step halo `j = -1` where the boundary values are
actually read; the result leaves the terminal `j = 1` face. The derived normals
are `X, Y → (0,-1)` and `R → (0,1)`.

## Schedule

The kernel is embarrassingly parallel across `i`, and the only dependence is the
one-step `j` carry. The canonical linear schedule

$$
\tau = [\,1,\; 1\,]
$$

is legal (`tau·theta ≥ 1` on the single `j`-carried edge), and so is the
[free schedule](../simulator/) — the data-flow-earliest firing order — which puts
all of `j = 0` at one wavefront and all of `j = 1` at the next.

Flux consistency picks out the legal schedules directly from the geometry: with
the input normal `(0,-1)` and output normal `(0,1)`, any $\tau = [\tau_i, \tau_j]$
with $\tau_j > 0$ makes the inputs influx (`tau·n = -τ_j < 0`) and the output
outflux (`tau·n = +τ_j > 0`). A backward $\tau_j$, e.g. $\tau = [1,-1]$, reverses
the flow and is rejected by flux revalidation before legality is even consulted.

## Memory cardinality

Under $\tau = [1,1]$ each carrier holds one live value per `i` at a time, so the
peak resident set is $2N$ (the `x` and `y` wavefronts), with latency 3 and total
work $2N \cdot 2$. For $N = 4$: `peakLiveValues = 8`, matching the eviction run.
This is the baseline elementwise footprint; reductions (`dot`, `nrm2`) collapse
the carrier to a single accumulator, and Level-2/3 operators grow it by the
reduction extent.

## Executable

The spec above is the executable `docs/SURE/axpy.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/axpy.sure
# free schedule; R = 2*{1,2,3,4} + {10,20,30,40} = {12,24,36,48}

dfactl --sure docs/SURE/axpy.sure --tau 1,-1
# ILLEGAL: backward j-flux, rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/axpy_sure.cpp` (CTest `test_axpy_sure`)
parses this document's spec, checks the derived face normals, verifies
$R = \alpha x + y$ against a direct reference, and confirms free/linear schedule
legality and the memory analysis.

## Note on the scalar α

The DSL binds integer parameters, so the executable pins $\alpha = 2$ for an exact
check. Nothing in the derivation depends on that: $\alpha$ is a coefficient in the
`y` equation, so a real scalar (or a per-lane scalar read on the injection face)
generalizes the kernel without changing the domain, the confluence structure, or
the schedule.
