# Reading BLAS names

BLAS (Basic Linear Algebra Subprograms) routine names look cryptic — `gemv`, `ger`,
`syr2`, `gemm` — but they are **systematic**: each name is a compressed description
of *what the routine does*. Once you can read the pieces, the names are
self-documenting and easy to remember. This page is the key.

## The three levels

BLAS is organized into levels by how much arithmetic a routine does relative to the
data it touches — which is exactly what decides whether it is memory-bound or
compute-bound, and how it tiles:

| level | shape | work / data | examples |
|---|---|---|---|
| **Level 1** | vector–vector | $O(n)$ / $O(n)$ | [`axpy`](axpy.md), [`dot`](dot.md), [`nrm2`](nrm2.md), [`scal`](scal.md) |
| **Level 2** | matrix–vector | $O(n^2)$ / $O(n^2)$ | [`gemv`](gemv.md), [`ger`](ger.md), `symv`, `syr` |
| **Level 3** | matrix–matrix | $O(n^3)$ / $O(n^2)$ | [`gemm`](matmul.md) |

Level 3 is the prize: $O(n^3)$ work on only $O(n^2)$ data means each element is
**reused** $O(n)$ times, so it tiles with high arithmetic intensity — the reason
`gemm` dominates DL compute.

## Anatomy of a name

A Level-2/3 name is three fields glued together: **`[type][structure][operation]`**.

**Field 1 — data type** (dropped throughout this catalog, where everything is real):

| `s` | `d` | `c` | `z` |
|---|---|---|---|
| single | double | complex | double complex |

**Field 2 — matrix structure** — which part of the matrix actually carries data:

| `ge` | `sy` | `he` | `tr` | `gb` / `sb` / `tb` |
|---|---|---|---|---|
| general | symmetric | Hermitian | triangular | banded |

**Field 3 — operation:**

| `mv` | `mm` | `r` | `r2` | `sv` |
|---|---|---|---|---|
| matrix–vector product | matrix–matrix product | rank-1 update | rank-2 update | triangular solve |

Read left to right and the name expands itself:

- **`gemv`** = `ge` + `mv` = **GE**neral **M**atrix–**V**ector product
- **`ger`** = `ge` + `r` = **GE**neral **R**ank-1 update
- **`syr2`** = `sy` + `r2` = **SY**mmetric **R**ank-**2** update
- **`gemm`** = `ge` + `mm` = **GE**neral **M**atrix–**M**atrix product

## The catalog, decoded

**Level 1** — the names predate the field scheme, so they are mnemonics rather than
`[type][structure][op]`:

| name | reads as | operation |
|---|---|---|
| [`axpy`](axpy.md) | **a**·**x** **p**lus **y** | $y := \alpha x + y$ |
| [`dot`](dot.md) | dot product | $s := x^{\mathsf T} y$ |
| [`nrm2`](nrm2.md) | **n**o**rm**, **2**-norm | $s := \lVert x\rVert_2$ |
| [`asum`](asum.md) | **a**bsolute **sum** | $s := \sum_i \lvert x_i\rvert$ |
| [`scal`](scal.md) | **scal**e | $x := \alpha x$ |
| [`swap`](swap.md) | swap | $x \leftrightarrow y$ |
| [`copy`](copy.md) | copy | $y := x$ |
| [`rot`](rot.md) | (Givens) **rot**ation | apply a plane rotation to $(x,y)$ |
| [`iamax`](iamax.md) | **i**ndex of **a**bs **max** | $\arg\max_i \lvert x_i\rvert$ |

**Level 2** — `[structure][operation]`:

| name | expands to | operation |
|---|---|---|
| [`gemv`](gemv.md) | general matrix–vector | $y := \alpha A x + \beta y$ |
| [`ger`](ger.md) | general rank-1 | $A := A + \alpha x y^{\mathsf T}$ |
| `trmv` | triangular matrix–vector | $x := T x$ |
| `symv` | symmetric matrix–vector | $y := \alpha A x + \beta y$, $A = A^{\mathsf T}$ |
| `syr` | symmetric rank-1 | $A := A + \alpha x x^{\mathsf T}$ |
| `syr2` | symmetric rank-2 | $A := A + \alpha(x y^{\mathsf T} + y x^{\mathsf T})$ |

**Level 3:**

| name | expands to | operation |
|---|---|---|
| [`gemm`](matmul.md) | general matrix–matrix | $C := \alpha A B + \beta C$ |

## Why the shorthand helps *here*

In the domain-flow view the name's fields often predict the **recurrence shape**:

- the **operation** field tells you the index-space geometry — `mv` is a *reduction*
  (a matvec contracts an axis), `r`/`r2` are *fully-parallel outer products* (a rank
  update has no reduction), and `mm` is the full *3-D reduction cube*;
- the **structure** field tells you the matrix is *referenced* only in a triangular
  half — `sy`/`he` exploit symmetry to read half the elements, `tr` is triangular.
  For the rank **updates** `syr`/`syr2` this restricts the *writes* too, so their
  domain of computation is a triangular prism rather than a box; the matrix–vector
  cases `symv`/`trmv` still produce a *full* vector, they just read half the matrix.

So `gemv` and `ger` being adjacent in the catalog is not a coincidence: they are the
inner-product and outer-product halves of the same matrix–vector interaction, and
their SUREs are almost mirror images (a reduction vs. a broadcast-only map).
