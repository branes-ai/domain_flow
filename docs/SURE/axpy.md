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

## Uniformity: every operand flows, nothing is broadcast

A **uniform** recurrence equation reads its arguments only at **constant offsets**
of the current index point: $y(p) = g\big(y(p - w_1), \dots\big)$ with the
dependence vectors $w_i$ *independent of* $p$ (Karp–Miller–Winograd, 1967). The
temptation with `axpy` is to write

$$
y(i,j) = y(i,j-1) + \alpha\,x(i,j-1),
$$

treating $\alpha$ as a literal scalar in the equation body. That is **not** a
uniform recurrence: a single value $\alpha$ read at every point is a *broadcast*,
whose dependence on the fixed source $p_0$ is $p - p_0$ — it grows with $p$. That
is an **affine** dependence, the very thing uniformity forbids, and physically it
is a fan-out bus rather than a nearest-neighbour connection.

$\alpha$ is an operand of `axpy` — the `a` in `a·x + y` — on equal footing with
the vectors $x$ and $y$. So, like them, it must enter through a confluence and be
carried by a uniform recurrence. Because $\alpha$ is a 0-dimensional scalar, we
**project** it into the index space: inject it on the `i = -1` boundary edge and
pipeline it across the lanes with the uniform shift

$$
a(i,j) = a(i-1,j),
$$

so every lane $i$ receives the same $\alpha$ through constant-offset hops instead
of a broadcast. This is the standard *uniformization* of a parameter: replace the
broadcast by a propagation variable seeded on a boundary. The product
$a(i-1,j)\cdot x(i,j-1)$ is then uniform — both taps are constant offsets of
$(i,j)$ — exactly as `matmul`'s $a(i,j-1,k)\,b(i-1,j,k)$ is uniform despite being
nonlinear in the *values*.

## Domain

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 2\,\}
$$

