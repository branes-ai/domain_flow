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
2. **Change the algorithm so there is no reduction to broadcast.** The
   **Gentleman–Kung Givens-rotation QR array** computes QR by a wavefront of
   Givens rotations — nearest-neighbour, no reduction. This is executable *today*
   (below), and it is the right target for uniformization.

## The Givens QR — executable, verified, near-uniform

`docs/SURE/qr_givens.sure` merges each row of `A` into the
accumulating upper-triangular `R` by a sweep of Givens rotations over `(i,p,q)`,
`p ≤ q` (row `i`, pivot `p`, updated column `q`):

```text
r(i,p,q) = (rold·R + ain·a) / √(rold² + ain² + ε)     // c·R + s·a
a(i,p,q) = (rold·a − ain·R) / √(rold² + ain² + ε)     // −s·R + c·a
```

with `rold = r(i-1,p,p)`, `ain = a(i,p-1,p)`. It produces the exact factor for the
classic 3×3 (`R = [[14,21,-14],[0,175,-70],[0,0,35]]`) and satisfies the sign-robust
invariant `RᵀR = AᵀA` on general/tall inputs — see `test_qr_givens_sure`.

Where does it sit on the uniform↔affine line? The two diagonal taps `r(i-1,p,p)`
and `a(i,p-1,p)` read the **pivot column `q = p`**, so they are still **affine** —
this remains a SARE. But the difference from MGS is decisive:

| | MGS-QR (`qr.sure`) | Givens-QR (`qr_givens.sure`) |
|---|---|---|
| affine dependence | `srp(M-1,…)` — a **reduction result** (`Σ_i`) broadcast to all `i` | `r(i-1,p,p)` — a **single diagonal value** broadcast along the row `+q` |
| at a tile boundary | an **all-reduce across `i`** — global collective | a **nearest-neighbour propagation along `q`** — a halo |
| to reach a pure SURE | remove the reduction *and* pipeline it | just pipeline the diagonal along `+q` |

So Givens's affine-ness is a **pure diagonal broadcast with no reduction** — a
propagation the DSL merely cannot *seed* yet (the [internal-confluence](#two-ways-forward)
extension would let `c(i,p,q) = c(i,p,q-1)` be seeded at `q = p` from the computed
rotation, making it a true SURE). It already **tiles with local communication**;
MGS does not.

The animation below tiles the `(i,p,q)` domain (`T = 4`, a `3×3×3` grid) and draws
the cross-tile dependency edges of the firing wavefront: **green** where a dependence
crosses to the *nearest* tile (a halo), **red** where it jumps *farther* (a
collective). Givens lights up **mostly green** — the near-uniform structure — with a
thin ribbon of **red** along the two affine diagonal taps `r(i-1,p,p)` and
`a(i,p-1,p)`, the row-broadcast of the rotation that a true SURE would pipeline
away. Compare the
[tiled matmul](tiling.md): pure green, **zero** red.

<div class="schedule-anim" data-src="schedules/qr_givens-free.json" data-tile="4" data-height="460" data-fps="4"></div>

## Why this matters for scaling

The uniform-vs-affine line is not a purity contest — it is the **tileability**
line. `axpy` and `matmul` uniformize (or already are uniform), so they tile with
halo exchange and scale across KPUs. MGS-QR does not, so at scale its `r_kj`
broadcasts become **all-reduce/broadcast collectives** on the Distributed Memory
Machine — the expensive path — *unless* it is first recast to the uniform Givens
array. Uniformization is thus the compiler's lever between "this tiles" and "this
needs the DMM."

_Part of the [Scaling & Distribution epic](https://github.com/branes-ai/domain_flow/issues/68) (issue #72)._
