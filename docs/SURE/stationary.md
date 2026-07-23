# Stationary iteration — Jacobi and Gauss–Seidel

Stationary iterative solvers attack `Ax = b` by repeating a cheap fixed-point sweep
until the iterate converges. They are the first *iterative* entries in the catalog: where
a direct solver runs a bounded elimination, an iterative solver is a **recurrence over the
iteration index `k`** whose body is a `gemv` (the residual) plus an `axpy` (the update).
This page derives **Jacobi** as the executable spec and contrasts it with **Gauss–Seidel**
— a clean difference in *schedule structure*.

## Jacobi — the residual sweep

Splitting `A = D + (L+U)`, Jacobi is `x^{k} = D⁻¹(b − (L+U)x^{k-1})`. In **residual
form** — which avoids having to exclude the diagonal from the sum — each sweep is

$$
x^{k}_i \;=\; x^{k-1}_i \;+\; \frac{1}{A_{ii}}\Bigl( b_i - \sum_j A_{ij}\,x^{k-1}_j \Bigr).
$$

The sum `Σ_j A_ij x^{k-1}_j` is a `gemv`; the outer `x^{k-1}_i + …/A_ii` is an `axpy`.
Crucially, **every `x^k_i` reads only the previous sweep `x^{k-1}`** — the rows of a
sweep are independent, so Jacobi is *fully parallel within a sweep*.

```text
system ((i,j,k) | 0 <= i < N, 0 <= j <= N, 0 <= k < K) {   // j runs one plane PAST the reduction
    acc(i,j,k) = acc(i,j-1,k) + am(i,j,k-1) * xs(j,N,k-1);                  // reduce Σ_j A(i,j) x^{k-1}_j
    xs(i,j,k)  = xs(i,j,k-1) + (bb(i,j-1,k) - acc(i,j-1,k)) / diag(i,j-1,k); // residual update on the j=N plane
}
```

Executable spec: `docs/SURE/stationary.sure` (`am`, `bb`, `diag` carry `A`, `b`, and the
diagonal `A_ii`). Reading `x^{k-1}` across every row is the matrix-vector **gather**: the
tap `xs(j,N,k-1)` reads unknown `j`'s finished value for *every* row `i`, an **affine**
`i↦j` transpose. That one arc makes Jacobi a **SARE** — but a benign *forward* one, since a
sweep reads only sweep `k-1`. For the bundled strongly diagonally dominant system
(`A = 10·I + (ones−I)`, `b = 12·1`), `K = 8` sweeps converge to `x = [1,1,1]`.

### Schedule — breaking the [0,0,0] fusion

Naively, each row's reduction and its residual divide would **fuse at the finishing cell**
`(i, N-1, k)`: `xs` there reads the completed `acc(i,N-1,k)` at the *same* lattice point — a
`[0,0,0]` self-dependence no linear `τ` can order (two computations, one point, one time),
exactly the wall the [triangular solve](trsolve.md) hits.

We break it by moving the update **one plane up**. The reduction runs `j = 0…N-1` and
finishes `acc(i,N-1,k)`; the residual divide is computed on a fresh plane `j = N`, where it
reads that completed sum as `acc(i,j-1,k)` — a translation `[0,+1,0]` **with slack**. The
remaining multiply-accumulate operands are read one step back along their carries (`am` at
`k-1`, held; `b`/`diag` at `j-1`, propagated) — the gemv trick — so *every* dependence now
has slack ≥ 1. The result is **both schedulable ways**:

- the **free** schedule is the parallel sweep: within a sweep (fixed `k`) all `N` rows finish
  together — a *wide* wavefront, Jacobi's parallelism — then the `k-1` dependence serializes
  sweep after sweep;
- a **linear** schedule now exists too — `τ = [-1, 1, 2N]` sweeps a plane through the lattice
  (`τ_i < 0` because the `j=N` update plane's `i=N` halo is an influx face, so rows fire
  *staggered* rather than all at once). That a linear `τ` exists **is** the payoff of the
  plane-shift.

### The RDG

**One** affine arc — the `xs → acc` gather that reads the previous iterate across all rows —
marks Jacobi a SARE; everything else (the reduction chain, the held `A`, the propagated
`b`/diagonal, and now the `xs` self-carry across sweeps) is a translation vector. The
plane-shift bought this: the fused form had *two* affine arcs (the `xs → xs` self-broadcast
became a uniform `k`-carry) and no linear schedule at all.

