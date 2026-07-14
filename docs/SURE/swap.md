# swap as a System of Uniform Recurrence Equations

The BLAS Level-1 `swap` exchanges two vectors:

$$
(x_i, y_i) \;\leftarrow\; (y_i, x_i), \qquad 0 \le i < N.
$$

It carries no arithmetic at all, which makes it the catalog's cleanest example of
a **multi-tensor confluence with crossed routing**: the whole operation lives in
*which flow leaves through which named face*.

## Routing, not computing

Two value-preserving flows carry the operands straight through the domain:

$$
x(i,j) = x(i,j-1), \qquad y(i,j) = y(i,j-1),
$$

with `x` seeded from `X` and `y` from `Y` on the `j = -1` halo. Nothing is added,
scaled, or reduced. The exchange happens where the flows *exit*: the first output
slot is bound to the `y`-flow and the second to the `x`-flow.

$$
\mathrm{Xout}[i] = y(i,\text{term}), \qquad \mathrm{Yout}[i] = x(i,\text{term}).
$$

A confluence is a *tensor ↔ oriented-face* association; here two input tensors bind
to two feed faces and two output tensors bind to the terminal face, with the
binding **crossed**. `swap` shows that a confluence is a routing decision — the
data-movement operators (`swap`, `copy`) are pure confluence, no interior compute.

## Two vectors, one two-cell pipeline

Both results are length-$N$ **vectors**, so — as for every vector output in the
catalog (`axpy`, `scal`) — they must leave through an oriented face, which a 1-D
domain cannot provide. The vectors ride the `i` axis and a two-cell pipeline axis
`j` gives the outputs a proper terminal face:

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 2\,\}.
$$

The flows are value-preserving (`x(i,j) = x(i,j-1)`), so `X[i]` and `Y[i]` arrive
unchanged at the terminal `j = 1` face where they are read out — crossed.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 2) {
    x(i,j) = x(i,j-1);                         // value-preserving flow carrying X
    y(i,j) = y(i,j-1);                         // value-preserving flow carrying Y
}
```

Each flow taps its predecessor at the constant offset `(0,-1)`, so both are uniform
recurrences. There is no cross-coupling in the *interior* — `x` never reads `y`;
the two streams are independent until the confluence crosses them at the drain.

## Confluences (oriented faces)

Faces are equality-pinned in the domain's own coordinates; the outward normal is
*derived* (inputs are influx, `tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  X[N]    ((i,j) | 0 <= i < N, j = -1) : x(i,j) = X[i];       // inject X  -> normal (0,-1)
input  Y[N]    ((i,j) | 0 <= i < N, j = -1) : y(i,j) = Y[i];       // inject Y  -> normal (0,-1)
output Xout[N] ((i,j) | 0 <= i < N, j = 1)  : Xout[i] = y(i,j);    // 1st slot <- Y  -> normal (0,1)
output Yout[N] ((i,j) | 0 <= i < N, j = 1)  : Yout[i] = x(i,j);    // 2nd slot <- X  -> normal (0,1)
```

Both operands flux in along the pipeline and both results flux out at the terminal
face — the two output confluences share the `j = 1` face but bind different tensors
to different flows. Derived normals: `X, Y → (0,-1)`, `Xout, Yout → (0,1)`. (In
place, `Xout`/`Yout` overwrite `X`/`Y`; the DSL names the four tensors separately
because a confluence binds one tensor to one oriented face.)

## Schedule and memory

The two independent flows make `swap` embarrassingly parallel across `i`, with only
the one-step `j` carry. $\tau = [1,1]$ is legal (every dependence vector at
`tau·theta = 1`) and so is the free schedule, at latency 2 (the two `j` steps).
Flux consistency needs $\tau_j > 0$ so both operands flux in and both results out; a
backward $\tau = [1,-1]$ is rejected by flux revalidation.

Under $\tau = [1,1]$ the analysis reports `peakLiveValues = 8` for $N = 4$ (per
variable `x = 4, y = 4` — the two elementwise wavefronts), work 16, matching the
eviction run. Twice `copy`'s footprint, as expected of two independent flows.

## Executable

The spec above is the executable `docs/SURE/swap.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/swap.sure
# free schedule; (X,Y) = ({1,2,3,4},{10,20,30,40}) -> (Xout,Yout) = (Y,X)

dfactl --sure docs/SURE/swap.sure --tau 1,-1
# ILLEGAL: backward j-flux, rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/swap_sure.cpp` (CTest `test_swap_sure`)
parses this document's spec, checks the two input and two output face normals,
verifies the crossed result ($\mathrm{Xout} = Y$, $\mathrm{Yout} = X$), and confirms
free/linear schedule legality and the memory analysis.

## Watch the schedule

Two value-preserving flows sweep the strip together and leave crossed. Shown at N = 12. Press play, or scrub; drag to orbit.

<div class="schedule-anim" data-src="schedules/swap-linear.json" data-height="340"></div>
