# syr2 as a System of Uniform Recurrence Equations

The BLAS Level-2 symmetric rank-2 update is

$$
A \;\mathrel{+}=\; \alpha\,(x\,y^{\mathsf T} + y\,x^{\mathsf T}),
\qquad
A_{ij} \;:=\; A_{ij} + \alpha\,(x_i\,y_j + y_i\,x_j),
\quad j \le i .
$$

`syr2` is [`syr`](syr.md) with **two vectors**: the two rank-1 outer products
$x\,y^{\mathsf T}$ and $y\,x^{\mathsf T}$ are **fused** into one symmetric update.
$A + \alpha(x y^{\mathsf T} + y x^{\mathsf T})$ is symmetric, so only the lower
triangle is stored and updated. It is the kernel behind **`syr2k`** and the
**LDLᵀ / eigensolver** trailing (Schur) updates.

## Two vectors, four feeds

The structure is exactly `syr`'s — a depth-2 feed/drain axis $k$ (inject → add →
drain) over the triangular output $j \le i$ — but the rank-2 combination needs
**four** per-cell values, $x_i, x_j, y_i, y_j$, fed from the two input vectors $X$
and $Y$ (each drives two of the four feeds):

$$
xi = X_i,\quad xj = X_j,\quad yi = Y_i,\quad yj = Y_j
\qquad(\text{all fed per cell on the }(i,j)\text{ face}).
$$

The symmetric rank-2 term is then formed in the recurrence from uniform taps:

$$
r(i,j,k) = r(i,j,k-1) + \alpha(i,j,k-1)\,\bigl(xi\cdot yj + yi\cdot xj\bigr),
\qquad \alpha(i,j,k) = 0 \ \text{(interior)} .
$$

As in `syr`, $\alpha$ is injected **only on the $k=-1$ halo**, so the rank-2 term is
added once (at $k=0$) and vanishes at the drain ($k=1$); $r$ is seeded by the
incoming $A$.

## SURE

$$
\begin{aligned}
\alpha(i,j,k) &= 0 \\
xi(i,j,k) = xi(i,j,k-1), &\quad xj(i,j,k) = xj(i,j,k-1) \\
yi(i,j,k) = yi(i,j,k-1), &\quad yj(i,j,k) = yj(i,j,k-1) \\
r(i,j,k) &= r(i,j,k-1) + \alpha(i,j,k-1)\,\bigl(xi\cdot yj + yi\cdot xj\bigr)
\end{aligned}
$$

Every dependence is a **constant offset**, so `syr2` is a pure **SURE** (uniform);
the two-outer-product fusion is just arithmetic on uniform taps, and the symmetry is
the triangular domain.

## Confluences (oriented faces)

Two vectors, each driving two feeds; all inputs enter on the $(i,j)$ face $k=-1$.

| tensor | face | outward normal | role |
|---|---|---|---|
| `A` | $j \le i$, $k = -1$ | $(0,0,-1)$ | lower triangle seeds $r$ |
| `Alpha` | $j \le i$, $k = -1$ | $(0,0,-1)$ | scalar $\alpha$ (halo only) |
| `X` → $xi$ | $j \le i$, $k = -1$ | $(0,0,-1)$ | $x_i$ |
| `X` → $xj$ | $j \le i$, $k = -1$ | $(0,0,-1)$ | $x_j$ |
| `Y` → $yi$ | $j \le i$, $k = -1$ | $(0,0,-1)$ | $y_i$ |
| `Y` → $yj$ | $j \le i$, $k = -1$ | $(0,0,-1)$ | $y_j$ |
| `Aout` (out) | $j \le i$, $k = 1$ | $(0,0,1)$ | updated lower triangle |

## Schedule & memory

As with `syr`, the result drains on the $k = 1$ face, so the canonical
$\tau = [1,1,1]$ is legal (no diagonal-output constraint), as is the free schedule; a
reversed-flux $\tau$ is rejected. The live set holds the four feeds and the
accumulator over the triangle; the liveness analysis matches an eviction run
(`test_syr2_sure`).

## Executable

```text
dfactl --sure docs/SURE/syr2.sure                 # free schedule
dfactl --sure docs/SURE/syr2.sure --tau 1,1,1     # canonical linear schedule
```

With the bundled lower-triangular $A = [[1],[2,3],[4,5,6]]$, $x = [1,2,3]$,
$y = [2,1,4]$, $\alpha = 1$: the updated lower triangle is $[[5],[7,7],[14,16,30]]$.
