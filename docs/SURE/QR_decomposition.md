# QR Decomposition SURE

To provide a SURE (System of Uniform Recurrence Equations) formulation for QR decomposition in the same style as the matrix multiplication (matmul) example, we need to express the QR decomposition algorithm in a way that captures its data dependencies and computations as a system of equations with uniform recurrence patterns. Let’s derive this step-by-step, ensuring the formulation aligns with the Domain Flow Architecture (DFA) methodology and matches the provided matmul structure.

### Understanding QR Decomposition
QR decomposition factorizes a matrix \( A \) (of size \( m \times n \), where \( m \geq n \)) into an orthogonal matrix \( Q \) (size \( m \times m \)) and an upper triangular matrix \( R \) (size \( m \times n \)), such that \( A = QR \). A common algorithm for QR decomposition is the Gram-Schmidt process, particularly the Modified Gram-Schmidt (MGS) variant, which is more numerically stable and can be expressed with uniform dependencies suitable for a SURE formulation. We’ll focus on MGS for an \( m \times n \) matrix, as it lends itself to a regular computational structure.

### Modified Gram-Schmidt Overview
For an input matrix \( A = [a_1, a_2, \ldots, a_n] \) with columns \( a_j \), MGS iteratively computes:
- Orthogonal vectors \( q_j \) (columns of \( Q \)).
- Upper triangular entries \( r_{ij} \) (elements of \( R \)).

The process can be summarized as:
1. For each column \( j = 1 \) to \( n \):
   - Initialize \( v_j = a_j \).
   - For each \( i = 1 \) to \( j \):
     - Compute \( r_{ij} = q_i^T v_j \) (projection coefficient).
     - Update \( v_j = v_j - r_{ij} q_i \) (orthogonalization).
   - Compute \( r_{jj} = \| v_j \|_2 \).
   - Normalize \( q_j = v_j / r_{jj} \).

### Formulating SURE for QR Decomposition
To express this as a SURE, we need:
- **Index space**: Define the iteration domain over indices, similar to \( (i,j,k) \) in matmul.
- **Variables**: Define arrays for inputs (\( A \)), intermediate results (\( V \)), and outputs (\( Q \), \( R \)).
- **Equations**: Express computations with uniform dependencies (e.g., referencing previous indices like \( j-1 \)).
- **Domain constraints**: Specify bounds for indices, e.g., \( 0 \leq i < m \), \( 0 \leq j < n \).

#### Step 1: Define the Index Space
The MGS algorithm involves:
- Iterating over columns \( j = 0 \) to \( n-1 \) (for each column of \( A \)).
- For each \( j \), iterating over rows \( i = 0 \) to \( m-1 \) (for vector elements).
- For orthogonalization, iterating over previous columns \( k = 0 \) to \( j-1 \) (for projections).

