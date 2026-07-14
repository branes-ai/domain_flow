# Halo vs collective: the cross-tile dependency dichotomy

When an operator is [tiled](tiling.md), every dependence in its recurrence either
stays inside a tile or crosses a tile boundary. The ones that cross are the entire
story of scaling, and there are only **two kinds**:

- a **uniform** (constant-offset) dependence becomes a **halo** — a nearest-neighbour
  boundary exchange with the adjacent tile;
- an **affine / domain-spanning** dependence becomes a **collective** — an
  all-reduce, broadcast, or transpose carried across the whole machine by the
  [Distributed Memory Machine](index.md).

Which one you get is **not** a runtime property — it is fixed by the *shape of the
dependence vector* in the recurrence, and can be read off at compile time. That is
why the catalog is so insistent about uniform-vs-affine: it *is* the halo-vs-collective
distinction, decided before a single tile is placed.

## The halo — a uniform dependence at a boundary

A constant-offset dependence `v(p) = v(p − δ)` with `δ` fixed reaches at most `|δ|`
cells past a tile edge. Cut the domain anywhere and the consumer tile needs only a
thin **border** of the producer tile — a halo.

**Stencil / convolution** is the textbook case.
[`conv2d.sure`](../SURE/conv2d.md) slides a 3×3 window:

```text
x(i,j,k,l) = x(i-1,j,k+1,l);          // padded-image value flows along an anti-diagonal
w(i,j,k,l) = w(i-1,j,k,l);            // kernel weight flows along +i
p(i,j,k,l) = p(i,j,k,l-1) + x*w;      // row sum  (uniform, along +l)
s(i,j,k,l) = s(i,j,k-1,l) + p(..,1);  // column sum (uniform, along +k)
```

Every dependence is a constant offset. Tile the `N×N` image into blocks; an output
pixel at a block edge needs the `⌊K/2⌋`-wide ring of input pixels from the
neighbouring block — a **halo of width 1** for a 3×3 kernel. Nothing else crosses.

```text
   tile A │ tile B                 the shaded column is A's east border;
   · · · ·│· · · ·                 B reads it to evaluate its west edge.
   · · · ▓│░ · · ·   halo width 1  A sends O(T) values to ONE neighbour,
   · · · ▓│░ · · ·   (3x3 kernel)  overlaps the send with interior compute,
   · · · ▓│░ · · ·                 and never talks to any farther tile.
   · · · ·│· · · ·
```

The same is true of [matmul](tiling.md): its `a`/`b`/`c` deps are all `±1`, so all
three cross-tile edges are halos. **Halo cost is `O(surface)`** — see below — and it
is nearest-neighbour, so it scales.

## The collective — an affine or domain-spanning dependence

An affine dependence names a source that does not sit a fixed offset away — a value
produced at *one* place and needed *everywhere*, or a total gathered from the
*whole* domain. At a tile boundary that is not a border exchange; it is a **global**
communication pattern. Three shapes cover the catalog:

| collective | arises from | catalog example |
|---|---|---|
| **reduce / all-reduce** | a reduction `Σ` whose axis is split across tiles — each tile makes a partial, and they must be combined (and, if the result is reused, broadcast back) | [`dot`](../SURE/dot.md), [`nrm2`](../SURE/nrm2.md), [`asum`](../SURE/asum.md); QR's `srp` |
| **broadcast** | one value fanned out to every cell | `axpy`'s `α` (before [uniformization](uniformization.md)) |
| **transpose / all-to-all** | the producer's tiling and the consumer's tiling disagree on which axis is split | a GEMM whose output feeds the next op along a different axis |

**Reduction** is the one to watch, because it is subtle.
[`dot`](../SURE/dot.md) is `s(i) = s(i-1) + X[i]·Y[i]` — inside a
single tile that accumulation chain is *uniform* (a `−1` hand-off, a pipeline). The
collective appears only when the **reduction axis itself is split** across tiles:
tile 0 produces a partial `P₀`, tile 1 produces `P₁`, … and the result is
`P₀+P₁+…+P_{P−1}` — a combine across all tiles.

- If the sum has a **single consumer** (dot's scalar output), that combine is a
  **reduce/gather** — a tree of depth `O(log P)`.
- If the result is **read back by every cell** (nrm2 normalizes each element by
  `‖x‖`; QR's `srp` is fanned to every row), it is a full **all-reduce**: the tree
  combine *plus* a broadcast back down. This is exactly the reduction-broadcast that
  [uniformization](uniformization.md) shows the DSL cannot yet pipeline — it is a
  genuine collective, not a halo.

```text
   all-reduce over P tiles                 t0   t1   t2   t3
   ─ combine up a tree  (log P deep)        \   /     \   /
   ─ then broadcast the total back           +        +      combine
                                              \      /
                                               \    /
                                                 +          total
                                              /  |  \  \
                                            t0  t1  t2  t3   broadcast back
   every tile waits on a machine-wide barrier; no near-neighbour locality.
```

## The cost argument

The two patterns live on opposite ends of the cost scale, and the gap *widens* with
scale.

**Halo — amortizes away.** A `d`-dimensional tile of side `T` does `O(T^d)` interior
work and exchanges `O(T^{d−1})` boundary words with a constant number of neighbours.
The **surface-to-volume ratio is `O(1/T)`** → as tiles grow, communication becomes a
vanishing fraction of compute, and the exchange is a **single hop** that overlaps
with the interior update. Halo-bound operators are weak-scaling-friendly.

**Collective — a latency floor that never amortizes.** A reduction/broadcast over
`P` tiles has a latency floor of `O(log P)` (tree) or `O(P)` (ring) that is there
**regardless of payload size** — it is a machine-wide synchronization. Its data
crosses `O(diameter)` of the interconnect, so its **energy scales with distance**,
not just volume, and it is hard to overlap because downstream work is *gated* on the
combined result.

| | halo (uniform) | collective (affine) |
|---|---|---|
| pattern | nearest-neighbour boundary exchange | all-reduce / broadcast / all-to-all |
| partners | `O(1)` neighbours | all `P` tiles |
| latency | `O(1)` hop | `O(log P)`–`O(P)`, payload-independent floor |
| data moved | `O(T^{d−1})` per tile | `O(payload)` across the whole machine |
| energy | local, `∝` surface | `∝` distance × payload |
| overlaps compute? | yes | poorly — downstream is gated on it |
| scales? | **yes** (`S/V → 0`) | only if it is rare, or removed |

## The compiler's job

This is why the whole section keeps returning to **uniformity**. The compiler cannot
make a collective cheap — its cost is set by the machine's diameter. What it *can*
do is keep dependences uniform so they stay halos, and pay a collective only where an
affine dependence genuinely demands one:

1. keep the recurrence uniform where the algorithm allows it (matmul, conv, stencils
   are already there);
2. [**uniformize**](uniformization.md) an affine dependence into a pipeline when it
   is a broadcast of an *input* (axpy's `α`), or choose an algorithm with no
   reduction-broadcast at all (the [Givens QR](uniformization.md));
3. and where a collective is irreducible, place it, schedule it, and hide as much of
   it behind compute as the dependence graph permits — the job of the
   [Distributed Memory Machine](index.md).

Uniform-vs-affine is the tiling criterion; halo-vs-collective is what it costs.

_Part of the [Scaling & Distribution epic](https://github.com/branes-ai/domain_flow/issues/68) (issue #71). See also: [tiling the index space](tiling.md) and [uniformization](uniformization.md)._
