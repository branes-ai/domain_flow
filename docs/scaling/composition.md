# Composition across the hierarchy

The [catalog](../SURE/axpy.md) derives one operator at a time. A real workload is a
**graph** of them — a [domain flow graph](../ideation-of-dfg-technology.md) whose
nodes are operators and whose edges carry tensors between them. Once every node is
[tiled](tiling.md), the graph becomes a *graph of tiled nodes*, and a new kind of
communication appears — **on the edges, between operators** — on top of the
intra-operator [halos and collectives](halo-vs-collective.md) inside each node.

This page is about that inter-operator layer: where the edge collectives come from,
how the compiler places them, and the global mapping problem of laying the whole
graph across the [hierarchy](hierarchy.md).

## The edge is where inter-operator communication lives

A DFG edge carries a tensor from a producer to a consumer. Each endpoint imposes a
**tiling** on that tensor — which axis is split across fabric units, and how the
blocks are shaped. The edge's cost is decided entirely by whether those two tilings
**agree**:

- **Aligned edge** — the producer's output tiling matches the consumer's input
  tiling. The producer's output block *is* the consumer's input block; place them on
  the same fabric unit and the tensor never moves. **No collective.** Making edges
  align is exactly what the [alignment / anchoring formalism](../alignment_formalism.md)
  computes: a rigid transformation of the adjacent operators' domains of computation
  so their shared face lines up (the `Cout(0,0) → Cin(0,0)` recurrence-mapping is
  remapped onto the consumer's input face — see [anchoring](../anchoring.md)).
- **Mismatched edge** — the tilings disagree (producer splits rows, consumer needs
  columns; producer's contraction axis differs from the consumer's). The tensor must
  be **reshuffled**: a [transpose / all-to-all](dmm.md) collective sits on the edge.

So an inter-operator collective is not fundamental — it is the **price of a layout
mismatch** the compiler failed (or chose not) to align away.

## Fuse or materialize

When an edge aligns, the compiler can **fuse** the two operators: keep the
intermediate tensor in-fabric, never write it to memory, never reshuffle.

- **Elementwise** consumers (bias-add, activation, `scal`, `rot`) fuse *trivially* —
  they read one input cell and write one output cell, so they inherit the producer's
  output tiling for free. A `matmul → bias → activation` chain runs as **one** fused
  region: the collectives are only whatever the matmul's own `K`-reduction needs;
  the bias and activation add **zero** communication. This is the best case and the
  common one in DNNs.
- A consumer that **changes layout or contracts a new axis** (a second GEMM, a
  transpose, a reduction over a different axis) cannot fuse for free — it forces a
  boundary, and the compiler must decide whether to align the tiling or pay a
  collective there.

Composition is therefore a running choice between **fuse** (align, keep the edge
local) and **materialize** (place a collective on the edge).

## Worked example — a two-layer block

Take an MLP block `Z = σ(X·W₁) · W₂`, three operators chained:

```text
   X ──▶ [ GEMM₁: H = X·W₁ ] ──▶ [ σ: A = act(H) ] ──▶ [ GEMM₂: Z = A·W₂ ] ──▶ Z
                                   (elementwise)
```

- **`σ` fuses into GEMM₁.** It is elementwise, so `A`'s tiles *are* `H`'s tiles, in
  place — no edge collective. GEMM₁ and `σ` are one region.
- **The `A → GEMM₂` edge is the decision.** GEMM₂ computes
  `Z[b][g] = Σ_f A[b][f]·W₂[f][g]` — it **contracts over `f`**, the feature axis
  GEMM₁ produced. Two tilings of GEMM₁'s output give two very different edges:

  | GEMM₁ output tiling | `A → GEMM₂` edge |
  |---|---|
  | split the **feature axis `f`** across units | GEMM₂'s `Σ_f` now spans units → an **all-reduce** per output block (a DMM collective) |
  | keep each row's **features whole** on one unit (split batch `b`) | GEMM₂'s `Σ_f` is **local** to the unit → a within-tile [halo-pipelined reduction](halo-vs-collective.md), no collective |

  Same graph, same math — but one mapping pays an all-reduce on every `A → GEMM₂`
  edge and the other pays nothing. **That** is collective placement: it is decided by
  the *tiling choice on the producer*, not by GEMM₂ in isolation.

## The mapping problem

The example generalizes to two coupled decisions the compiler makes over the **whole
graph at once**:

1. **Tiling / layout** — for each operator, which axis splits and how blocks are
   shaped; and
2. **Placement** — which fabric unit ([tile → KPU → SoC](hierarchy.md)) each block
   lands on.

They are coupled because an operator's output tiling *is* the next operator's input
tiling — you cannot choose them node-by-node. A locally optimal tiling for GEMM₁
(say, split `f` for perfect PE utilization) can force a global all-reduce that a
slightly worse GEMM₁ tiling would have avoided. The compiler's objective is the
**total** edge-collective cost across the graph, each collective priced at the
[lowest hierarchy level](dmm.md) that covers its two endpoints — a global
optimization, not a greedy per-node one.

This is precisely the spatial-mapping problem the repo's DFG / RDG tooling targets:
the [DFG](../ideation-of-dfg-technology.md) is the operator graph, alignment and
anchoring are the geometric machinery for making edges flow without reshuffles, and
[pipelining schedules](../pipelining_schedules.md) time the fused regions.
Composition is where tiling (#70), the halo/collective dichotomy (#71),
uniformization (#72), and the hierarchy/DMM cost model (#73) all come together into
the compiler's actual job: **lay the graph across the fabric so that almost all
traffic stays local, and every collective that remains is both necessary and cheaply
placed.**

_Part of the [Scaling & Distribution epic](https://github.com/branes-ai/domain_flow/issues/68) (issue #74). This closes the section: [tiling](tiling.md) → [halo vs collective](halo-vs-collective.md) → [uniformization](uniformization.md) → [hierarchy](hierarchy.md) & [DMM](dmm.md) → composition._
