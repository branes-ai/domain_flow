# LU factorization as a recurrence system

LU factorization — Gaussian elimination `A = L·U` with `L` unit-lower-triangular and
`U` upper-triangular — is the workhorse **direct solver**. This page derives it in two
steps: first the classic right-looking elimination as a **SARE** (issue #42, step 1),
then a **uniformization** of the pivot broadcast into a nearest-neighbour propagation
(step 2), whose free schedule makes the propagation constraints visible.

:::caution
The executable here factors **without pivoting**. Partial pivoting is a
*data-dependent* row permutation and cannot be a static SURE — see
[Partial pivoting](#partial-pivoting-is-not-a-sure) below.
:::

## The Schur elimination as a recurrence

Right-looking (Doolittle) LU eliminates one column per step. Let `a(i,j,k)` be entry
`(i,j)` after eliminating with pivots `0..k`. Step `k` applies the rank-1 Schur update
to the trailing submatrix:

$$
a(i,j,k) \;=\; a(i,j,k-1) \;-\; \underbrace{\frac{a(i,k,k-1)}{a(k,k,k-1)}}_{l(i,k)\ \text{multiplier}}\; \underbrace{a(k,j,k-1)}_{u(k,j)\ \text{pivot row}} .
$$

Each cell `(i,j)` is updated for `k = 0 .. min(i,j)-1`, over the strict trailing domain
`k ≤ i-1, k ≤ j-1`.

## A SARE — but with a linear schedule

The three operand taps are **affine**, not uniform: the pivot `a(k,k,k-1)`, the
multiplier column `a(i,k,k-1)`, and the pivot row `a(k,j,k-1)` are read via index maps
that *project onto row/column `k`* — a broadcast to the whole trailing submatrix. So LU
is a **SARE**, like the [MGS QR](QR_decomposition.md).

But here the affine dependence is **benign**. Every consumer `(i,j,k)` sits strictly
below-and-right of the pivot `(k,k,k-1)` (the domain guarantees `i,j > k`), so the
dependence vector `(i-k, j-k, 1)` is always *forward*. Unlike QR's reduction-broadcast,
a **linear schedule exists**: `τ = [1,1,2]` is legal (and so is the free schedule).
This is why LU tiles far more readily than QR.

<div class="schedule-anim" data-src="schedules/lu-free.json" data-height="440" data-fps="4"></div>

## Extracting L and U

Each cell finalizes at `k = min(i,j)-1`, so the factors leave on **super-diagonal**
faces:

- `U(i,j)` for `j ≥ i` leaves at `k = i-1` (the face `i-k = 1`);
- `L(i,j)` for `i > j` is the multiplier `a(i,j,j-1)/a(j,j,j-1)`, on `j-k = 1`.

The **first row of `U`** (`i=0`) is `A`'s first row and the **first column of `L`**
(`j=0`) is `A[:,0]/A[0,0]` — both read from the `k=-1` seed halo (they are never
eliminated). The regression test reconstructs the full `L`, `U` and verifies
`L·U = A`.

## Partial pivoting is not a SURE

Partial pivoting selects, at each step, the row with the largest `|a(i,k)|` (a per-
column [`iamax`](iamax.md)) and swaps it to the pivot. That
choice is made *at runtime from the values*, and the resulting **row permutation `P`**
depends on the data. A SURE's index space is fixed at compile time, so it cannot
express a data-dependent permutation — `A = PLU` with real pivoting is outside the
model. (Numerically, the spec above requires the leading minors to be non-singular, as
its bundled matrix is.)

## Step 2 — uniformizing the pivot into a propagation

The pivot broadcast is exactly the affine dependence [uniformization](../scaling/uniformization.md)
turns into a pipeline. For a **single** elimination step (`k = 0`) the pivot row and
multiplier column *are the first row/column of the input `A`*, so they can seed
nearest-neighbour propagations legally (input confluences read tensors):

$$
\begin{aligned}
pr(i,j,k) &= pr(i-1,j,k) &&\text{pivot row } A(0,j)\ \text{propagates down } +i\\
mc(i,j,k) &= mc(i,j-1,k) &&\text{multiplier col } A(i,0)\ \text{propagates across } +j\\
r(i,j,k)  &= r(i,j,k-1) - g\,\bigl(mc(i,j-1,k)/pv(i-1,j,k)\bigr)\,pr(i-1,j,k)
\end{aligned}
$$

Every dependence is now a constant offset — a **pure SURE** (`τ = [1,1,1]`), applying
the rank-1 Schur term exactly once via [`ger`](ger.md)'s inject → add → drain. This is
`docs/SURE/lu_propagate.sure`.

**Why only one step.** For `k > 0` the pivot row `a(k,:,k-1)` is a *computed* value, and
seeding a propagation from a recurrence variable is an **internal confluence** the DSL
does not have — `dfactl` rejects it with *"input expressions may not read recurrence
variables"*. This is the same gap that keeps the MGS QR a SARE. So the full multi-step
LU stays a SARE; only the single step uniformizes.

**What the free schedule shows.** Because the pivot must hop `i` rows down and the
multiplier `j` columns across, cell `(i,j)` cannot fire until `t ≈ max(i,j)` — the
broadcast's "everyone at once" becomes a diagonal **propagation wavefront**. That
staircase is the cost the propagation trades for uniformity (it now tiles with halo
exchange). Watch it below:

<div class="schedule-anim" data-src="schedules/lu_propagate-free.json" data-height="440" data-fps="4"></div>

## Executable

```text
dfactl --sure docs/SURE/lu.sure            --tau 1,1,2    # the SARE
dfactl --sure docs/SURE/lu_propagate.sure  --tau 1,1,1    # the propagation SURE
```

For `A = [[4,3,2],[8,7,5],[2,3,4]]`: `L·U = A` with
`U = [[4,3,2],[0,1,1],[0,0,1.5]]`, `L = [[1,0,0],[2,1,0],[0.5,1.5,1]]`; and one Schur
step gives the complement `[[0,0,0],[0,1,1],[0,1.5,3]]`.
