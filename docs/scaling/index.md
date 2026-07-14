# Scaling & Distribution

The [`SURE Algorithms`](../SURE/axpy.md) catalog derives each
operator as if its whole index space fits one fabric. Real problems don't fit.
A KPU tile has a **fixed** spatial extent — a fixed processing-element array and a
fixed local memory — but an operator's domain of computation **grows with the
problem size** `N`. This section is about what changes when the domain no longer
fits: how algorithms are **tiled** across many KPU tiles, and eventually across
many SoCs.

## The scale-out problem

Two things must happen when the domain exceeds the fabric:

1. **Partition the domain** into tile-sized blocks — *capacity*. Each block maps
   to a tile; blocks are executed spatially (across tiles) and/or temporally
   (reusing a tile over time).
2. **Handle the dependencies that now cross a tile boundary** — *communication*.
   A dependence that used to be a wire between adjacent processing elements
   becomes a message between tiles.

Partitioning is the easy half. The communication is where scaling is won or lost,
and it is decided entirely by the **kind of dependency** that crosses the cut.

## The one thing that matters: what crosses a tile boundary?

| dependency in the recurrence | at a tile boundary it becomes | cost | tiles? |
|---|---|---|---|
| **uniform** — constant-offset, nearest-neighbour (`x(i,j) = x(i-1,j)`) | a **halo / boundary exchange** with the adjacent tile | local, `O(surface)` | **yes, linearly** |
| **affine / broadcast** — spans the domain (a reduction result fanned out to every cell; a fixed far row read) | a **collective** — all-reduce, broadcast, transpose — on the **Distributed Memory Machine** | global, payload/topology-dependent, latency-bound | **only after uniformization** |

```text
   uniform (SURE)                         affine (SARE)
   ┌─────┬─────┬─────┐                    ┌─────┬─────┬─────┐
   │ t00 │ t01 │ t02 │  halo: each tile   │ t00 │ t01 │ t02 │  collective: a value
   ├──╫──┼──╫──┼──╫──┤  needs only a thin ├─────┼──█──┼─────┤  at one tile must reach
   │ t10 │ t11 │ t12 │  border from its   │ t10 │ █←█→█ │ t12 │  ALL tiles — an all-reduce
   ├──╫──┼──╫──┼──╫──┤  neighbours        ├─────┼──█──┼─────┤  or broadcast across the
   │ t20 │ t21 │ t22 │  (║ = halo swap)   │ t20 │ t21 │ t22 │  whole grid (█ = long haul)
   └─────┴─────┴─────┘                    └─────┴─────┴─────┘
```

This is why the catalog cares so much about **uniformity**, and why the
[QR page](../SURE/QR_decomposition.md) is flagged as a **SARE**, not a SURE: a
uniform recurrence tiles by nearest-neighbour halo exchange and scales; an affine
one forces global communication and does **not** tile until its affine
dependencies are removed. Uniform-vs-affine *is* the tiling criterion.

## What has to be solved

The rest of this section works through each piece:

- **Tiling the index space** — index-set partitioning and the two-level (across-tile
  + within-tile) schedule, with **blocked matmul** as the clean example.
- **Halo vs collective** — the dependency dichotomy above, worked out concretely.
- **[Uniformization](uniformization.md)** — turning affine dependencies into uniform
  ones so an operator tiles: pipeline the broadcast, or change algorithm (the
  Givens-rotation QR). This is where QR's "SARE dishonesty" is resolved for real.
- **The memory & communication hierarchy and the DMM** — PE → tile → KPU → SoC →
  cluster; the energy–delay–**distance** cost; and the Distributed Memory Machine
  execution model that carries the collectives.
- **Composition across the hierarchy** — operator graphs that span tiles and SoCs,
  and where the compiler places the collectives.

_Tracked by the [Scaling & Distribution epic](https://github.com/branes-ai/domain_flow/issues/68)._