- `i` indexes the $N$ vector elements — the spatial extent of every face.
- `j` is a two-step pipeline: **inject** at the front, **drain** at the back.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 2) {
    a(i,j) = a(i-1,j);                         // coefficient alpha, pipelined across lanes
    x(i,j) = 0;                                // one-shot injection: in-domain x is the additive identity
    y(i,j) = y(i,j-1) + a(i-1,j) * x(i,j-1);   // stream y along +j, pick up alpha*x once
}
```

- `a(i,j)` carries the scalar $\alpha$. It is seeded on the `i = -1` edge (see the
  confluences below) and propagates along `+i` by the uniform shift `a(i-1,j)`, so
  every lane holds $\alpha$.
- `x(i,j)` is the vector operand's carrier. The datum $x_i$ is **injected on the
  halo** `j = -1`; the in-domain equation `x(i,j) = 0` is a one-shot injection
  (defined precisely below).
- `y(i,j)` streams along `+j`. It is seeded on the halo `j = -1` with the incoming
  vector $y_i$, accumulates $\alpha\,x_i$ at `j = 0`, and passes through unchanged
  to `j = 1`.

### One-shot injection

The `y` equation is an **accumulation** along `j`: it is the prefix sum of the
increments $m(i,j) = a(i-1,j)\,x(i,j-1)$ over the pipeline, on top of the seed
$y(i,-1) = y_i$. Its value at the drain face is

$$
y(i,1) \;=\; y_i \;+\; \sum_{j=0}^{1} a(i-1,j)\,x(i,j-1)
        \;=\; y_i \;+\; \alpha \sum_{j=0}^{1} x(i,j-1),
$$

since $a \equiv \alpha$. For this to equal the target $y_i + \alpha x_i$, the
increments must contribute $x_i$ **exactly once**, i.e. $\sum_{j} x(i,j-1) = x_i$.

We arrange that by making `x` a **one-shot injection**. Read the accumulation's
$+$ as the monoid $(\mathbb{R}, +, 0)$ with identity element $0$. The datum $x_i$
is placed *only* on the injection face (the halo `j = -1`); the in-domain equation
sets the carrier to that **additive identity**, `x(i,j) = 0`. The tap `x(i,j-1)`
therefore reads $x_i$ at the single step `j = 0` whose offset reaches the face, and
the identity $0$ at every later step:

$$
x(i,j-1) = \begin{cases} x_i & j = 0 \ (\text{taps the halo}) \\[2pt] 0 & j \ge 1 \ (\text{taps in-domain}) \end{cases}
\qquad\Longrightarrow\qquad \sum_{j} x(i,j-1) = x_i.
$$

So the accumulation adds $\alpha x_i$ once and the identity thereafter — the
injected datum is consumed by the one cell that reads the face and leaves no
residue in the stream. (This is the general convention for a value that must enter
a reduction exactly once: inject on the boundary, reset in-domain to the
accumulator's identity element — $0$ for $+$, $1$ for $\times$.)

Tracing a fixed `i` makes the two increments explicit:

| step | value |
|------|-------|
| `y(i,0) = y(i,-1) + a(i-1,0)·x(i,-1)` | $y_i + \alpha x_i$  &nbsp;(`x(i,-1)=x_i`, the injection) |
| `y(i,1) = y(i,0)  + a(i-1,1)·x(i,0)`  | $(y_i + \alpha x_i) + \alpha\cdot 0 = y_i + \alpha x_i$  &nbsp;(`x(i,0)=0`, the identity) |

The result $R_i = \alpha x_i + y_i$ is present at the terminal face `j = 1`.

## Confluences (oriented faces)

Faces are equality-pinned in the domain's own coordinates; the outward normal is
*derived* as the equality plane's outward normal with respect to the domain
(inputs are influx, `tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  Alpha[1] ((i,j) | i = -1, 0 <= j < 2) : a(i,j) = Alpha[0];  // project alpha -> normal (-1, 0)
input  X[N]     ((i,j) | 0 <= i < N, j = -1) : x(i,j) = X[i];       // inject x      -> normal (0,-1)
input  Y[N]     ((i,j) | 0 <= i < N, j = -1) : y(i,j) = Y[i];       // seed y        -> normal (0,-1)
output R[N]     ((i,j) | 0 <= i < N, j = 1)  : R[i] = y(i,j);       // drain R       -> normal (0, 1)
```

The scalar $\alpha$ is projected onto the `i = -1` edge (a codimension-1 face over
`j`); the vectors `x` and `y` sit on the one-step halo `j = -1` where their
boundary values are read; the result leaves the terminal `j = 1` face. The derived
normals are `Alpha → (-1,0)`, `X, Y → (0,-1)`, and `R → (0,1)` — `alpha` fluxes in
across the lanes, the vectors flux in along the pipeline, and the result fluxes
out.

## Schedule

The kernel is parallel across `i`, with a one-step `j` carry and the lane-to-lane
`i` carry that pipelines `alpha`. The canonical linear schedule

$$
\tau = [\,1,\; 1\,]
$$

is legal — every dependence vector (`+j` for the `x`/`y` carries, `+i` for the
`alpha` pipeline) has `tau·theta = 1 ≥ 1` — and so is the
[free schedule](../simulator/), the data-flow-earliest firing order.

Flux consistency picks out the legal schedules directly from the geometry. With
the derived normals `Alpha → (-1,0)`, `X, Y → (0,-1)`, `R → (0,1)`, any
$\tau = [\tau_i, \tau_j]$ with $\tau_i > 0$ and $\tau_j > 0$ makes `alpha` flux in
across the lanes (`tau·n = -τ_i < 0`), the vectors flux in along the pipeline
(`tau·n = -τ_j < 0`), and the result flux out (`tau·n = +τ_j > 0`). A backward
component, e.g. $\tau = [1,-1]$, reverses a flow and is rejected by flux
revalidation before legality is even consulted.

## Memory cardinality

Under $\tau = [1,1]$ the three carriers (`a`, `x`, `y`) hold a bounded wavefront
of live values. For $N = 4$ the analysis reports `peakLiveValues = 8` (per
variable `a = 4, x = 2, y = 2`), latency 5, and work 24, and the eviction run
realizes the same footprint. The `alpha` pipeline `a` costs a wavefront across the
lanes; the vector carriers `x` and `y` are the baseline elementwise footprint.
Reductions (`dot`, `nrm2`) collapse a carrier to a single accumulator, and
Level-2/3 operators grow it by the reduction extent. (The free schedule trades a
larger footprint — `peakLiveValues = 10` — for lower latency.)

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

$\alpha$ enters as data on the `Alpha` confluence, not as a literal in an equation
body, so it is a genuine flowing operand rather than a broadcast constant. The
executable binds `data Alpha = 2` for an exact integer check, but nothing in the
derivation depends on the value: the same projection carries any real $\alpha$, and
a per-lane coefficient (`data Alpha = { ... }` read along the `i = -1` edge)
generalizes `axpy` to a diagonal scaling without changing the domain, the
confluence structure, or the schedule.
