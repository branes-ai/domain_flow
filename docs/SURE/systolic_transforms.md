# Systolic SURE transforms — regularizing to uniform dataflow

A **SURE** (System of *Uniform* Recurrence Equations) is the sweet spot for spatial hardware:
every data dependence is a **constant displacement vector**, so each cell talks only to fixed
neighbours — nearest-neighbour wires, no global routing. A **SARE** relaxes that: some
dependence is an **affine map** `p ↦ A·p + b` rather than a translation, which on silicon means
a *broadcast* or a *transpose* — long, irregular wires that a regular array cannot provide.

The [stationary iteration](stationary.md) page derives Jacobi and Gauss–Seidel as SAREs. This
page shows the **transformations** that turn them into pure SUREs — the same moves that
regularize most iterative linear-algebra kernels — and presents the RDG and wavefront animation
of each all-uniform result.

## The obstacle: the matrix-vector gather is a transpose

Both methods reduce `Σ_j A_ij x_j` per row `i`. Written directly,

```text
acc(i,j,k) = acc(i,j-1,k) + A(i,j) * x(j, …, k);   // reads unknown j's solution
```

the tap `x(j, …)` reaches from row `i` to **row `j`** — the `i↦j` transpose. Its dependence
matrix is not the identity, so the arc is **affine**: the operator is a SARE. Every column's
solution must reach every row — a broadcast. Three transforms remove it.

## Transform 1 — pipeline the state (a *piecewise* variable)

Instead of *gathering* `x_j`, **route** it. Introduce `xx(i,j,k)` = the value of unknown `j` as
seen at row `i`, **produced at the diagonal `i=j`** (where "row" equals "column", so no
transpose) and **broadcast** to the other rows by nearest-neighbour steps:

```text
xx(i,j,k | i > j) = xx(i-1,j,k);   // flow DOWN  from the diagonal   d = [+1,0,0]
xx(i,j,k | i < j) = xx(i+1,j,k);   // flow UP    from the diagonal   d = [-1,0,0]
xx(i,j,k | i = j) = …the solve…    // the source cell
```

Now the reduction reads its own **neighbour** (`xx(i-1,j,k)` or `xx(i,j,k-1)`) — a constant
displacement. The affine broadcast is gone, paid for by giving `xx` **three equations on the
disjoint sub-domains** `i>j`, `i<j`, `i=j`. That is a **piecewise variable** (a conditional
recurrence); the DSL dispatches each lattice point to the branch whose sub-domain contains it.

## Transform 2 — split the reduction so the solve reads *adjacent* columns

The solve for row `i` needs the finished sum, and it must live at the diagonal `(i,i)` to feed
the broadcast. But one reduction over all columns finishes at the **fixed** column `N-1`, so a
diagonal cell reading it spans a distance `N-1 … i` that **varies with `i`** — affine again.

Split the sum into a **lower** part (columns `j<i`) and an **upper** part (columns `j>i`):

```text
accL(i,j,k | j < i) = accL(i,j-1,k) + A(i,j) * xx(…);   // finishes at column i-1
accU(i,j,k | j > i) = accU(i,j+1,k) + A(i,j) * xx(…);   // reduces -j, finishes at column i+1
```

Now the diagonal solve reads `accL` and `accU` at the columns **immediately adjacent** to the
diagonal — a fixed one-step distance.

## Transform 3 — write the diagonal reads in `j`, not `i`

One subtlety makes or breaks it. On the diagonal the solve reads column `i-1` and `i+1`. Writing
those as `accL(i, i-1, k)` puts an `i` in the *column* slot → the dependence matrix maps `j ↦ i`
→ **affine**. But the solve equation lives **only on `i=j`**, where `i-1 == j-1`. Writing the
*same* value as `accL(i, j-1, k)` makes the displacement a constant `[0,+1,0]` — **uniform** —
and the classifier sees a SURE. The restriction to the diagonal is what licenses the rewrite.

```text
xx(i,j,k | i = j) = ( b_i - accL(i, j-1, k) - accU(i, j+1, k) ) / A(i,i);
```

## Jacobi — the all-uniform result

Putting the three transforms together (`docs/SURE/jacobi_systolic.sure`): the state is broadcast
up and down (all rows use the *previous* iterate, so `accL`/`accU` read `xx(i,j,k-1)`), and the
solution leaves on the **bottom row** `i=N-1`, where the down-broadcast has delivered the whole
vector. `dfactl` reports `kind: SURE` — **zero affine arcs**.

<div class="rdg" data-src="rdg/jacobi_systolic.json" data-height="560"></div>

<div class="schedule-anim" data-src="schedules/jacobi_systolic-free.json" data-height="380"></div>

## Gauss–Seidel — the two-sweep subtlety

Gauss–Seidel's lower sum reads **this** sweep (`x^k_j`, `j<i`) and its upper sum the **previous**
one (`x^{k-1}_j`, `j>i`). Both become uniform with the *right* neighbour:

- **lower** reads `xx(i-1,j,k)` — this sweep's **down**-broadcast of a just-solved `x^k_j`;
- **upper** reads `xx(i,j,k-1)` — the **previous** sweep's **up**-broadcast, which carries
  `x^{k-1}_j` (the diagonal of sweep `k-1` sent up to the rows above it).

The tempting shortcut — route the "previous iterate" straight forward in time as `x(k-1,i,j)` —
looks uniform but is **wrong**: for a row above the diagonal that value never re-touches the
diagonal, so it decays back to the *initial* `x^0_j`, not `x^{k-1}_j`. Reading the prior sweep's
up-broadcast is what actually delivers the previous iterate. (`docs/SURE/gauss_seidel_systolic.sure`
— again `kind: SURE`, zero affine arcs, and it reaches the exact solution in fewer sweeps than
Jacobi because it reuses fresh data.)

<div class="rdg" data-src="rdg/gauss_seidel_systolic.json" data-height="560"></div>

<div class="schedule-anim" data-src="schedules/gauss_seidel_systolic-free.json" data-height="380"></div>

## What "systolic" bought — and what it didn't

Every arc is now a translation, so the operators map to a regular `N`-cell array with only
nearest-neighbour wires — the point of the exercise. Two honest caveats:

- These forms are still **free-schedule-only** in this framework: the per-row reduce-and-divide
  fuse, so no single linear `τ` orders them. The *dataflow* is uniform; the *timing* is the
  free (data-flow-earliest) wavefront the animations show.
- The regularity is **piecewise**-uniform — the state's three branches are three constant
  displacements on three sub-domains, not one global vector. That is the honest meaning of a
  *conditional* URE, and exactly the machinery the DSL needed to gain to express it.
