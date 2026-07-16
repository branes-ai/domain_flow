# dot as a System of Uniform Recurrence Equations

The BLAS Level-1 `dot` product is the canonical **reduction**:

$$
s \;=\; x^{\mathsf T} y \;=\; \sum_{i=0}^{N-1} x_i\,y_i .
$$

Where `axpy` is a fully-parallel map, `dot` is its opposite — a single scalar
distilled from the whole vector by a chain of multiply-adds. It is the running
example for how the domain-flow representation treats a reduction: the linear
accumulation chain, the operand feed, and the latency/memory trade of the free
schedule versus a tree.

## The reduction as a recurrence

Written as a recurrence over the accumulation index `i`,

$$
s(i) = s(i-1) + x_i\,y_i, \qquad s(-1) = 0,
$$

so $s(i) = \sum_{i' \le i} x_{i'} y_{i'}$ and the answer is the last partial sum
$s(N-1)$. Two structural facts follow immediately and shape everything below:

- **The result is a scalar.** It leaves through the single terminal point
  `i = N-1` — a codimension-1 face of the reduction axis. (Contrast `axpy`, whose
  length-$N$ result needed a full output face and hence a second axis.)
- **The chain is strictly sequential.** $s(i)$ depends on $s(i-1)$, a dependence
  vector of $+1$ along `i` with no slack to spare. This linear chain is what makes
  a reduction latency-bound (see *Schedule*).

## Feeding the operands: a depth-1 array

The operands $x_i, y_i$ are indexed by the **reduction coordinate** `i` itself. A
1-D domain over `i` therefore has nowhere to inject them: its only halo is the
single point `i = -1`, but we need a distinct $x_i$ at *every* cell. Per the
catalog convention, an operand enters through a confluence on a boundary face —
so we add a **feed axis** `j` and inject $X[i], Y[i]$ on the `j = -1` halo:

$$
D = \{\,(i,j) \mid 0 \le i < N,\; 0 \le j < 1\,\}.
$$

Why is the feed only **one cell deep** (`0 ≤ j < 1`)? Because a reduction
*consumes* each operand exactly once — $x_i$ is multiplied into the sum at cell
`i` and never reused. This is the precise structural difference from `matmul`,
whose operands `A(i,k)`, `B(k,j)` are reused across the orthogonal axis and so
*propagate* through the domain. `dot` reuses nothing, so the array is a single
column of multiply-add cells fed from `j = -1`, accumulating along `+i`.

## SURE

```
system ((i,j) | 0 <= i < N, 0 <= j < 1) {
    x(i,j) = x(i,j-1);                         // operand X, fed from the j = -1 halo
    y(i,j) = y(i,j-1);                         // operand Y, fed from the j = -1 halo
    s(i,j) = s(i-1,j) + x(i,j-1) * y(i,j-1);   // accumulate X[i]*Y[i] along +i
}
```

The multiply-add reads its operands at the constant offset `(0,-1)` — the feed
halo — and the previous partial sum at `(-1,0)`. Both are constant offsets of the
current point, so `s` is a **uniform** recurrence (nonlinear in the values, uniform
in the dependences, exactly as `matmul`'s `a·b`). `x` and `y` are consumed at the
halo, so their in-domain equations are never evaluated (the reduction reads the
injected value directly); they are present only so each operand tensor has a
confluence to enter through.

## Confluences (oriented faces)

Faces are equality-pinned in the domain's own coordinates; the outward normal is
*derived* as the equality plane's outward normal with respect to the domain
(inputs are influx, `tau·n < 0`; outputs are outflux, `tau·n > 0`).

```
input  X[N] ((i,j) | 0 <= i < N, j = -1)  : x(i,j) = X[i];   // feed X  -> normal (0,-1)
input  Y[N] ((i,j) | 0 <= i < N, j = -1)  : y(i,j) = Y[i];   // feed Y  -> normal (0,-1)
input       ((i,j) | i = -1, 0 <= j < 1)  : s(i,j) = 0;      // seed    -> normal (-1,0)
output S[1] ((i,j) | i = N-1, 0 <= j < 1) : S[0] = s(i,j);   // result  -> normal (1, 0)
```

Both operands are true vectors, fed in across the `j = -1` halo; the accumulator
seed fluxes in at the `i = -1` face; and the scalar result fluxes out at the
terminal `i = N-1` face. Derived normals: `X, Y → (0,-1)`, `seed → (-1,0)`,
`S → (1,0)`.

## Schedule

The reduction is a linear chain, and that has a sharp consequence: **the free
schedule and the linear schedule coincide.** The data-flow-earliest firing order
must respect $s(i) \succ s(i-1)$, so it degenerates to the sequential order
$t\big(s(i)\big) = i$ — there is no parallelism in a chain to recover. The
canonical

$$
\tau = [\,1,\; 1\,]
$$

is legal (every dependence vector — the `+i` accumulation, the `+j` feed reads —
has `tau·theta = 1 ≥ 1`), and its latency is $N$. For $N = 4$ both the free and
linear schedules report latency 4.

Flux consistency reads off the geometry: with `seed → (-1,0)`, `X,Y → (0,-1)`,
`S → (1,0)`, any $\tau = [\tau_i, \tau_j]$ with $\tau_i > 0$ and $\tau_j > 0$ makes
the seed and operands influx and the result outflux. Reversing $\tau_i$, e.g.
$\tau = [-1, 1]$, turns the seed into an outflux and is rejected by flux
revalidation before legality is consulted.

### Latency vs. memory: the tree alternative

The linear chain is latency-bound at $N$ but needs only an **$O(1)$ accumulator**.
The dual is a **tree reduction** obtained by *index-set splitting*: pair the terms
and sum pairwise, $\lceil \log_2 N \rceil$ deep. That cuts latency from $N$ to
$\log_2 N$ but holds up to $N/2$ partial sums at the widest level — the classic
reduction trade of latency against resident set. The linear-chain SURE here is the
minimum-memory end of that spectrum; a split domain (a follow-up, as with the
piecewise QR schedules) walks toward the minimum-latency end.

## Memory cardinality

Under $\tau = [1,1]$ the analysis reports `peakLiveValues = 2` — only the
accumulator is resident (the producer $s(i-1)$ and its consumer $s(i)$ overlap for
one step), latency 4, work 12, and the eviction run realizes the same footprint.
The operands contribute nothing to the resident set: they are read once at the
feed halo and never held. This is the reduction signature — an $O(1)$ footprint
regardless of $N$ — and the baseline that Level-2/3 reductions (`gemv`, `gemm`)
replicate across their output faces.

## Executable

The spec above is the executable `docs/SURE/dot.sure`. Run it through the SURE
front-end:

```bash
dfactl --sure docs/SURE/dot.sure
# free schedule; s = 1*4 + 2*3 + 3*2 + 4*1 = 20

dfactl --sure docs/SURE/dot.sure --tau -1,1
# ILLEGAL: reversed i-flux (seed/result), rejected by flux revalidation
```

The regression test `src/dfa/tests/sim/dot_sure.cpp` (CTest `test_dot_sure`) parses
this document's spec, checks the derived face normals, verifies $s = x^{\mathsf T}y$
against a direct reference, confirms free/linear schedule legality, and asserts the
$O(1)$ accumulator footprint.

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `dot`'s three variables — the accumulator `s` and the two streamed operands
`x`, `y` — connect with constant offsets only:

| arc | θ | dependence |
| --- | --- | --- |
| `s → s` | `[1,0]ᵀ` | the accumulator chains along the reduction |
| `x → s` | `[0,1]ᵀ` | `x` feeds the product |
| `y → s` | `[0,1]ᵀ` | `y` feeds the product |
| `x → x` | `[0,1]ᵀ` | `x` streamed to the accumulator |
| `y → y` | `[0,1]ᵀ` | `y` streamed to the accumulator |

Every arc is a translation vector, so `dot` is a genuine **SURE**. The self-loop
`s → s` *is* the reduction chain — the source of the sequential dependence that makes the
inner product a length-`N` wavefront with no parallelism to recover.

<div class="rdg" data-src="rdg/dot.json" data-height="520"></div>

## Watch the schedule

The inner product is a linear-chain reduction: the wavefront is a single point sweeping the accumulation from `i = 0` to `i = N-1` in `N` steps — a chain has no parallelism to recover. Shown at N = 16. Press play, or scrub; drag to orbit.

<div class="schedule-anim" data-src="schedules/dot-linear.json" data-height="340"></div>
