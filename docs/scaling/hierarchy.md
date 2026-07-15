# The memory & communication hierarchy

A single KPU tile is the smallest complete fabric. Everything larger is built by
**composition** — many tiles in a KPU, many KPUs on an SoC, many SoCs in a cluster —
and every level of that composition adds a boundary that data must cross. This page
defines the levels and the cost of crossing them, and shows why **locality compounds
in value** the higher you go.

## The levels

| level | contains | shared memory? | a crossing is… |
|---|---|---|---|
| **PE** (processing element) | one MAC + registers | registers | a wire between adjacent PEs |
| **tile** | a PE array + local SRAM | tile-local SRAM | a nearest-neighbour PE hop (the systolic array) |
| **KPU** | many tiles | **no** — each tile owns its SRAM | a tile-to-tile message on the on-KPU fabric |
| **SoC** | many KPUs | **no** | a KPU-to-KPU message on the on-SoC interconnect |
| **cluster** | many SoCs | **no** | an SoC-to-SoC message over the inter-SoC network |

The decisive fact: **there is no shared memory above the tile.** A tile's PEs share
its SRAM, but two tiles — even on the same KPU — do not share anything. Every
cross-tile datum moves by an *explicit message*. That is what makes the machine a
[Distributed Memory Machine](dmm.md), and it is why the
[halo-vs-collective](halo-vs-collective.md) distinction is the whole game: a halo is
a message to the *nearest* neighbour on *one* level; a collective is a storm of
messages that may climb to the *top* level to reach its farthest participant.

## The cost of a crossing: energy–delay–distance

Moving a word is not free, and the cost is not flat — it grows sharply with the
level crossed. Three quantities move together:

- **energy** — picojoules to move the word, dominated by wire and I/O, not compute;
- **delay** — latency, which grows with physical span and with queueing on shared links;
- **distance** — how far across the machine the word physically travels (hops).

Energy tracks distance almost linearly (charging a longer wire costs more), so a word
that stays in a register is *orders of magnitude* cheaper than one that leaves the
SoC. Using relative tiers (exact figures live in the [energy model](https://github.com/branes-ai/domain_flow/tree/main/include/energy), `sw::energy`, whose `NetworkEvent` counts hop/forward traffic):

| crossing | relative energy / word | relative latency | distance |
|---|---|---|---|
| register → ALU | **1×** | 1 | 0 hops |
| tile-local SRAM | ~10× | few | on-tile |
| tile → tile (on KPU) | ~10²× | 10s | 1 fabric hop |
| KPU → KPU (on SoC) | ~10²–10³× | 100s | on-SoC |
| SoC → SoC (cluster) | ~10³×+ | 1000s | off-chip network |

The numbers are illustrative order-of-magnitude tiers, not a spec — but the *shape*
is the point: **each level up is roughly an order of magnitude more expensive per
word**, and the jump off-chip is the steepest.

> **A note on "EDDO."** In this repository **EDDO** is
> [*Explicit Decoupled Data Orchestration*](https://github.com/branes-ai/domain_flow/blob/main/docs/targeting-EDDO-with-Polyhedral.md) —
> the *architecture class* whose separate, distributed programs explicitly manage
> memory, compute, and data movement across exactly this hierarchy (no caches; data
> orchestration is programmed, not inferred). The **energy–delay–distance** triple
> above is the *cost model* that makes such orchestration worthwhile. They are not
> the same acronym: EDDO is what the machine *is*; energy–delay–distance is what a
> crossing *costs*. The KPU is an EDDO architecture, and the compiler's job is to
> orchestrate movement so that the costly crossings are rare.

## Why locality compounds

Here is the lever the whole section turns on. Consider the *same* dependence
evaluated on progressively larger fabrics:

- A **uniform** dependence (a halo) always crosses to the **nearest** neighbour. As
  the problem grows and tiles multiply, the halo still hops one level — tile-to-tile
  on the KPU. Its per-word cost is **pinned to the bottom of the hierarchy** no
  matter how big the machine gets. Scaling the problem does not move the halo up.
- An **affine** dependence (a collective) must reach **every** participant. As the
  problem grows, those participants spread across more tiles, more KPUs, eventually
  more SoCs — so the *same* logical dependence is forced to **climb the hierarchy**,
  and its cost per word climbs with it, super-linearly (energy ∝ distance × count).

So the gap between "kept it uniform" and "left it affine" is not a fixed penalty —
it **widens with scale**. On one tile the two barely differ; on a cluster the
collective is paying `10³×` per word across the whole diameter while the halo still
pays `10²×` to a neighbour. **Locality is worth more the higher you go**, which is
exactly why [uniformization](uniformization.md) — spending effort to keep a
dependence uniform — pays off more at scale, and why the compiler treats climbing the
hierarchy as the cost of last resort.

## What this sets up

The hierarchy defines *where* data lives and *what* a crossing costs. The
[Distributed Memory Machine](dmm.md) defines the *execution model* over it — the
collective primitives the compiler emits when a dependence cannot be kept local, and
their cost tiers on these levels. And [composition](index.md) is how operator graphs
are laid across the hierarchy so that most traffic stays low.

_Part of the [Scaling & Distribution epic](https://github.com/branes-ai/domain_flow/issues/68) (issue #73). See also: [halo vs collective](halo-vs-collective.md) and [the Distributed Memory Machine](dmm.md)._
