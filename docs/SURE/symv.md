# symv — symmetric matrix–vector product

`symv` computes **`y := αAx + βy`** for a symmetric `A = Aᵀ` stored as a single
triangle. Numerically it is [`gemv`](gemv.md); the interest is how one stored triangle
drives a full matrix–vector product.

## The SURE

A **two-face feed** over the full square realizes the symmetric storage: the lower
cells (`j ≤ i`) read the stored `A[i][j]`, the upper cells (`i < j`) read the
**reflected** `A[j][i]`. So cell `(i,j)` always sees `A_sym(i,j)` and the rest is the
ordinary `gemv` reduction (`x` reused across rows, `α`/`β` projected, `βy` epilogue).

```text
system ((i,j,k) | 0 <= i < N, 0 <= j < N, 0 <= k < 1) {
    a(i,j,k)     = a(i,j,k-1);                                              // A_sym(i,j)
    xx(i,j,k)    = xx(i-1,j,k);
    alpha(i,j,k) = alpha(i,j,k-1);
    beta(i,j,k)  = beta(i-1,j,k);
    yin(i,j,k)   = yin(i,j-1,k);
    acc(i,j,k)   = acc(i,j-1,k) + alpha(i,j,k-1)*a(i,j,k-1)*xx(i-1,j,k);
}
// A feeds on TWO faces of one stored triangle:
input a(i,j,k) = A[i][j]   on  j <= i    // lower: the stored element
input a(i,j,k) = A[j][i]   on  i <  j    // upper: the reflected element
output Y[i] = acc(i,N-1,0) + beta*yin;
```

Executable spec: `docs/SURE/symv.sure`. For the symmetric-storage confluence pattern,
see [deriving the matrix–vector operators](../matrix_vector_operators.md).

## Schedule

Box domain, so the canonical **τ = [1,1,1]** is legal, as is the free schedule. The
wavefront sweeps the `(i,j)` plane exactly as `gemv`.

<div class="schedule-anim" data-src="schedules/symv-linear.json" data-height="440"></div>

## Reduced dependency graph

The [reduced dependency graph](reduced_dependency_graph.md) (RDG) draws the recurrence as
one node per variable and one arc per dependence, each annotated with its **translation
vector** `θ`. `symv`'s six variables mirror `gemv`'s — the symmetry lives in the *storage*
(one triangle of `A`, read on two faces), not the dependence structure, which stays
uniform:

| arc | θ | dependence |
| --- | --- | --- |
| `acc → acc` | `[0,1,0]ᵀ` | the reduction chains along `+j` |
| `a → acc` | `[0,0,1]ᵀ` | `A` (fed on `k`) enters the product |
| `alpha → acc` | `[0,0,1]ᵀ` | α enters the product |
| `xx → acc` | `[1,0,0]ᵀ` | `x` enters the product |
| `a → a` | `[0,0,1]ᵀ` | `A` held on the feed axis |
| `xx → xx` | `[1,0,0]ᵀ` | `x` pipelined across rows `+i` |
| `alpha → alpha` | `[0,0,1]ᵀ` | α projected on the feed face |
| `beta → beta` | `[1,0,0]ᵀ` | β projected on the `i = -1` face |
| `yin → yin` | `[0,1,0]ᵀ` | the incoming `y` streamed along `+j` |

Every arc is a translation vector, so `symv` is a genuine **SURE**.

<div class="rdg" data-src="rdg/symv.json" data-height="620"></div>
