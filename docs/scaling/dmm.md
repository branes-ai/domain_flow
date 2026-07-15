# The Distributed Memory Machine

Above the tile there is [no shared memory](hierarchy.md): every tile, KPU, and SoC
owns a private partition, and cross-partition data moves only by explicit messages.
The **Distributed Memory Machine (DMM)** is the abstract machine that captures this —
the target the compiler lowers to for everything a [halo](halo-vs-collective.md)
cannot cover. It has two parts: a **partitioned memory** (one partition per fabric
unit, no implicit coherence) and a small set of **collective primitives** that move
data between partitions.

Halos are handled *in the fabric* — a nearest-neighbour boundary exchange never
becomes a DMM operation. The DMM exists for the **affine** dependencies: the ones
that, at a tile boundary, are not a border swap but a domain-spanning pattern.

## The collective primitive set

Every affine dependence in the [catalog](../SURE/axpy.md) lowers
to one of four primitives. Each is named by *what data-movement shape* it is, and
each is produced by a specific dependence shape (the mapping established in
[halo vs collective](halo-vs-collective.md)):

| primitive | moves | produced by | catalog source |
|---|---|---|---|
| **reduce** | many partials → one total (combine tree) | a reduction with a single consumer | `dot`'s scalar output |
| **broadcast** | one value → every partition | a value fanned out to all cells | `axpy`'s `α`; the back-half of a normalize |
| **all-reduce** | reduce **then** broadcast the total back | a reduction whose result is read everywhere | `nrm2`→normalize; QR's `srp` |
| **scatter / gather** | distribute / collect partitions of a tensor | placing or retrieving a tiled operand | loading a blocked `A` across tiles |
| **transpose (all-to-all)** | reshuffle so axis *x* becomes axis *y* | producer tiling ≠ consumer tiling | a GEMM chain that switches contraction axis |

`all-reduce = reduce ∘ broadcast`; scatter and gather are duals. This is the entire
substrate — a compiler that can place these four shapes can realize any cross-tile
affine dependence the catalog produces.

## Cost tiers

A collective's cost is set by two things: **how many partitions** `P` it spans, and
**which hierarchy level** it has to climb to reach them (its cost per word comes
straight from the [energy–delay–distance table](hierarchy.md)). Unlike a halo — one
hop, `O(1)` partners, cost pinned to the bottom level — a collective's cost is a
function of both:

| primitive | latency | words moved | worst level reached |
|---|---|---|---|
| reduce / broadcast | `O(log P)` (tree) | `O(P)` combined / `O(P)` fanned | the level that covers all `P` |
| all-reduce | `O(log P)` | `O(P)` up + `O(P)` down | same, twice |
| scatter / gather | `O(log P)`–`O(P)` | `O(payload)` | the level the tensor spans |
| transpose / all-to-all | `O(P)` stages | `O(payload)` **every-to-every** | typically the **highest** — worst case |

Two rules of thumb fall out:

1. **Level dominates count.** A reduce over 64 tiles *inside one KPU* is far cheaper
   than a reduce over 8 partitions that *straddles two SoCs* — the off-chip crossing
   costs `~10×` per word (see the hierarchy). The compiler's placement goal is to
   keep every collective at the **lowest level that still covers its participants**.
2. **All-to-all is the cliff.** Transpose moves every partition's data to every
   other, so it has no locality to exploit and tends to reach the top of the
   hierarchy. It is the pattern to design *out* — by choosing operand layouts and
   tilings that keep producer and consumer axes aligned.

## The compiler's contract with the DMM

Putting the section together, lowering an operator graph to the DMM is a four-step
discipline:

1. **Keep it uniform.** Every dependence that can stay a constant offset stays a
   halo, handled in-fabric — never a DMM op. This is most of matmul, conv, and the
   stencils.
2. **[Uniformize](uniformization.md) what you can.** An affine *input* broadcast
   (`axpy`'s `α`) becomes a pipeline; an algorithm with a reduction-broadcast is
   swapped for one without (the [Givens QR](uniformization.md)). Each such move
   deletes a collective outright.
3. **Lower the irreducible rest** to the four primitives above, and **place each at
   the lowest hierarchy level** that covers its participants.
4. **Overlap.** Because a collective is `O(log P)` of pure latency gated on
   downstream work, the schedule hides as much of it as the dependence graph allows
   behind independent compute.

This is precisely the *explicit, decoupled data orchestration* an
[EDDO architecture](hierarchy.md) demands: the DMM primitives are the vocabulary of
that orchestration, and the [Hardware Space-Time](https://github.com/branes-ai/domain_flow/blob/main/docs/targeting-EDDO-with-Polyhedral.md)
analysis is how their space-time cost is reasoned about at compile time. Uniformity
keeps traffic at the bottom of the hierarchy; the DMM is what carries the rest.

_Part of the [Scaling & Distribution epic](https://github.com/branes-ai/domain_flow/issues/68) (issue #73). See also: [the hierarchy](hierarchy.md), [halo vs collective](halo-vs-collective.md), and [uniformization](uniformization.md)._
