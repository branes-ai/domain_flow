# gemv as a System of Uniform Recurrence Equations

The BLAS Level-2 general matrix-vector product is

$$
y \;=\; \beta\,y \;+\; \alpha\,A\,x,
\qquad
y_i \;=\; \beta\,y_i \;+\; \alpha \sum_{j=0}^{N-1} A_{ij}\,x_j,
\quad 0 \le i < M .
$$

It is the first Level-2 operator in the catalog, and structurally it is **`matmul`
with the second matrix collapsed to a single vector `x`**: a reduction over the
column index $j$ in which one operand ($x$) is reused across the rows while the
other ($A$) is consumed exactly once. Everything the catalog established for
[`matmul`](matmul.md) (the systolic reduction) and for the scalar handling in
[`scal`](scal.md)/[`axpy`](axpy.md)
(project-and-pipeline) reappears here in one operator.

## The matvec as a reduction

The core is $M$ independent inner products — one per row — sharing the vector $x$:

$$
\mathrm{acc}(i,j) \;=\; \mathrm{acc}(i,j-1) \;+\; A_{ij}\,x_j ,
\qquad \mathrm{acc}(i,-1) = 0,
$$

with the completed sum at the terminal $j = N-1$. The reduction runs along $+j$; the
rows $i$ are independent, so they form the parallel (wavefront) direction.

## Feeding the matrix: the $(i,j)$ face of a depth-1 axis

`matmul`'s operand $A_{ik}$ is **reused** along the orthogonal output axis, so it can
enter on a face and propagate. `gemv`'s $A_{ij}$ is indexed by **both** domain axes
and read **once** — it has no free axis to enter on. As with [`dot`](dot.md)'s
once-read operands, we add a **depth-1 feed axis** $k$ (`0 <= k < 1`): $A$ enters on the
$(i,j)$ face $k=-1$ and is read one step downstream by the accumulator.

$$
a(i,j,k) = a(i,j,k-1), \qquad a(i,j,-1) = A_{ij}.
$$

This is exactly what the issue means by *"$A$ enters on its $(i,j)$ face."*

## Pipelining $x$ and projecting the scalars

The remaining operands enter by the catalog's uniform idioms — never as literals in
a recurrence body:

- **$x$ is reused across rows**, so it is *pipelined* along $+i$:
  $xx(i,j,k) = xx(i-1,j,k)$, seeded $xx(-1,j,k) = x_j$.
- **$\alpha$ and $\beta$ are scalars**, so they are *projected onto a face and
  pipelined* (project-and-pipeline / one-shot injection). $\alpha$ is folded into the
  reduction product; $\beta$ multiplies the incoming $y$ in the epilogue:
  $\alpha(i,j,k) = \alpha(i,j,k-1)$ on the $(i,j)$ face, $\beta(i,j,k) = \beta(i-1,j,k)$
  on the $i=-1$ face.
- **The incoming $y_i$** is pipelined along the reduction $+j$ so it reaches the
  terminal face: $yin(i,j,k) = yin(i,j-1,k)$, seeded $yin(i,-1,k) = y_i$.

Because $\alpha$ and $\beta$ arrive as *recurrence variables*, the product
$\alpha \cdot A \cdot x$ is a product of three uniform taps — no affine broadcast.

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

Every dependence is a **constant offset**, so `gemv` is a pure **SURE** (uniform).
The operand taps $a(i,j,k-1)$, $\alpha(i,j,k-1)$ and $xx(i-1,j,k)$ are read one step
*upstream* of their propagation (never at the same point as `acc`), which keeps the
schedule strictly ordered.

## The epilogue: $\alpha A x + \beta y$

$\alpha$ is already inside `acc`, so the completed sum at $j=N-1$ is $\alpha (Ax)_i$.
The $\beta y_i$ term is added on the **output face** — a terminal-face epilogue that
reads the accumulator and the pipelined $y$:

$$
Y_i \;=\; \mathrm{acc}(i,N-1,0) \;+\; \beta(i,N-1,0)\,yin(i,N-1,0)
     \;=\; \alpha\!\sum_j A_{ij} x_j \;+\; \beta\,y_i .
$$

## Confluences (oriented faces)

Each tensor binds to a face; the outward normal is *derived* from the face's equality
(inputs influx, $\tau\!\cdot\!n < 0$; outputs outflux, $\tau\!\cdot\!n > 0$).

| tensor | face | outward normal | role |
|---|---|---|---|
| `A` | $k = -1$ (the $(i,j)$ face) | $(0,0,-1)$ | matrix feed |
| `Alpha` | $k = -1$ | $(0,0,-1)$ | scalar $\alpha$ |
| `X` | $i = -1$ | $(-1,0,0)$ | reused vector |
| `Beta` | $i = -1$ | $(-1,0,0)$ | scalar $\beta$ |
| `Yin` | $j = -1$ | $(0,-1,0)$ | incoming $y$ |
| seed | $j = -1$ | $(0,-1,0)$ | reduction seed $0$ |
| `Y` (out) | $j = N-1$ | $(0,1,0)$ | result |

## Schedule

The canonical wavefront $\tau = [1,1,1]$ is legal (`minSlack` $\tau\!\cdot\!\theta = 1$),
and so is the data-flow-earliest free schedule. Reversing the reduction flux — e.g.
$\tau = [-1,-1,-1]$ — is rejected by flux revalidation, since the seed/result faces
would then carry the wrong sign.

## Memory cardinality

Unlike `dot`'s $O(1)$ accumulator, `gemv` keeps several pipelined fields live at once
(the $x$ column, the $y$ pipeline, the running accumulator), so its peak live set
scales with the wavefront width rather than staying constant. The liveness analysis
agrees exactly with an eviction run (`test_gemv_sure`).

## Executable

The spec is the single source of truth; the test parses this file.

```text
dfactl --sure docs/SURE/gemv.sure                 # free schedule
dfactl --sure docs/SURE/gemv.sure --tau 1,1,1     # canonical linear schedule
```

With the bundled data ($A = [[1..4],[5..8],[9..12]]$, $x = [1,2,3,4]$, $\alpha = 2$,
$\beta = 10$, $y = [1,2,3]$): $Ax = [30,70,110]$ and
$y = \beta y + \alpha Ax = [70, 160, 250]$.
