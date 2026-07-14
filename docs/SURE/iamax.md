# iamax as a System of Uniform Recurrence Equations

The BLAS Level-1 `iamax` returns the index of the largest-magnitude element:

$$
\operatorname{iamax}(x) \;=\; \arg\max_{0 \le i < N} \lvert x_i \rvert .
$$

It is a reduction like [`dot`](dot.md), but a **select reduction**: the fold is not
arithmetic — it *chooses* — and it carries an `(index, value)` pair rather than a
running sum. `iamax` is the pivot-selection primitive behind LU partial pivoting,
the LP simplex entering/leaving rules, and threshold tests in the eigensolvers, and
it is the first catalog operator whose recurrence needs a comparison.

## A select reduction over a carried pair

The running state is the best pair seen so far — the largest magnitude and the
index that achieved it:

$$
\big(\mathrm{mval}(i),\ \mathrm{midx}(i)\big) =
\begin{cases}
\big(\lvert x_i \rvert,\ i\big) & \lvert x_i \rvert > \mathrm{mval}(i-1) \\[2pt]
\big(\mathrm{mval}(i-1),\ \mathrm{midx}(i-1)\big) & \text{otherwise,}
\end{cases}
$$

seeded with $(\mathrm{mval},\mathrm{midx})(-1) = (-1,-1)$, a sentinel that loses to
any real magnitude so lane 0 always wins the first comparison. The result is
$\mathrm{midx}(N-1)$. The comparison is **strict** ($>$), so on a tie the *earlier*
index is kept — matching the BLAS convention.

## Two new exact built-ins: `gt` and `select`

The arithmetic reductions (`dot`, `nrm2`, `asum`) fold with `+`; a select
reduction needs a choice, which the expression grammar could not express. Two
exact selection built-ins were added to the DSL for this class of operator:

| built-in | value | note |
|----------|-------|------|
| `gt(a,b)`      | `1` if `a > b`, else `0` | strict indicator; ties → `0` |
| `select(c,x,y)`| `x` if `c ≠ 0`, else `y` | exact branch, no rounding |

They are **pointwise value operations**, not index dependences, so they leave the
recurrence uniform — every argument is still a constant-offset tap. With them the
pair update is a pair of uniform equations sharing one condition:

$$
\begin{aligned}
\mathrm{mval}(i) &= \mathtt{select}\big(\mathtt{gt}(\lvert x_i\rvert,\ \mathrm{mval}(i{-}1)),\ \lvert x_i\rvert,\ \mathrm{mval}(i{-}1)\big),\\
\mathrm{midx}(i) &= \mathtt{select}\big(\mathtt{gt}(\lvert x_i\rvert,\ \mathrm{mval}(i{-}1)),\ i,\ \mathrm{midx}(i{-}1)\big).
\end{aligned}
$$

## Feeding the operand — and the index

As in [`dot`](dot.md), the operand is indexed by the reduction coordinate, so it
enters on a **depth-1 feed axis** `j`. Two things enter on that halo:

- the **magnitude** $\lvert x_i \rvert$, via the `abs` input-face prologue (as in
  [`asum`](asum.md));
- the **index** $i$ itself. The equation body cannot read the loop index (bare
  index variables are forbidden), so — consistent with the *project-and-pipeline*
  convention — the index is **injected as data**: `Idx[i] = i` enters on the feed
  halo just like any operand, and `midx` selects among the injected `idx` values.

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 1\,\}.
$$

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 1) {
    av(i,j)   = av(i,j-1);                                                       // magnitude, fed from j = -1
    idx(i,j)  = idx(i,j-1);                                                      // index, fed from j = -1
    mval(i,j) = select(gt(av(i,j-1), mval(i-1,j)), av(i,j-1), mval(i-1,j));      // keep the larger magnitude
    midx(i,j) = select(gt(av(i,j-1), mval(i-1,j)), idx(i,j-1), midx(i-1,j));     // keep the winner's index
}
```

Every tap — `av(i,j-1)`, `idx(i,j-1)`, `mval(i-1,j)`, `midx(i-1,j)` — is a constant
offset, so all four equations are uniform. `mval` and `midx` share the single
condition `gt(av(i,j-1), mval(i-1,j))`: the value stream decides, and the index
stream follows the same choice.

## Confluences (oriented faces)

Faces are equality-pinned; the outward normal is *derived* (inputs are influx,
`tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  X[N]   ((i,j) | 0 <= i < N, j = -1)  : av(i,j)  = abs(X[i]);   // feed |X|   -> normal (0,-1)
input  Idx[N] ((i,j) | 0 <= i < N, j = -1)  : idx(i,j) = Idx[i];      // feed index -> normal (0,-1)
input         ((i,j) | i = -1, 0 <= j < 1)  : mval(i,j) = -1;         // seed value -> normal (-1,0)
input         ((i,j) | i = -1, 0 <= j < 1)  : midx(i,j) = -1;         // seed index -> normal (-1,0)
output I[1]   ((i,j) | i = N-1, 0 <= j < 1) : I[0] = midx(i,j);       // argmax     -> normal (1, 0)
```

The magnitude and index flux in along the feed; the carried pair is seeded on the
`i = -1` face; and the argmax index fluxes out at the terminal `i = N-1` face.
Derived normals: `X, Idx → (0,-1)`, `seeds → (-1,0)`, `I → (1,0)`.

## Schedule and memory

`iamax` is a linear reduction chain, so — like `dot` — the free and linear
schedules coincide (a chain has no parallelism to recover), $\tau = [1,1]$ is legal
(every dependence vector at `tau·theta = 1`), and the latency is $N$. The `gt`/
`select` built-ins add no dependence edge; they are pure value ops at each cell.

Under $\tau = [1,1]$ the analysis reports `peakLiveValues = 4` for $N = 4$ — the two
carried streams `mval` and `midx` at two live values each; the magnitude and index
are read once at the feed halo and never held. Twice `dot`'s accumulator, because
the state is a *pair*, but still $O(1)$ in $N$.

## Executable

The spec above is the executable `docs/SURE/iamax.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/iamax.sure
# free schedule; |X| = {1,5,3,2} -> argmax at index 1

dfactl --sure docs/SURE/iamax.sure --tau -1,1
# ILLEGAL: reversed i-flux (seed/result), rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/iamax_sure.cpp` (CTest `test_iamax_sure`)
parses this document's spec and checks the confluence normals, the argmax result,
and — on inline specs — the `gt`/`select` semantics that matter for a pivot rule:
strict tie-breaking to the earliest index, magnitude (not signed) comparison, a
last-element winner, and the all-zeros case.
