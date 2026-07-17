# Neighbour pivoting — uniformizing the pivot

[LU with partial pivoting](lu.md) is the honest direct solver, but its pivoting is the
one operation that a uniform recurrence cannot express. This page shows why — and how
**neighbour pivoting** recovers a genuine SURE by trading global pivot selection for
nearest-neighbour compare-exchanges. It is the pivoting counterpart of the
[reduced dependency graph](reduced_dependency_graph.md)'s central theme: uniformization
turns an affine broadcast into translation vectors.

## Why partial pivoting is a SARE

At elimination step `k`, partial pivoting does two things:

1. **select** the pivot — `argmax_{i ≥ k} |a(i,k)|`, a reduction over the *whole*
   column;
2. **interchange** — move row `argmax` to row `k`, a data-dependent permutation.

The selection reads the entire column, and the interchange moves a row an *arbitrary*
distance — both are **affine** dependences that project onto row/column `k` (a
broadcast). That is exactly the affine arc that makes [LU](lu.md) a **SARE**, and a
permutation chosen at run time is not a static map at all. Partial-pivoting `PLU` is
therefore inexpressible as a uniform system — the open part of the LU story.

## Neighbour pivoting — only adjacent rows

Neighbour pivoting (the systolic/pairwise pivoting strategy) restricts **every**
comparison and interchange to **adjacent rows**. Rather than find the global maximum and
teleport it to the diagonal, it performs a **bubble pass**: sweep the column comparing
neighbours `(i-1, i)`, and where the lower row has the larger magnitude, exchange them.
One pass carries the largest-magnitude entry to the pivot position; the loser of each
comparison settles one step back. Every read is now to an *immediate neighbour* — a
constant offset — so the whole operation is **uniform**.

This is [`iamax`](iamax.md)'s nearest-neighbour argmax carry, extended to also
**exchange**. Carry the running max-magnitude value `piv` (and its original row index
`pidx`) down the column; emit the loser `lo` at each cell:

$$
\begin{aligned}
piv(i) &= \operatorname{select}\bigl(\,gt(|x(i)|,\,|piv(i-1)|),\; x(i),\; piv(i-1)\bigr),\\
pidx(i) &= \operatorname{select}\bigl(\,gt(|x(i)|,\,|piv(i-1)|),\; \iota(i),\; pidx(i-1)\bigr),\\
lo(i) &= \operatorname{select}\bigl(\,gt(|x(i)|,\,|piv(i-1)|),\; piv(i-1),\; x(i)\bigr),
\end{aligned}
$$

with the exact selection built-ins `gt(a,b) = [a > b]` and `select(c,x,y) = x if c else
y` (as in `iamax`), and `ι(i)` the row index fed as data. The pivot leaves at the bottom
`i = N-1`; the settled losers `lo` give the compare-exchanged column — the permuted
column is `[ lo(1), …, lo(N-1), piv(N-1) ]`.

```text
system ((i,j) | 0 <= i < N, 0 <= j < 2) {
    piv(i,j)  = select(gt(abs(x(i,j-1)), abs(piv(i-1,j))), x(i,j-1),  piv(i-1,j));
    pidx(i,j) = select(gt(abs(x(i,j-1)), abs(piv(i-1,j))), xi(i,j-1), pidx(i-1,j));
    lo(i,j)   = select(gt(abs(x(i,j-1)), abs(piv(i-1,j))), piv(i-1,j), x(i,j-1));
}
```

Executable spec: `docs/SURE/lu_neighbor.sure`. For the column `x = [1, -5, 3, -2]` the
pass moves `|-5|` (from row 1) to the pivot position and yields the permuted column
`[1, 3, -2, -5]` — verified by `test_lu_neighbor_sure`.

## The RDG: all translations, no broadcast

Because `piv` reads only its neighbour `piv(i-1)` and the fed value `x(i,j-1)`, **every
arc is a translation vector** — the reduced dependency graph is a pure **SURE**, with no
affine arc:

<div class="rdg" data-src="rdg/lu_neighbor.json" data-height="620"></div>

Put this beside [LU](lu.md)'s RDG — one node with three **affine** pivot arcs
(`(i,j,k) ↦ (k,k,k-1)` and friends). That is uniformization made visible: neighbour
pivoting replaces the pivot broadcast with nearest-neighbour compare-exchanges, so the
selection admits a single linear schedule **`τ = [1,1]`** and maps to a fixed local
stencil on the KPU fabric — no global column read, no long-range interchange.

<div class="schedule-anim" data-src="schedules/lu_neighbor-linear.json" data-height="360"></div>

## The trade

Neighbour pivoting is **not** partial pivoting. A single bubble pass brings *a* large
element to the pivot — not necessarily the global maximum — so the growth factor is
bounded but larger than partial pivoting's. This is the standard trade for a systolic
array: weaker numerical guarantees in exchange for nearest-neighbour communication and a
uniform schedule. It is why partial-pivoting `PLU` (#42) is documented as a **SARE that a
static SURE cannot express**, while neighbour pivoting is a genuine **SURE** — the
pivoting that a domain-flow fabric can actually run.
