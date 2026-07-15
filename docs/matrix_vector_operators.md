# Deriving the matrix–vector operators

The BLAS Level-2 operators — `gemv`, `ger`, `trmv`, `symv`, `syr`, `syr2` — are all
built from a small set of recurring **derivation patterns**. This page collects those
patterns in one place; the [SURE Algorithms](SURE/blas-naming.md) catalog then presents
each operator leanly (the algorithm, its SURE, and an animation) and links back here
for the *why*. Reading this once makes every L2 spec self-explanatory.

Throughout, the design goal is a **pure SURE** — every dependence a constant offset —
because uniformity is what makes an operator [tile and schedule](sure-simulator.md)
cleanly. The art is getting the *operands onto faces* and the *scalars into the
recurrence* without introducing an affine (domain-spanning) dependence.

## Pattern 1 — the reduction skeleton

A matrix–vector product $y_i = \sum_j A_{ij}\,x_j$ is a **reduction over $j$**. It is
[`matmul`](SURE/matmul.md) with the second matrix collapsed to a single vector:

$$
\mathrm{acc}(i,j) = \mathrm{acc}(i,j-1) + A_{ij}\,x_j,
\qquad \mathrm{acc}(i,-1) = 0,
$$

with the completed sum at the terminal of the $j$-sweep. The rows $i$ are
independent — the parallel (wavefront) direction. This is the core of `gemv`, `trmv`,
and `symv`.

## Pattern 2 — the feed axis (a matrix needs a face)

An operand indexed by a **single** domain axis (like $x_j$) can enter on a boundary
face and pipeline. But a **matrix** $A_{ij}$ is indexed by *both* axes and read
*once*, so it has no free axis to enter on. The fix is a **depth-1 feed axis** $k$:
$A$ enters on the $(i,j)$ face $k=-1$ and is read one step downstream,

$$
a(i,j,k) = a(i,j,k-1),\qquad a(i,j,-1) = A_{ij}.
$$

(This mirrors how [`dot`](SURE/dot.md) adds a feed axis for its once-read
operands.) When the operator's **result is also a full matrix** — the rank updates
`ger`, `syr`, `syr2` — there is no reduction axis for it to exit along either, so the
feed axis is **depth-2** (`0 ≤ k < 2`): the matrix seeds the accumulator on $k=-1$,
the update is added at $k=0$, and the result drains out on the $k=1$ face — the same
*inject → add → drain* shape as [`scal`](SURE/scal.md)/[`axpy`](SURE/axpy.md).

## Pattern 3 — project-and-pipeline the scalars

A scalar like $\alpha$ or $\beta$ must **never** appear as a literal in a recurrence
body — that would be an affine broadcast (one value read everywhere). Instead it is
**projected onto a face and pipelined**, then consumed as a *product of recurrence
variables*:

$$
\alpha(i,j,k) = \alpha(i,j,k-1),\qquad \alpha(i,j,-1) = \text{Alpha}[0],
$$

so $\alpha\,A_{ij}\,x_j$ becomes a product of three uniform taps. For the rank
updates the trick is sharper still: injecting $\alpha$ **only on the halo** (interior
value $0$) makes the update contribute *exactly once* — it fires at $k=0$ and
vanishes at the $k=1$ drain. This "one-shot injection" is how `ger`/`syr`/`syr2` add
their outer product a single time.

## Pattern 4 — the terminal-face epilogue

`gemv`/`symv` finish with $y := \alpha A x + \beta y$. With $\alpha$ folded into the
reduction, the completed sum is $\alpha(Ax)_i$; the $\beta y$ term joins it on the
**output face** — a terminal-face epilogue reading the accumulator and the pipelined
incoming $y$:

$$
Y_i = \mathrm{acc}(i,N-1) + \beta\,y_i.
$$

Output faces may read taps and do arithmetic, so the epilogue needs no extra machinery.

## Pattern 5 — triangular domains and the diagonal-output constraint

`trmv`, `syr`, `syr2` compute over a **triangular** index set $j \le i$ — the first
non-box domains in the catalog. Two consequences:

- **Operand entry changes.** On a triangular domain a column begins at the diagonal,
  so a $+i$ broadcast would have to enter on a diagonal face — parallel to the output,
  which admits *no linear schedule*. The remedy is to feed such an operand **on the
  $k$ face** (per cell) instead of pipelining it along $+i$ (used by `trmv` for $x$,
  and by `syr`/`syr2` for the vectors).
- **The output can land on a diagonal.** In `trmv` the reduction for row $i$
  completes at $j = i$, so the result leaves on the **diagonal face** $i-j=0$, whose
  outward normal $(-1,1,0)$ forces $\tau_j > \tau_i$. The box-operator default
  $\tau = [1,1,1]$ is *rejected* (zero flux on the diagonal); $\tau = [1,2,1]$ is the
  canonical schedule. This is a concrete way the domain geometry constrains the
  schedule. (The rank updates `syr`/`syr2` drain on the $k=1$ face instead, so they
  keep $\tau = [1,1,1]$.)

## Pattern 6 — the symmetric confluence

Symmetric operators store **one triangle** and route it to both sides.

- **`symv`** ($y = \alpha A x$, $A = A^{\mathsf T}$): a **two-face feed** over the
  full square — the lower cells ($j \le i$) read the stored $A_{ij}$, the upper cells
  ($i < j$) read the **reflected** $A_{ji}$. So cell $(i,j)$ always sees
  $A^{\text{sym}}_{ij}$ and the reduction is the ordinary `gemv` sweep; a stored
  $A_{rc}$ is read by cell $(r,c)$ *and* its mirror $(c,r)$ — the two contributions,
  as two confluence faces reading one triangle.
- **`syr` / `syr2`** ($A \mathrel{+}= \alpha x x^{\mathsf T}$ and
  $\alpha(x y^{\mathsf T} + y x^{\mathsf T})$): the outer product itself is symmetric,
  so the **output** is the triangle. A single vector drives both axes (fed per cell as
  $x_i$ and $x_j$); `syr2` needs two vectors and four feeds and forms
  $\alpha(x_i y_j + y_i x_j)$ from uniform taps.

In every case the symmetry lives in the **confluence or the domain**, never in the
recurrence — which stays a pure SURE.

## Summary

| operator | reduction? | feed | scalars | domain | notable |
|---|---|---|---|---|---|
| `gemv` | yes ($+j$) | depth-1 | α, β | box | reduction + epilogue |
| `ger` | no | depth-2 | α (once) | box | outer product |
| `trmv` | yes ($+j$) | depth-1 | — | triangular | diagonal output, $\tau_j>\tau_i$ |
| `symv` | yes ($+j$) | depth-1 | α, β | box | two-face symmetric feed |
| `syr` | no | depth-2 | α (once) | triangular | one vector, both axes |
| `syr2` | no | depth-2 | α (once) | triangular | two vectors, fused rank-2 |

Each row is a combination of Patterns 1–6. The [catalog](SURE/blas-naming.md) pages show
the resulting SURE and its schedule in motion.
