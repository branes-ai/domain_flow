# Symmetric eigensolver — cyclic Jacobi

The cyclic Jacobi method diagonalizes a symmetric `A = QΛQᵀ` by a sequence of two-sided
Givens rotations, each chosen to **zero one off-diagonal pair** `(p,q)`. Sweep through all
pairs, repeat, and the off-diagonal mass falls monotonically to zero: the diagonal holds
the eigenvalues `Λ`, the accumulated rotations the eigenvectors `Q`. It is the most
rotation-native eigensolver — but only *part* of it is a single recurrence system.

## One rotation is the kernel

Zeroing `A[p][q]` is a two-sided similarity `A' = JᵀAJ` with `J` a rotation in the `(p,q)`
plane. The angle is chosen algebraically (no `atan` needed):

$$
\tau = \frac{A_{qq} - A_{pp}}{2\,A_{pq}},
\quad
t = \frac{\operatorname{sign}\tau}{|\tau| + \sqrt{\tau^2 + 1}},
\quad
c = \frac{1}{\sqrt{t^2+1}},
\quad
s = t\,c,
$$

so `A'[p][q] = cs(A_{qq}-A_{pp}) + (c^2-s^2)A_{pq} = 0`. (If `A_{pq}=0` the division
overflows and `t→0`, `c→1`, `s→0`: no rotation, exactly right.) This spec is that single
rotation on the fixed pivot `(p,q) = (0,1)` — the executable heart of the method.

## Two one-sided rotations

A two-sided update touches only rows and columns `p,q`, which is awkward to express as one
formula. Split it into **right- then left-multiply**:

- `B = A·J` mixes **columns** `p,q`: `B[i][p] = c·A[i][p] − s·A[i][q]`,
  `B[i][q] = s·A[i][p] + c·A[i][q]`, other columns unchanged;
- `A' = Jᵀ·B` mixes **rows** `p,q` the same way.

The "is this column/row `p` or `q`?" test cannot read the loop index, so it reads **fed
indicator vectors** `Pind = [1,0,0]`, `Qind = [0,1,0]` and a `select`; the pivot
rows/columns enter as **fixed-index affine taps** `a0(i,0)`, `a0(i,1)`, `bcol(0,j)`,
`bcol(1,j)`. Those affine taps (and the rotation-angle taps of `A[0][0]`, `A[1][1]`,
`A[0][1]`) make it a **SARE**.

```text
tau  = (a0(1,1) - a0(0,0)) / (2 a0(0,1));                 // angle from fixed pivot entries
cs   = 1/sqrt(t^2+1);   sn = t*cs;                        // t = sign(tau)/(|tau|+sqrt(tau^2+1))
bcol = select(cp, cs*a0(i,0) - sn*a0(i,1),               // right-multiply: mix columns p,q
       select(cq, sn*a0(i,0) + cs*a0(i,1), a0(i,j)));
arot = select(rp, cs*bcol(0,j) - sn*bcol(1,j),           // left-multiply: mix rows p,q
       select(rq, sn*bcol(0,j) + cs*bcol(1,j), bcol(i,j)));
```

Executable spec: `docs/SURE/eig_jacobi.sure`. For `A = [[2,1,1],[1,3,1],[1,1,4]]` the
rotation zeros `A'[0][1]` and, being an orthogonal similarity, preserves the spectrum —
`test_eig_jacobi_sure` checks `A' = JᵀAJ` to machine precision (trace `9`, Frobenius²
`35` invariant). The `(0,1)` block diagonalizes to its eigenvalues `1.382, 3.618`.

## Why the full sweep is not one SURE

A cyclic sweep applies this rotation to a **changing** pivot `(p,q)`. Computing each
rotation needs `A[p][p]`, `A[q][q]`, `A[p][q]` — but with `p,q` varying per step those are
reads at **data-dependent indices** (a gather), which the confluence DSL cannot express:
its taps are affine in the *loop* indices, not indexed by runtime data. This is the same
obstruction that keeps [partial pivoting](lu.md) out of the SURE world, and it is why the
executable artifact is the fixed-pivot rotation while the sweep — cycling `(p,q)` over all
pairs and repeating to convergence — is a *scheduled composition* of these rotations,
described here but not a single uniform recurrence. On a real fabric it is exactly that: a
fixed program of rotations streamed over the array, each a local two-sided update.

## The RDG

The rotation's dependence graph: the held matrix `a0`, the four fed indicators, the
rotation-angle chain (`tau → t → cs, sn`), and the two update variables `bcol`, `arot`,
wired by the fixed-index **affine** taps that read the pivot rows, columns, and diagonal —
seven of them, the signature of the two-sided rotation.

<div class="rdg" data-src="rdg/eig_jacobi.json" data-height="620"></div>

<div class="schedule-anim" data-src="schedules/eig_jacobi-free.json" data-height="360"></div>
