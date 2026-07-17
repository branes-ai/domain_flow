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
system ((i,j,k) | 0 <= i < N, 0 <= j < N, 0 <= k < K) {
    acc(i,j,k) = acc(i,j-1,k) + am(i,j,k) * xs(j,N-1,k-1);              // reduce Σ_j A(i,j) x^{k-1}_j
    xs(i,j,k)  = xs(i,N-1,k-1) + (bb(i,j,k) - acc(i,j,k)) / diag(i,j,k); // residual update, correct at j=N-1
}
```

Executable spec: `docs/SURE/stationary.sure` (`am`, `bb`, `diag` carry `A`, `b`, and the
diagonal `A_ii`). Reading `x^{k-1}` across every row is the matrix-vector **gather**: the
taps `xs(i,N-1,k-1)` and `xs(j,N-1,k-1)` project onto the finished-solution face `j=N-1`,
so they are **affine** — Jacobi is a **SARE**, but a benign *forward* one, since a sweep
reads only sweep `k-1`. For the bundled strongly diagonally dominant system
(`A = 10·I + (ones−I)`, `b = 12·1`), `K = 8` sweeps converge to `x = [1,1,1]`.

## Schedule — free only, and why that is the point

Like the [triangular solve](trsolve.md), each row's reduction and its residual divide
**fuse at the finishing cell** `(i, N-1, k)`: `xs` there reads the completed `acc(i,N-1,k)`
at the *same* lattice point — a zero-slack self-dependence no linear `τ` can order. So the
solver is **free-schedule only**, and the free schedule is exactly the picture we want:

- **within a sweep** (fixed `k`), all `N` rows finish together — a *wide* wavefront, the
  visible signature of Jacobi's parallelism;
- **across sweeps**, the `k-1` dependence serializes — one sweep-block after another.

## Contrast — Gauss–Seidel serializes the sweep

Gauss–Seidel uses the *newest* components as soon as they are available:

$$
x^{k}_i \;=\; \frac{1}{A_{ii}}\Bigl( b_i - \sum_{j<i} A_{ij}\,x^{k}_j - \sum_{j>i} A_{ij}\,x^{k-1}_j \Bigr).
$$

The `j<i` sum reads **this sweep's** `x^{k}_j`, so row `i` cannot start until rows
`0..i-1` of the *same* sweep have finished — an **intra-sweep triangular wavefront**, the
[`trsolve`](trsolve.md) dependence pattern layered inside each iteration. Gauss–Seidel
usually converges in fewer sweeps (it reuses fresh data), but each sweep is *serial across
rows* rather than parallel. In RDG/schedule terms the two methods differ by exactly one
tap direction — `x^{k-1}` (Jacobi, parallel sweep) versus `x^{k}` for `j<i` (Gauss–Seidel,
triangular sweep) — a crisp illustration that the same operator body yields very different
concurrency depending on which iterate a dependence reads.

## The RDG

Two affine arcs — the `xs → acc` and `xs → xs` gathers that read the previous iterate
across all rows — mark Jacobi a SARE; the rest (the reduction chain, the held `A`, the
propagated `b`/diagonal) are translation vectors.

<div class="rdg" data-src="rdg/stationary.json" data-height="620"></div>

<div class="schedule-anim" data-src="schedules/stationary-free.json" data-height="380"></div>
