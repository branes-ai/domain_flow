# The Reduced Dependency Graph

A System of Uniform or Affine Recurrence Equations (SURE/SARE) defines a computation
over an index space — one value per lattice point, per variable. Drawn out in full,
its **dependence graph** has a node for *every* computed value and an edge for every
read; it grows without bound as the problem grows. That expanded graph is unwieldy and,
worse, it hides the thing we actually care about: the *structure* of the recurrence is
finite and repeats at every point.

The **Reduced Dependency Graph (RDG)** is that finite structure. It collapses the
expanded graph two ways:

- **one node per recurrence variable** — every lattice point of variable `a` becomes
  the single node `a`;
- **one arc per dependence** — every instance of "this equation reads that operand"
  becomes a single labeled arc.

The RDG is the object a systolic-array synthesizer reasons over. It is
**schedule-independent** and **size-independent** — a property of the recurrence
*form* alone — and it makes the SURE/SARE distinction visible at a glance.

## Nodes and arcs

Take an equation

$$
v(p) \;=\; f\bigl(\dots,\; s(A\,p + b),\; \dots\bigr).
$$

For each value it reads — each **tap** — the RDG adds an arc `s → v`. The arc is
labeled with the operand's **affine dependency map** `p ↦ A p + b`: the producer point
that a consumer at `p` reads. A variable that reads its own earlier values contributes
a **self-loop**.

### Uniform arcs — translation vectors

When `A = I`, the operand index is `p + b`: a **constant offset** from the consumer,
independent of `p`. The arc is a plain **translation vector**

$$
\theta \;=\; p - (I\,p + b) \;=\; -b,
$$

a nearest-neighbour wire. For example `a(i,j,k) = a(i,j-1,k)` reads the operand at
`(i,j-1,k)`, so `b = (0,-1,0)` and

$$
\theta \;=\; (0,1,0)^{\mathsf T},
$$

a self-loop on `a` pointing one step along `+j`. A system in which **every** arc is a
translation is a genuine **SURE** — it wires up as a fixed local stencil, the same at
every point (Karp–Miller–Winograd, 1967).

### Affine arcs — projections and broadcasts

When `A ≠ I`, the operand index is an **affine function of `p`** — a projection or
permutation. The dependence vector `p − (A p + b)` then **grows with `p`**: physically
a fan-out bus, not a local wire. A single affine arc marks the whole system a **SARE**.
The canonical example is a **pivot broadcast**: [LU](lu.md)'s Schur update reads the
pivot `a(k,k,k-1)`, whose map projects

$$
(i,j,k)\;\longmapsto\;(k,k,k-1),
$$

i.e. `A` has every row `(0,0,1)` and `b = (0,0,-1)`. Every cell of the trailing
submatrix reads that one pivot — an affine dependence the RDG draws as a distinct,
dashed arc.

## Worked example — `gemv` is a SURE

`gemv` (`y := βy + αAx`) projects and pipelines every operand, so its RDG is
**all-uniform**: six variable nodes (`a`, `xx`, `alpha`, `beta`, `yin`, `acc`) wired
by translation vectors only. The reduction `acc(i,j,k) = acc(i,j-1,k) + …` is the
`acc→acc` self-loop `θ = (0,1,0)`; `xx→acc` carries `θ = (1,0,0)` (the pipelined `x`);
`a→acc` and `alpha→acc` carry `θ = (0,0,1)` (the feed axis). No affine arc — a genuine
SURE, schedulable by a single linear `τ`.

<div class="rdg" data-src="rdg/gemv.json" data-height="440"></div>

## Worked example — `lu` is a SARE

[LU](lu.md) is the same picture with the pivot broadcast made explicit. Its single
variable `a` carries the uniform Schur self-loop `θ = (0,0,1)` (the `k-1` tap) **plus
three affine self-loops** — the multiplier column `a(i,k,k-1)`, the pivot row
`a(k,j,k-1)`, and the pivot `a(k,k,k-1)`. Those three affine arcs are exactly what make
LU a SARE, and they are the arcs a **neighbour-pivoting** reformulation would replace
with translation vectors (uniformization). The RDG is where that transformation becomes
legible — the broadcast and its uniformized propagation drawn side by side.

<div class="rdg" data-src="rdg/lu.json" data-height="440"></div>

## Reading the graph

- **Nodes** are recurrence variables; **self-loops** are a variable reading its own
  earlier values.
- **Solid arcs** are uniform (translation `θ`, nearest-neighbour); **dashed arcs** are
  affine (a map `p ↦ Ap+b`, a projection/broadcast).
- The badge reads **SURE** when every arc is a translation, **SARE** when any arc is
  affine — the single most important structural fact about a recurrence system, since
  a SARE needs uniformization (or a free schedule) where a SURE admits a linear one.

---

Emitted by `dfactl --sure <op>.sure --emit-rdg <out.json>` and rendered by the
docs-site RDG viewer. Every catalog operator carries its RDG in the operator's
*Theory* derivation.
