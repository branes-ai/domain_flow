# syr as a System of Uniform Recurrence Equations

The BLAS Level-2 symmetric rank-1 update is

$$
A \;\mathrel{+}=\; \alpha\,x\,x^{\mathsf T},
\qquad
A_{ij} \;:=\; A_{ij} + \alpha\,x_i\,x_j,
\quad j \le i .
$$

`syr` is [`ger`](ger.md) with $y = x$ — a **single vector on both axes** — and the
output restricted to **one triangle**, because $A + \alpha x x^{\mathsf T}$ is
symmetric so only the lower triangle need be stored and updated. It is the
symmetric-positive-definite accumulation primitive behind **Cholesky's trailing
(Schur) update**, and it combines two patterns already in the catalog: `ger`'s
inject → add → drain and `trmv`'s triangular domain.

## Triangular output, one vector, two axes

The update is fully parallel (no reduction) over the triangular index set $j \le i$:

$$
A_{ij} \;:=\; A_{ij} + \alpha\,x_i\,x_j .
$$

Each cell needs $x_i$ and $x_j$. As in `ger` the matrix $A$ is twice-indexed and
consumed once, and here the *result* is also a triangle with no reduction axis to
exit along — so $A$ enters (and, updated, exits) on the $(i,j)$ face of a **depth-2
feed/drain** axis $k$: seed on $k=-1$, add at $k=0$, drain on $k=1$.

The single vector $x$ drives **both** axes. On `ger`'s box domain we broadcast $x$
along $+j$ and $y$ along $+i$; on `syr`'s triangular domain a $+i$ broadcast would
have to enter on a diagonal face (each column starts at the diagonal — cf.
[`trmv`](trmv.md)), so $x$ is instead **fed per cell on the $k$ face**: $x_i$ as
$xx$, $x_j$ as $yy$, both from the same input tensor.

## Adding the rank-1 term exactly once

$r$ is seeded by the incoming $A$ and would add $\alpha x_i x_j$ at both $k=0$ and
$k=1$. As in `ger`, injecting $\alpha$ **only on the $k=-1$ halo** (interior value
$0$) makes the product contribute once (at $k=0$) and vanish at the drain ($k=1$):

$$
r(i,j,k) = r(i,j,k-1) + \alpha(i,j,k-1)\,xx(i,j,k-1)\,yy(i,j,k-1),
\qquad \alpha(i,j,k) = 0 \ \text{(interior)} .
$$

## SURE

$$
\begin{aligned}
\alpha(i,j,k) &= 0 \\
xx(i,j,k)     &= xx(i,j,k-1) \\
yy(i,j,k)     &= yy(i,j,k-1) \\
r(i,j,k)      &= r(i,j,k-1) \;+\; \alpha(i,j,k-1)\,xx(i,j,k-1)\,yy(i,j,k-1)
\end{aligned}
$$

Every dependence is a **constant offset**, so `syr` is a pure **SURE** (uniform); the
symmetry lives entirely in the *triangular domain*, not the recurrence.

## Confluences (oriented faces)

One vector $x$ feeds both axes; all inputs enter on the $(i,j)$ feed face $k=-1$.

| tensor | face | outward normal | role |
|---|---|---|---|
| `A` | $j \le i$, $k = -1$ | $(0,0,-1)$ | lower triangle seeds $r$ |
| `Alpha` | $j \le i$, $k = -1$ | $(0,0,-1)$ | scalar $\alpha$ (halo only) |
| `X` → $xx$ | $j \le i$, $k = -1$ | $(0,0,-1)$ | $x_i$ |
| `X` → $yy$ | $j \le i$, $k = -1$ | $(0,0,-1)$ | $x_j$ (same vector) |
| `Aout` (out) | $j \le i$, $k = 1$ | $(0,0,1)$ | updated lower triangle |

## Schedule & memory

The result drains on the $k = 1$ face (normal $(0,0,1)$), so unlike
[`trmv`](trmv.md)'s diagonal output there is no $\tau_j > \tau_i$ requirement: the
canonical $\tau = [1,1,1]$ is legal, as is the free schedule; a reversed-flux $\tau$
is rejected. The live set is small (the two feeds and the accumulator over the
triangle); the liveness analysis matches an eviction run (`test_syr_sure`).

## Executable

```text
dfactl --sure docs/SURE/syr.sure                 # free schedule
dfactl --sure docs/SURE/syr.sure --tau 1,1,1     # canonical linear schedule
```

With the bundled lower-triangular $A = [[1],[2,3],[4,5,6]]$, $x = [1,2,3]$,
$\alpha = 2$: the updated lower triangle is $[[3],[6,11],[10,17,24]]$.