<div class="rdg" data-src="rdg/stationary.json" data-height="620"></div>

Below, the same Jacobi sweep under both schedules. **Free**: each sweep's rows fire together —
the wide wavefront is the parallelism, and sweep blocks march along `k`.

<div class="schedule-anim" data-src="schedules/stationary-free.json" data-height="380"></div>

**Linear** `τ = [-1, 1, 2N]`: the wavefront is now a plane sweeping the lattice — the visible
proof that the `[0,0,0]` fusion is gone. (`τ_i < 0` staggers the rows, so it reads less like a
single parallel sweep than the free schedule and more like a systolic front.)

<div class="schedule-anim" data-src="schedules/stationary-linear.json" data-height="380"></div>

## Gauss–Seidel — the newest components

Gauss–Seidel uses the *newest* components as soon as they are available:

$$
x^{k}_i \;=\; \frac{1}{A_{ii}}\Bigl( b_i - \sum_{j<i} A_{ij}\,x^{k}_j - \sum_{j>i} A_{ij}\,x^{k-1}_j \Bigr).
$$

The `j<i` sum reads **this sweep's** `x^{k}_j`, so row `i` cannot start until rows
`0..i-1` of the *same* sweep have finished — an **intra-sweep triangular wavefront**, the
[`trsolve`](trsolve.md) dependence pattern layered inside each iteration. In RDG/schedule
terms the two methods differ by exactly one tap direction — `x^{k-1}` (Jacobi, parallel
sweep) versus `x^{k}` for `j<i` (Gauss–Seidel, triangular sweep) — a crisp illustration that
the same operator body yields very different concurrency depending on which iterate a
dependence reads.

### Schedule — the triangular sweep

Gauss–Seidel usually converges in fewer sweeps (it reuses fresh data), but each sweep is
*serial across rows* rather than parallel. The lower-triangular `x^{k}_j` (`j<i`) dependence
is exactly [`trsolve`](trsolve.md)'s substitution wavefront, and — like `trsolve` — it fuses
the reduction and the divide at the diagonal, a zero-slack self-dependence: Gauss–Seidel is
**free-schedule only**. The free schedule is the diagonal substitution front, one row after
another, nested inside each `k` sweep.

### The RDG

Expressing the `j<i` / `j>i` split needs each half of the sum on its **own sub-domain** — the
executable spec (`docs/SURE/gauss_seidel.sure`) uses **per-equation domains**:

```text
accL(i,j,k | j < i)        = accL(i,j-1,k) + A(i,j) * xs(j,N,k);    // lower, reads THIS sweep
accU(i,j,k | i < j, j < N) = accU(i,j+1,k) + A(i,j) * xs(j,N,k-1);  // upper, reads LAST sweep
xs(i,j,k) = ( b_i - accL(i,i-1,k) - accU(i,i+1,k) ) / A(i,i);       // solve on the j=N plane
```

so the `k`-vs-`k-1` choice is *structural* (which variable) rather than a per-point test. The
RDG is a **SARE with four affine arcs** — the two matrix-vector gathers (`xs→accL`, `xs→accU`)
and the two diagonal-adjacent solve reads (`accL→xs` at `j=i-1`, `accU→xs` at `j=i+1`).

<div class="rdg" data-src="rdg/gauss_seidel.json" data-height="620"></div>

The free schedule is the **triangular substitution front**: row `i`'s lower sum reads
`x^k_j` for `j<i`, produced by the solves of earlier rows *this* sweep, so the rows fire in
order — nested inside each `k` sweep. Contrast the wide, parallel Jacobi wavefront above.

<div class="schedule-anim" data-src="schedules/gauss_seidel-free.json" data-height="380"></div>

## Making them systolic

Both specs above are **SAREs**: their matrix-vector reads reach across rows to a column's
solution (an `i↦j` transpose), an *affine* arc. For a **regular spatial mapping** we want every
dependence to be a constant displacement — a genuine **SURE**. That transformation (pipeline the
state, split the reduction) is achievable for *both* methods, and is worked through — with the
RDGs and animations of the resulting all-uniform forms — on its own page:
[Systolic SURE transforms](systolic-transforms.md).
