# symv as a System of Uniform Recurrence Equations

The BLAS Level-2 symmetric matrix-vector product is

$$
y \;:=\; \beta\,y \;+\; \alpha\,A\,x,
\qquad A = A^{\mathsf T},
$$

where only **one triangle** of the symmetric $A$ is stored. Numerically it is
[`gemv`](gemv.md); the interest is entirely in the **symmetric-storage confluence
pattern** — how a single stored triangle drives a full matrix–vector product.

## One triangle, two contributions

The efficient symmetric algorithm reads each stored element $A_{ij}$ (with $j \le i$)
**once** and routes it to **two** accumulations:

$$
y_i \mathrel{+}= A_{ij}\,x_j
\qquad\text{and}\qquad
y_j \mathrel{+}= A_{ij}\,x_i .
$$

The second is the symmetric partner ($A_{ji} = A_{ij}$). In the domain-flow view the
cleanest way to express "one element, two contributions" is a **two-face feed** over
the full square domain $(i,j)$: the operand variable $a(i,j)$ is bound to the stored
triangle in two ways,

$$
a(i,j) = \begin{cases}
A_{ij} & j \le i \quad(\text{lower cell reads the stored element}),\\[2pt]
A_{ji} & i < j \quad(\text{upper cell reads the } \textbf{reflected}\ \text{element}),
\end{cases}
$$

so cell $(i,j)$ always sees $A^{\text{sym}}_{ij}$ and the reduction
$y_i = \sum_j a(i,j)\,x_j$ is the ordinary `gemv` sweep. A stored element $A_{rc}$
($r \ge c$) is read by cell $(r,c)$ — contributing $A_{rc}x_c$ to $y_r$ — **and** by
its mirror cell $(c,r)$ — contributing $A_{rc}x_r$ to $y_c$. That is exactly the two
accumulations, realized as two confluence faces reading one triangle. The diagonal
$A_{rr}$ sits in the lower face only, so it contributes once — no double count.

## Everything else is gemv

With $a$ carrying $A^{\text{sym}}$, the rest is [`gemv`](gemv.md) verbatim: $x_j$ is
reused across rows (pipelined along $+i$), the scalars $\alpha,\beta$ are
project-and-pipelined, and the $\beta y$ term joins the completed sum in a
terminal-face epilogue.

## SURE

$$
\begin{aligned}
a(i,j,k)     &= a(i,j,k-1) \\
xx(i,j,k)    &= xx(i-1,j,k) \\
\alpha(i,j,k) &= \alpha(i,j,k-1) \\
\beta(i,j,k)  &= \beta(i-1,j,k) \\
yin(i,j,k)   &= yin(i,j-1,k) \\
\mathrm{acc}(i,j,k) &= \mathrm{acc}(i,j-1,k) \;+\; \alpha(i,j,k-1)\,a(i,j,k-1)\,xx(i-1,j,k)
\end{aligned}
$$

Every dependence is a **constant offset**, so `symv` is a pure **SURE** (uniform) —
the symmetry lives entirely in the confluence (the reflected upper feed $A_{ji}$),
not in the recurrence.

## Confluences (oriented faces)

The feed for $a$ is **two** faces over the same stored triangle:

| tensor | face | outward normal | role |
|---|---|---|---|
| `A` (lower) | $j \le i$, $k=-1$ | $(0,0,-1)$ | $a = A_{ij}$ |
| `A` (upper) | $i < j$, $k=-1$ | $(0,0,-1)$ | $a = A_{ji}$ (reflected) |
| `Alpha` | $k = -1$ | $(0,0,-1)$ | scalar $\alpha$ |
| `X` | $i = -1$ | $(-1,0,0)$ | reused vector |
| `Beta` | $i = -1$ | $(-1,0,0)$ | scalar $\beta$ |
| `Yin` | $j = -1$ | $(0,-1,0)$ | incoming $y$ |
| seed | $j = -1$ | $(0,-1,0)$ | reduction seed $0$ |
| `Y` (out) | $j = N-1$ | $(0,1,0)$ | result |

## Schedule & memory

The domain is a box, so the canonical wavefront $\tau = [1,1,1]$ is legal, as is the
free schedule; a reversed-flux $\tau$ is rejected. The live set matches `gemv`'s
(the $x$ column, the $y$ pipeline, the accumulator); the liveness analysis agrees
with an eviction run (`test_symv_sure`).

## Executable

```text
dfactl --sure docs/SURE/symv.sure                 # free schedule
dfactl --sure docs/SURE/symv.sure --tau 1,1,1     # canonical linear schedule
```

With the bundled symmetric $A = [[2,3,5],[3,4,6],[5,6,7]]$ (stored as its lower
triangle), $x = [1,2,3]$, $\alpha = 2$, $\beta = 10$, $y = [1,2,3]$:
$A x = [23,29,38]$ and $y = \beta y + \alpha A x = [56,78,106]$.
