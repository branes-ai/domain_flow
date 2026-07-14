# rot as a System of Uniform Recurrence Equations

The BLAS Level-1 `rot` applies a plane (Givens) rotation to a pair of vectors:

$$
\begin{pmatrix} x_i \\ y_i \end{pmatrix}
\;\leftarrow\;
\begin{pmatrix} c & s \\ -s & c \end{pmatrix}
\begin{pmatrix} x_i \\ y_i \end{pmatrix},
\qquad 0 \le i < N,
$$

with a shared cosine/sine pair $(c,s)$, $c^2 + s^2 = 1$. It is the catalog's
**rotation primitive** — the elementwise kernel of the Jacobi/Givens sweeps that
drive [`qr`](QR_decomposition.md), the SVD, and the symmetric eigensolvers — and,
structurally, it is [`axpy`](axpy.md) promoted from a scalar multiply-add to a
$2\times2$ linear map.

## axpy, generalized to a 2×2 map

`axpy` had one projected scalar ($\alpha$), one injected operand ($x$), and one
result stream ($y := \alpha x + y$). `rot` has the same machinery, doubled:

| | `axpy` | `rot` |
|---|--------|-------|
| projected scalars | $\alpha$ | $c,\ s$ |
| injected operands | $x$ | $x,\ y$ |
| result streams    | $y$ | $\mathrm{rx},\ \mathrm{ry}$ |

Each result stream is a linear combination of the two operands with the two
scalars:

$$
\mathrm{rx}(i) = c\,x_i + s\,y_i, \qquad \mathrm{ry}(i) = -s\,x_i + c\,y_i .
$$

Everything the earlier entries established applies unchanged: the scalars are
**projected and pipelined**, the operands are **one-shot injected**, and the
result streams are **seeded with the additive identity** and drained at the
terminal face.

## Domain and flows

Both results are length-$N$ vectors, so — as always for a vector output — they
ride the `i` axis and use a two-cell pipeline `j` for an oriented output face:

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 2\,\}.
$$

- **`c`, `s`** are 0-D operands, each **projected** onto the `i = -1` edge and
  pipelined across the lanes: `c(i,j) = c(i-1,j)`, `s(i,j) = s(i-1,j)`. (Inlining
  a literal would be a broadcast — an affine, non-uniform dependence; see
  [`axpy`](axpy.md), *Uniformity*.)
- **`x`, `y`** are **injected** on the `j = -1` halo and consumed once
  (`x(i,j) = 0` in-domain — the additive identity, so each product is added
  exactly once).
- **`rx`, `ry`** accumulate their two-term combination at the front cell and carry
  it to `j = 1`.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 2) {
    c(i,j)  = c(i-1,j);                                              // cosine pipelined across lanes
    s(i,j)  = s(i-1,j);                                              // sine   pipelined across lanes
    x(i,j)  = 0;                                                     // one-shot injection
    y(i,j)  = 0;                                                     // one-shot injection
    rx(i,j) = rx(i,j-1) + c(i-1,j)*x(i,j-1) + s(i-1,j)*y(i,j-1);     //  c*x + s*y
    ry(i,j) = ry(i,j-1) - s(i-1,j)*x(i,j-1) + c(i-1,j)*y(i,j-1);     // -s*x + c*y
}
```

Every product tap is a constant offset of the current point, so all six equations
are uniform recurrences. `rx` and `ry` read the *same* four flows (`c`, `s`, `x`,
`y`) at the *same* offsets and differ only in the sign pattern — the two rows of
the rotation matrix.

## Confluences (oriented faces)

Faces are equality-pinned; the outward normal is *derived* (inputs are influx,
`tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  C[1] ((i,j) | i = -1, 0 <= j < 2) : c(i,j) = C[0];    // project c -> normal (-1, 0)
input  S[1] ((i,j) | i = -1, 0 <= j < 2) : s(i,j) = S[0];    // project s -> normal (-1, 0)
input  X[N] ((i,j) | 0 <= i < N, j = -1) : x(i,j) = X[i];    // inject x  -> normal (0,-1)
input  Y[N] ((i,j) | 0 <= i < N, j = -1) : y(i,j) = Y[i];    // inject y  -> normal (0,-1)
input       ((i,j) | 0 <= i < N, j = -1) : rx(i,j) = 0;      // seed rx   -> normal (0,-1)
input       ((i,j) | 0 <= i < N, j = -1) : ry(i,j) = 0;      // seed ry   -> normal (0,-1)
output Xout[N] ((i,j) | 0 <= i < N, j = 1) : Xout[i] = rx(i,j);   // -> normal (0,1)
output Yout[N] ((i,j) | 0 <= i < N, j = 1) : Yout[i] = ry(i,j);   // -> normal (0,1)
```

Six input confluences (two projected scalars, two injected operands, two result
seeds) and two output confluences — the widest confluence structure in the L1
catalog. Derived normals: `C, S → (-1,0)`; `X, Y, rx-seed, ry-seed → (0,-1)`;
`Xout, Yout → (0,1)`.

## Schedule and memory

`rot` inherits `axpy`'s schedule structure. $\tau = [1,1]$ is legal — every
dependence vector (the `+j` carries, the `+i` scalar pipelines) has
`tau·theta = 1` — and so is the free schedule. Flux consistency needs
$\tau_i, \tau_j > 0$ so the scalars flux in across the lanes and the operands/
results along the pipeline; a backward $\tau = [1,-1]$ is rejected.

Under $\tau = [1,1]$ the analysis reports `peakLiveValues = 20` for $N = 4$ (per
variable `c = 5, s = 5, x = 4, y = 4, rx = 2, ry = 2`), matching the eviction run:
the two scalar pipelines plus the two operand wavefronts dominate; the two result
accumulators stay $O(1)$.

## Correctness: a rotation preserves the norm

Because $c^2 + s^2 = 1$, the map is orthogonal, so $\mathrm{rx}_i^2 +
\mathrm{ry}_i^2 = x_i^2 + y_i^2$ for every lane. The regression test checks this
invariant alongside the exact rotated values — e.g. for $(c,s) = (0.6,0.8)$ and
$(x,y)_0 = (1,5)$: $(\mathrm{rx},\mathrm{ry})_0 = (4.6, 2.2)$ with
$4.6^2 + 2.2^2 = 26 = 1^2 + 5^2$.

## Executable

The spec above is the executable `docs/SURE/rot.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/rot.sure
# free schedule; (c,s) = (0.6,0.8):  Xout = 0.6X + 0.8Y,  Yout = -0.8X + 0.6Y

dfactl --sure docs/SURE/rot.sure --tau 1,-1
# ILLEGAL: backward j-flux, rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/rot_sure.cpp` (CTest `test_rot_sure`) parses
this document's spec, checks the six input and two output face normals, verifies
the rotated pair against the reference matrix *and* the norm-preserving invariant,
and confirms free/linear schedule legality and the memory analysis.

## Watch the schedule

The six Givens-rotation flows — two projected scalars, two operands, two result streams — sweep the strip together under `τ = [1,1]`. Shown at N = 12. Press play, or scrub; drag to orbit.

<div class="schedule-anim" data-src="schedules/rot-linear.json" data-height="340"></div>
