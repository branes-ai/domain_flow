# Tiling the index space: blocked matmul

A KPU tile has a **fixed** processing-element array; a matmul's index space is the
cube `(i,j,k)`, `0 ≤ i < M`, `0 ≤ j < N`, `0 ≤ k < K`, whose volume grows with the
problem. When the cube no longer fits the fabric it must be **partitioned into
tile-sized blocks**, each block mapped to a tile and scheduled — across tiles and
within a tile. This page works that partition on matmul, the clean case, and shows
the payoff: because matmul is **uniform**, every dependence that a tile cut severs
is a **halo**, never a collective.

## The recurrence does not change — that is the whole point

Tiling is often presented as a *rewrite* of the algorithm ("blocked matmul"). It is
not a different recurrence. [`matmul.sure`](../theory/matmul.md) is

```text
a(i,j,k) = a(i,j-1,k);                            // A streams along +j
b(i,j,k) = b(i-1,j,k);                            // B streams along +i
c(i,j,k) = c(i,j,k-1) + a(i,j-1,k) * b(i-1,j,k);  // C accumulates along +k
```

Every dependence is a **constant** offset — `(0,-1,0)`, `(-1,0,0)`, `(0,0,-1)` —
independent of *where* in the cube you evaluate it. So if you cut the cube into
`T×T×T` blocks and re-index a point

```text
(i, j, k)  ↦  ( tile (I,J,K),  offset (ii,jj,kk) ),   i = T·I + ii,  …
```

the recurrence inside every block is *identical* — the same three wires — and it is
identical to the un-tiled recurrence. **Uniformity is exactly the property that
makes the index space partitionable without touching the algorithm.** "Blocked
matmul" is a *schedule* of `matmul.sure`, not a new spec. (Contrast an operator
with an affine tap, where a cut severs a domain-spanning edge — see
[uniformization](uniformization.md).)

## The two-level schedule

Partitioning splits the schedule into two nested levels:

- **Within a tile** — the systolic wavefront over the offsets `(ii,jj,kk)`: the
  canonical `τ = [1,1,1]` sweeps diagonal planes `ii+jj+kk = const` through the
  block. This is the same wavefront the [catalog](../theory/matmul.md) already
  animates, now confined to a `T×T×T` sub-cube.
- **Across tiles** — a schedule over the block grid `(I,J,K)`. And here matmul is
  **self-similar**: the block-level computation is *itself* a matmul,

  ```text
  C[I][J] = Σ_K  A[I][K] · B[K][J]        // blocks as "scalars", tile-matmul as "×"
  ```

  a multiply-accumulate along the `+K` block axis — the same shape as the inner
  `c(…,k) = c(…,k-1) + a·b`. The outer loop nest and the inner array run the *same*
  recurrence at two granularities.

### Worked example — 4×4 as 2×2 blocks (`T = 2`)

With `A`, `B` cut into 2×2 blocks, the top-left result block accumulates over the
`K`-blocks `0,1`:

```text
C00 = A00·B00 + A01·B10
    = [1 2; 3 4]·[1 0; 0 1] + [0 1; 1 0]·[1 1; 0 1]
    = [1 2; 3 4]           + [0 1; 1 1]
    = [1 3; 4 5]
```

`C00` reads the **`I=0` row of A-blocks** and the **`J=0` column of B-blocks`**,
summed across `K`. That sum *is* the `c`-along-`+k` accumulation, lifted to block
granularity: each `K`-block contributes a partial `C00`, handed to the next.

## What crosses a tile boundary: three halos, no collective

Cut the cube on all three axes and ask, for each severed dependence, *what does a
tile need from its neighbour?* Because every offset is `±1`, the answer is always
"one boundary face from the adjacent block" — a halo:

| dependence | offset | at a tile face it becomes | from which neighbour |
|---|---|---|---|
| `a(i,j,k) = a(i,j-1,k)` (A stream) | `(0,−1,0)` | the A-values on the block's `+j` face flow into the next `J`-block | west (`−j`) neighbour |
| `b(i,j,k) = b(i-1,j,k)` (B stream) | `(−1,0,0)` | the B-values on the `+i` face flow into the next `I`-block | north (`−i`) neighbour |
| `c(…,k) = c(…,k-1) + …` (C accum) | `(0,0,−1)` | the **partial accumulator** on the `+k` face flows into the next `K`-block | previous (`−k`) `K`-block |

All three are **nearest-neighbour, `O(T²)` surface exchanges** — halos. None is a
reduction fanned out to the whole grid; none needs the Distributed Memory Machine's
collectives. The `C`-accumulation across `K`-blocks *looks* like a reduction, but it
is a **pipelined** one: each `K`-block passes its partial to the single next block,
a `±1` hand-off, not an all-reduce. **Matmul tiles linearly.** This is the
concrete, best-case instance of the [halo-vs-collective](index.md) dichotomy — and
the baseline against which the affine operators are measured.

## The partition, animated

The cube below is `matmul` at 15³, partitioned into `T = 5` blocks — a `3×3×3`
grid of 27 tiles. Colour encodes the **tile block**; the boxes are the tile
boundaries; the bright white wavefront is the `τ = [1,1,1]` schedule sweeping
diagonally *across* the blocks. Watch the wavefront cross a tile boundary: that
crossing is a halo exchange — a thin face of data handed to the neighbour, nothing
more.

<div class="schedule-anim" data-src="schedules/matmul-linear.json" data-tile="5" data-height="480" data-fps="6"></div>

_Part of the [Scaling & Distribution epic](https://github.com/branes-ai/domain_flow/issues/68) (issue #70). Next: [halo vs collective](index.md) worked out across the catalog, and the [memory & communication hierarchy](index.md)._
