# trmv as a System of Uniform Recurrence Equations

The BLAS Level-2 triangular matrix-vector product overwrites $x$ with $Tx$ for a
triangular $T$. For a **lower**-triangular $T$,

$$
x_i \;:=\; \sum_{j \le i} T_{ij}\,x_j,
\qquad 0 \le i < N .
$$

`trmv` is [`gemv`](gemv.md) on a **triangular domain** — the first **non-box** index
space in the catalog. Only the entries $j \le i$ exist, so the reduction for row $i$
sweeps a *variable extent* $j = 0..i$, and the domain of computation is a triangular
prism rather than a box. (The upper-triangular case is the mirror image: constrain
$j \ge i$.) It is where the confluence DSL's equality/inequality face pinning meets a
non-rectangular region.

## The triangular domain

$$
\mathcal D = \{\,(i,j,k)\;\mid\; 0 \le i < N,\; 0 \le j \le i,\; 0 \le k < 1\,\}.
$$

The constraint $j \le i$ is the whole story: it makes each row's reduction shorter
than the last, and — crucially — it moves the reduction's terminal onto the
**diagonal** $j = i$ instead of a flat face $j = N-1$.

## Feeding $T$ and $x$

As in `gemv`, $T_{ij}$ is indexed by both domain axes and read once, so it enters on
the $(i,j)$ face of a depth-1 feed axis $k$. The input vector $x_j$ is fed the **same
way** — per cell, on the $(i,j)$ face — rather than pipelined across the rows:

$$
t(i,j,k) = t(i,j,k-1),\qquad xx(i,j,k) = xx(i,j,k-1).
$$

Why not pipeline $x$ along $+i$ as `gemv` does? On a triangular domain, column $j$
begins at the diagonal cell $(j,j)$, so a $+i$ pipeline would have to be **seeded on
a diagonal face** — one parallel to the output diagonal. Two parallel faces carrying
opposite flux admit **no linear schedule**. Feeding $x$ on the $k$ face sidesteps
this and keeps a clean linear $\tau$.

## The reduction and its diagonal exit

$$
\mathrm{acc}(i,j,k) = \mathrm{acc}(i,j-1,k) + t(i,j,k-1)\,xx(i,j,k-1),
\qquad \mathrm{acc}(i,-1,k) = 0 .
$$

The seed enters on the $j = -1$ face; for row $i$ the sum completes when $j$ reaches
its maximum $i$, so the result leaves on the **diagonal face** $i-j = 0$:

$$
\mathrm{Xout}_i = \mathrm{acc}(i,\,i,\,0).
$$

## SURE

$$
\begin{aligned}
t(i,j,k)   &= t(i,j,k-1) \\
xx(i,j,k)  &= xx(i,j,k-1) \\
\mathrm{acc}(i,j,k) &= \mathrm{acc}(i,j-1,k) \;+\; t(i,j,k-1)\,xx(i,j,k-1)
\end{aligned}
$$

Every dependence is a **constant offset**, so `trmv` is a pure **SURE** (uniform) —
the triangularity lives entirely in the *domain*, not the recurrence.

## Confluences (oriented faces)

| tensor | face | outward normal | role |
|---|---|---|---|
| `T` | $k = -1$ (the $(i,j)$ face) | $(0,0,-1)$ | lower triangle of $T$ |
| `Xin` | $k = -1$ | $(0,0,-1)$ | input vector |
| seed | $j = -1$ | $(0,-1,0)$ | reduction seed $0$ |
| `Xout` (out) | $i - j = 0$ (diagonal) | $(-1,1,0)$ | result |

## Schedule

The diagonal output face has outward normal $(-1,1,0)$, so its outflux
$\tau\!\cdot\!n > 0$ requires $\tau_j > \tau_i$. The canonical $\tau = [1,2,1]$
satisfies this (`minSlack` $\tau\!\cdot\!\theta = 2$), as does the free schedule. The
box-operator default $\tau = [1,1,1]$ is **rejected** — it puts *zero* flux on the
diagonal — which is a concrete illustration of how a non-rectangular domain
constrains the legal schedules. (Reversing further, $\tau_j < \tau_i$, is rejected
the same way.)

## Memory cardinality

The reduction keeps only a running accumulator plus the two feeds live per lane, so
the peak live set is small ($O(1)$ per active reduction). The liveness analysis
agrees exactly with an eviction run (`test_trmv_sure`).

## Executable

The spec is the single source of truth; the test parses this file.

```text
dfactl --sure docs/SURE/trmv.sure                 # free schedule
dfactl --sure docs/SURE/trmv.sure --tau 1,2,1     # canonical linear schedule
```

With the bundled lower-triangular $T$ and $x = [1,2,3,4]$:
$Tx = [2,\,11,\,38,\,100]$.
