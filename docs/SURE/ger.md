# ger as a System of Uniform Recurrence Equations

The BLAS Level-2 rank-1 update is

$$
A \;\mathrel{+}=\; \alpha\,x\,y^{\mathsf T},
\qquad
A_{ij} \;:=\; A_{ij} \;+\; \alpha\,x_i\,y_j,
\quad 0 \le i < M,\; 0 \le j < N .
$$

`ger` is the **outer-product counterpart** to [`gemv`](gemv.md). Where `gemv`
*contracts* a matrix and a vector into a vector (an inner product, a reduction),
`ger` *expands* two vectors into a matrix (an outer product, no reduction). Every
output $A_{ij}$ is independent — it is a **2-D fully-parallel** map, the natural
generalization of [`axpy`](axpy.md) to a matrix. It is the rank-1 building block
reused by LU's Schur update and by `syr`/`syr2`.

## The update as a fully-parallel map

There is no accumulation: each cell is a single fused multiply-add of the incoming
matrix, the scalar, and one element each of $x$ and $y$:

$$
A_{ij} \;:=\; A_{ij} + \alpha\,x_i\,y_j .
$$

$x_i$ is needed at every column $j$; $y_j$ at every row $i$. So the two vectors
**broadcast** across the domain — $x$ along $+j$, $y$ along $+i$ — by the
project-and-pipeline idiom (never a literal in a recurrence body):

$$
xx(i,j,k) = xx(i,j-1,k),\qquad yy(i,j,k) = yy(i-1,j,k).
$$

## Feeding — and draining — the matrix

$A_{ij}$ is indexed by **both** domain axes and consumed **once**, so — exactly as
`gemv`'s matrix — it has no free axis to enter on and needs a feed axis $k$. But
`ger`'s result is *also* a full matrix, and a fully-parallel operator has no
reduction axis for it to exit along. So $k$ is a **depth-2 feed/drain** axis
(`0 <= k < 2`): the incoming $A$ seeds the accumulator on the $k=-1$ halo, the
rank-1 term is added at $k=0$, and the updated matrix drains out on the $k=1$ face —
the same *inject → add → drain* shape as [`scal`](scal.md)/[`axpy`](axpy.md), but on
the $(i,j)$ face. This is what the issue means by *"A enters and exits on the
$(i,j)$ face."*

## Adding the rank-1 term exactly once

The accumulator $r$ is seeded by $A$ and would otherwise add $\alpha x y$ at *both*
$k=0$ and $k=1$. The fix is `scal`'s trick: inject $\alpha$ **only on the $k=-1$
halo**, with interior value $0$. Then the product contributes once (at $k=0$, where
it reads the halo $\alpha$) and vanishes at the drain (at $k=1$, where it reads the
interior $0$):

$$
r(i,j,k) = r(i,j,k-1) + \alpha(i,j,k-1)\,xx(i,j-1,k)\,yy(i-1,j,k),
\qquad \alpha(i,j,k) = 0 \ \text{(interior)} .
$$

## SURE

$$
\begin{aligned}
\alpha(i,j,k) &= 0 \\
xx(i,j,k)     &= xx(i,j-1,k) \\
yy(i,j,k)     &= yy(i-1,j,k) \\
r(i,j,k)      &= r(i,j,k-1) \;+\; \alpha(i,j,k-1)\,xx(i,j-1,k)\,yy(i-1,j,k)
\end{aligned}
$$

Every dependence is a **constant offset**, so `ger` is a pure **SURE** (uniform).
The operand taps are read one step *upstream* of their propagation, so no cell reads
a value produced at the same point.

## Confluences (oriented faces)

The outward normal is *derived* from each face's equality (inputs influx,
$\tau\!\cdot\!n < 0$; outputs outflux, $\tau\!\cdot\!n > 0$).

| tensor | face | outward normal | role |
|---|---|---|---|
| `A` | $k = -1$ (the $(i,j)$ face) | $(0,0,-1)$ | matrix seed |
| `Alpha` | $k = -1$ | $(0,0,-1)$ | scalar $\alpha$ (halo only) |
| `X` | $j = -1$ | $(0,-1,0)$ | row vector, broadcast $+j$ |
| `Y` | $i = -1$ | $(-1,0,0)$ | column vector, broadcast $+i$ |
| `Aout` (out) | $k = 1$ | $(0,0,1)$ | updated matrix |

## Schedule

The canonical wavefront $\tau = [1,1,1]$ is legal (`minSlack` $\tau\!\cdot\!\theta = 1$),
as is the data-flow-earliest free schedule. Because the operator is fully parallel,
its latency is dominated by the broadcast reach, not by any serial chain. Reversing
the feed/drain flux — e.g. $\tau = [-1,-1,-1]$ — is rejected by flux revalidation.

## Memory cardinality

`ger` holds the two broadcast vectors and the matrix accumulator live across the
wavefront, so its peak live set scales with the wavefront width. The liveness
analysis agrees exactly with an eviction run (`test_ger_sure`).

## Executable

The spec is the single source of truth; the test parses this file.

```text
dfactl --sure docs/SURE/ger.sure                 # free schedule
dfactl --sure docs/SURE/ger.sure --tau 1,1,1     # canonical linear schedule
```

With the bundled data ($A = [[1,2,3],[4,5,6]]$, $x = [10,20]$, $y = [1,2,3]$,
$\alpha = 2$): $A + \alpha x y^{\mathsf T} = [[21,42,63],[44,85,126]]$.
