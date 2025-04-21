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

### Final SURE Formulation
Combining all parts, the SURE for QR decomposition is:


# SURE for QR Decomposition

**System**: \((i, j, k) \mid 0 \leq i < m, 0 \leq j < n, 0 \leq k \leq j\)

**Equations**:
1. **Initialize intermediate vector**:
   \[
   v(i, j, 0) = a(i, j)
   \]

2. **Compute partial sum for projection coefficient**:
   \[
   s_r(i, j, k) = 
   \begin{cases} 
   q(i, k) \cdot v(i, j, k) & \text{if } i = 0 \\
   s_r(i-1, j, k) + q(i, k) \cdot v(i, j, k) & \text{if } 0 < i < m 
   \end{cases}
   \]

3. **Projection coefficient**:
   \[
   r(k, j, k) = s_r(m-1, j, k)
   \]

4. **Update intermediate vector**:
   \[
   v(i, j, k+1) = v(i, j, k) - r(k, j, k) \cdot q(i, k)
   \]

5. **Compute partial sum for norm**:
   \[
   s_norm(i, j, j) = 
   \begin{cases} 
   v(i, j, j)^2 & \text{if } i = 0 \\
   s_norm(i-1, j, j) + v(i, j, j)^2 & \text{if } 0 < i < m 
   \end{cases}
   \]

6. **Norm (diagonal R entry)**:
   \[
   r(j, j, j) = \sqrt{s_norm(m-1, j, j)}
   \]

7. **Normalize to get Q column**:
   \[
   q(i, j) = v(i, j, j) / r(j, j, j)
   \]

**Notes**:
- \( a(i, j) \): Input matrix \( A \) of size \( m \times n \).
- \( v(i, j, k) \): Intermediate vectors during orthogonalization.
- \( s_r(i, j, k) \): Partial sums for computing \( r(k, j, k) \).
- \( r(i, j, k) \): Upper triangular matrix \( R \) entries (only \( i \leq j \)).
- \( s_norm(i, j, j) \): Partial sums for computing the norm.
- \( q(i, j) \): Orthogonal matrix \( Q \) of size \( m \times n \).
- The formulation assumes \( m \geq n \) and uses Modified Gram-Schmidt for stability.


### Notes on the Formulation
- **Uniform Dependencies**: The dependencies are uniform (e.g., \( i-1 \), \( k-1 \), \( k+1 \)), matching the style of the matmul SURE. The reductions are handled via partial sum arrays \( s_r \) and \( s_norm \), which introduce dependencies on \( i-1 \).
- **Comparison to Matmul**: Like the matmul SURE, this uses a 3D index space and expresses computations as recurrences. However, QR involves reductions (sums) and a non-linear operation (square root), which are handled carefully to maintain uniformity where possible.
- **DFA Compatibility**: The SURE is designed to be compatible with DFA’s requirement for uniform dependencies, making it suitable for mapping to hardware or parallel execution in a domain flow graph.
- **Assumptions**: Assumes \( m \geq n \) (tall or square matrix). For \( m < n \), the formulation would need adjustment. The output \( Q \) is \( m \times n \) (thin QR), but can be extended to \( m \times m \) if needed.

This formulation captures the essence of QR decomposition in a way that aligns with the DFA methodology and the provided matmul example. 