The primary computation involves three indices:
- \( i \): Row index (0 to \( m-1 \)).
- \( j \): Column index being processed (0 to \( n-1 \)).
- \( |k|: Index for orthogonalization steps (0 to \( j \)).

This suggests a 3D index space \( (i, j, k) \), similar to the matmul SURE, but we’ll adjust the role of \( k \).

#### Step 2: Define Variables
We define the following arrays:
- **Input**: \( a(i, j) \), the input matrix \( A \) of size \( m \times n \).
- **Intermediates**:
  - \( v(i, j, k) \): Intermediate vectors during orthogonalization.
  - \( r(i, j, k) \): Coefficients \( r_{ij} \) for \( i \leq j \).
- **Outputs**:
  - \( q(i, j) \): Orthogonal matrix \( Q \) (columns \( q_j \)).
  - \( r(i, j) \): Upper triangular matrix \( R \).

#### Step 3: Express Computations as Uniform Recurrence Equations
We break down the MGS steps into equations with uniform dependencies.

1. **Initialize \( v_j = a_j \)**:
   For each column \( j \), the initial vector is the column of \( A \).
   \[
   v(i, j, 0) = a(i, j)
   \]
   Here, \( k=0 \) indicates the start of orthogonalization.

2. **Compute \( r_{ij} = q_i^T v_j \)**:
   For each \( j \), and for each \( k = 0 \) to \( j-1 \), compute the projection:
   \[
   r(k, j, k) = \sum_{i=0}^{m-1} q(i, k) \cdot v(i, j, k)
   \]
   This is a reduction over \( i \), which we’ll handle by introducing an auxiliary array to compute the sum incrementally.

3. **Update \( v_j = v_j - r_{ij} q_i \)**:
   Update the vector after each projection:
   \[
   v(i, j, k+1) = v(i, j, k) - r(k, j, k) \cdot q(i, k)
   \]
   For \( k = 0 \) to \( j-1 \).

4. **Compute \( r_{jj} = \| v_j \|_2 \)**:
   After orthogonalization (at \( k = j \)):
   \[
   r(j, j, j) = \sqrt{\sum_{i=0}^{m-1} v(i, j, j)^2}
   \]
   Another reduction over \( i \).

5. **Normalize \( q_j = v_j / r_{jj} \)**:
   \[
   q(i, j) = v(i, j, j) / r(j, j, j)
   \]

#### Step 4: Handle Reductions
The reductions (sums for \( r_{ij} \) and \( r_{jj} \)) require iterating over \( i \). To make dependencies uniform, we introduce partial sum arrays:
- \( s_r(i, j, k) \): Partial sum for \( r(k, j, k) \).
- \( s_norm(i, j, j) \): Partial sum for the norm computation.

For \( r_{ij} \):
\[
s_r(i, j, k) = 
\begin{cases} 
q(i, k) \cdot v(i, j, k) & \text{if } i = 0 \\
s_r(i-1, j, k) + q(i, k) \cdot v(i, j, k) & \text{if } i > 0 
\end{cases}
\]
\[
r(k, j, k) = s_r(m-1, j, k)
\]

For \( r_{jj} \):
\[
s_norm(i, j, j) = 
\begin{cases} 
v(i, j, j)^2 & \text{if } i = 0 \\
s_norm(i-1, j, j) + v(i, j, j)^2 & \text{if } i > 0 
\end{cases}
\]
\[
r(j, j, j) = \sqrt{s_norm(m-1, j, j)}
\]

#### Step 5: Define the Domain
The index space is:
\[
(i, j, k) \mid 0 \leq i < m, 0 \leq j < n, 0 \leq k \leq j
\]
- \( i \): Rows of the matrix (\( 0 \leq i < m \)).
- \( j \): Columns of the matrix (\( 0 \leq j < n \)).
- \( k \): Orthogonalization step (\( 0 \leq k \leq j \)), since we process up to \( j \) projections, and \( k = j \) is used for the norm and normalization.

### Final SURE Formulation (v2 confluence DSL)

The derivation above sketches the algorithm with case splits (`if i = 0`),
partial-domain equations (`v(i,j,0) = a(i,j)`, `r(k,j,k) = ...`), and
mixed-rank outputs (`q(i,j)` is 2D while `v(i,j,k)` is 3D). None of that
survives contact with the Domain Flow discipline: every equation must be a
*total* recurrence over the shared domain, base cases must enter through
oriented input confluences on the halo, and results must leave through
oriented output confluences on faces of the domain. The refined,
executable formulation ([`qr.sure`](qr.sure)):

```text
M = 3; N = 3;

system ((i,j,k) | 0 <= i < M, 0 <= j < N, 0 <= k < N, k <= j) {
    v(i,j,k)     = v(i,j,k-1) - srp(M-1,j,k-1) * qhat(i,k-1,k-1);   // orthogonalize
    srp(i,j,k)   = srp(i-1,j,k) + qhat(i,k,k) * v(i,j,k);           // r_kj reduction
    snorm(i,j,k) = snorm(i-1,j,k) + v(i,j,k) * v(i,j,k);            // ||v_j||^2 reduction
    qhat(i,j,k)  = v(i,j,k) / sqrt(snorm(M-1,j,k));                 // normalize
}

input A[M][N] ((i,j,k) | 0 <= i < M, 0 <= j < N, k = -1)             : v(i,j,k) = A[i][j];
input         ((i,j,k) | M-1 <= i < M, 0 <= j < N, k = -1)           : srp(i,j,k) = 0;
input         ((i,j,k) | 0 <= j < N, 0 <= k < N, k <= j, i = -1)     : srp(i,j,k) = 0;
input         ((i,j,k) | 0 <= j < N, 0 <= k < N, k <= j, i = -1)     : snorm(i,j,k) = 0;
input         ((i,j,k) | 0 <= i < M, -1 <= k < 0, j = -1)            : qhat(i,j,k) = 0;

output Q[M][N]    ((i,j,k) | 0 <= i < M, 0 <= j < N, 0 <= k < N, k - j = 0)   : Q[i][j] = qhat(i,j,k);
output Rdiag[N]   ((i,j,k) | M-1 <= i < M, 0 <= j < N, 0 <= k < N, k - j = 0) : Rdiag[j] = sqrt(snorm(i,j,k));
output Roff[N][N] ((i,j,k) | 0 <= j < N, 0 <= k < N, k < j, i = M-1)          : Roff[k][j] = srp(i,j,k);
```

Run it:

```console
$ dfactl --sure docs/SURE/qr.sure
  input  A -> v  normal (0,0,-1)
  ...
  output Q <- qhat  normal (0,-1,1)
  output Rdiag <- snorm  normal (0,-1,1)
  output Roff <- srp  normal (1,0,0)
  Q[0][0] = 0.857143 ... Rdiag[0] = 14 ... Roff[0][1] = 21 ...
```

### The refinements, one by one

1. **Case splits become input confluences.** `v(i,j,0) = a(i,j)` is not a
   separate equation: the recurrence `v(i,j,k) = v(i,j,k-1) - ...` simply
   *escapes* the domain at `k = 0`, and the input confluence on the `k = -1`
   halo face supplies `A[i][j]` there. The subtractive term vanishes on that
   first step because its own taps escape to zero-valued faces: `srp` on the
   `k = -1` face, and `qhat` at the corner points `(i,-1,-1)`.

2. **The corner escape and the one-equality rule.** A face has exactly one
   equality (codimension 1). The `qhat` escape lands on the codimension-2
   set `(i,-1,-1)` — the trick is to pin the second coordinate with
   *inequalities*: `((i,j,k) | 0 <= i < M, -1 <= k < 0, j = -1)` has the
   single equality `j = -1` while `-1 <= k < 0` confines `k` exactly.

3. **Reductions are first-class recurrence variables.** The derivation's
   `s_r` and `s_norm` partial sums become `srp` and `snorm`, seeded to zero
   through their `i = -1` faces and read where the reduction completes, on
   the `i = M-1` face. The completed value is fetched with an *affine*
   (broadcast) tap `srp(M-1, j, k-1)` — a constant row index, which is what
   makes this a SARE rather than a pure uniform system.

4. **`q` is uniformized as `qhat` over the whole domain.** The 2D output
   `q(i,j)` of the sketch cannot live in a 3D system. Instead `qhat`
   normalizes `v` by the running column norm at *every* point; the only
   points other equations ever tap are the diagonal `(i,k,k)`, where
   `qhat(i,k,k) = v_k / ||v_k|| = q(i,k)` exactly. The off-diagonal values
   are uniformization slack: computed, never consumed. This is the classic
   space-time trade of embedding a lower-dimensional value in the full
   domain.

5. **`R` leaves through two oriented faces.** The sketch's `r(k,j,k)`
   conflates two different results. The off-diagonal projection
   coefficients `r_kj` (k < j) complete where the reduction ends and exit
   through the `i = M-1` face with outward normal `(1,0,0)`; the diagonal
   norms `r_jj` exit through the *diagonal* face `k = j` (outward normal
   `(0,-1,1)`), the same face `Q` leaves through. Non-axis-aligned output
   faces are exactly why face regions are equality-pinned constraint sets
   rather than named box sides.

6. **No global linear schedule.** The broadcast taps (`srp(M-1,...)`,
   `snorm(M-1,...)`) and the diagonal taps (`qhat(i,k,k)`) give dependence
   *vectors that vary with the point*, so no single `tau` satisfies
   `tau.theta >= 1` everywhere — `qr.sure` declares no `tau` and runs under
   the free (ASAP) schedule. Index-set splitting (piecewise schedules) is
   the standard remedy and remains future work.

### Verification

`qr.sure` binds the classic 3x3 example `A = [[12,-51,4],[6,167,-68],[-4,24,-41]]`
with the exact factorization `R = [[14,21,-14],[0,175,-70],[0,0,35]]`. The
test suite (`src/dfa/tests/sim/sure_parser.cpp`) checks `R` exactly,
`Q^T Q = I` and `Q R = A` to machine precision (both residuals < 1e-15),
and the free-schedule legality of the parsed system.

### Notes

- The reductions are linear chains along `i` (O(m) depth); a balanced
  reduction tree would shorten the critical path but needs log-indexed
  dependencies outside the affine form — a representation question for the
  DFA, not the DSL.
- The formulation assumes `m >= n` (thin QR) and a full-rank `A` (the
  normalization divides by the running column norm).
- Givens-rotation QR (the Gentleman-Kung array) admits a fully *uniform*
  system with a linear schedule; expressing it in the DSL is a natural
  companion exercise.
