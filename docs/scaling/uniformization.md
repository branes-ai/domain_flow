# Uniformization: making affine operators tile

An operator tiles cleanly only when every dependence that crosses a tile boundary
is **uniform** — a constant-offset, nearest-neighbour edge that becomes a halo
exchange (see [the scale-out problem](index.md)). **Affine** dependencies — a value
produced at one place and needed *everywhere* — become global collectives and do
not tile. **Uniformization** is the transformation that removes an affine
dependence by turning the far, all-to-one/one-to-all data movement into a chain of
nearest-neighbour hops: you *pipeline* the value through the domain.

This page works the transformation on the two flavours of broadcast the catalog
actually contains, and reports honestly where the current DSL can express it and
where it cannot.

## The easy case: a broadcast of an input — `axpy`'s `α` ✅

`axpy` is `y := α·x + y`. The scalar `α` is one value needed at **every** lane `i`
— a broadcast. Written naively, `α` is a literal in the equation body, which is an
affine dependence on a single fixed source (see [axpy](../SURE/axpy.md),
*Uniformity*). Uniformizing it: **project** `α` onto the `i = -1` boundary and
**pipeline** it across the lanes,

```text
a(i,j) = a(i-1,j);                         // α hops lane-to-lane — uniform
input Alpha[1] ((i,j) | i = -1, ...) : a(i,j) = Alpha[0];
```

so each lane receives `α` through a constant-offset hop instead of a fan-out bus.
This **is** expressible in the confluence DSL, because the pipeline is **seeded
from data** — an input confluence reads the `Alpha` tensor. A broadcast of a
*known input* uniformizes, and therefore tiles: the pipeline crosses a tile
boundary as one halo value.

## The hard case: a broadcast of a *computed reduction* — QR's `r_kj` ❌ (today)

QR's affine dependence is different in kind. In [`qr.sure`](../SURE/QR_decomposition.md)
the orthogonalization reads

```text
v(i,j,k) = v(i,j,k-1) - srp(M-1,j,k-1) * qhat(i,k-1,k-1);
```

`srp(M-1,j,k-1)` is the **completed reduction** `r_kj = Σ_i q·v` — a value produced
at the single cell `i = M-1` and read by **every** row `i`. That is exactly the
broadcast we want to pipeline. The same idea applies: reduce up the `i`-axis, then
propagate the total back down,

```text
b(i,j) = b(i+1,j);              // pipeline the reduction result back down -i
```

but the pipeline must be **seeded from the reduction result** `srp(M-1,j,k)` — a
*recurrence variable*, not an input tensor. And here the confluence DSL stops:

```console
$ dfactl --sure normalize-uniformized.sure
error: sure parser: line 12: input expressions may not read recurrence variables
```

Confluences bind **tensors** (data) to faces; they cannot seed a variable from
another variable at a face, and a uniform equation cannot case-split to do it at
`i = M-1`. So a **computed-reduction broadcast is not uniformizable in the current
DSL** — this is the essential difference from `axpy`'s input broadcast, and it is
why QR is a genuine SARE rather than a SURE we merely wrote carelessly.

## Two ways forward

1. **Extend the representation** with an *internal confluence* — a first-class
   variable→face→variable coupling that lets a reduction result seed a propagation
   axis. This is a representation question for the DFA, tracked separately; it
   would make the up-sweep / down-sweep (Blelloch-style) uniformization of *any*
   reduction-broadcast expressible.
2. **Change the algorithm so there is no broadcast to pipeline.** The
   **Gentleman–Kung Givens-rotation QR array** computes QR by a wavefront of
   Givens rotations that propagate **nearest-neighbour** through a triangular
   array — it never forms a reduction-then-broadcast, so it is **uniform by
   construction** with a linear schedule, and it tiles. This is the executable
   uniform QR the [flagship issue](https://github.com/branes-ai/domain_flow/issues/72)
   targets; deriving and verifying it in the confluence DSL (with the rotation
   `c = a/r`, `s = b/r`, `r = √(a²+b²)`) is the substantial remaining work.

## Why this matters for scaling

The uniform-vs-affine line is not a purity contest — it is the **tileability**
line. `axpy` and `matmul` uniformize (or already are uniform), so they tile with
halo exchange and scale across KPUs. MGS-QR does not, so at scale its `r_kj`
broadcasts become **all-reduce/broadcast collectives** on the Distributed Memory
Machine — the expensive path — *unless* it is first recast to the uniform Givens
array. Uniformization is thus the compiler's lever between "this tiles" and "this
needs the DMM."

_Part of the [Scaling & Distribution epic](https://github.com/branes-ai/domain_flow/issues/68) (issue #72)._
