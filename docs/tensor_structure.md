# Tensor shape and structure

Understanding the "shape" of a tensor like `tensor<4x256x256x16xf32>` is key to working with multidimensional data, especially in contexts like machine learning, scientific computing, or your domain flow graph project. Let’s break it down clearly, address the concept of matrix slices, and provide a intuitive way to think about tensor shapes.

---

### 1. What is a Tensor and its Shape?
A **tensor** is a generalized multidimensional array that can store data in any number of dimensions. The **shape** of a tensor describes the size (extent) of each dimension. For example:

- `tensor<4x256x256x16xf32>` is a **4-dimensional tensor** where:
  - The dimensions are `4`, `256`, `256`, and `16`.
  - The data type (`f32`) indicates each element is a 32-bit floating-point number.
- The shape can be written as `(4, 256, 256, 16)`, meaning:
  - The first dimension has 4 elements.
  - The second dimension has 256 elements.
  - The third dimension has 256 elements.
  - The fourth dimension has 16 elements.

The total number of elements in the tensor is the product of the dimensions:
\[
4 \times 256 \times 256 \times 16 = 33,554,432 \text{ elements}
\]

Each element can be accessed using a 4-tuple index `[i, j, k, l]`, where:
- \(0 \leq i < 4\)
- \(0 \leq j < 256\)
- \(0 \leq k < 256\)
- \(0 \leq l < 16\)

---

### 2. Matrix Slices vs. Tensor Dimensions
You asked whether tensor dimensions are "matrix slice based" or if there’s a better way to think about them. Let’s explore this:

#### Matrix Slices
A **matrix** is a 2D tensor (e.g., `tensor<256x256xf32>`). You can think of a matrix as a grid where each element is accessed by a row index `i` and column index `j`. A "slice" of a matrix might refer to a subset, like a specific row (`matrix[i, :]`) or column (`matrix[:, j]`).

For a 4D tensor like `tensor<4x256x256x16xf32>`, you can interpret it as a collection of matrices by fixing some dimensions:
- If you fix the first and last dimensions (e.g., select `i = 2` and `l = 5`), you get a 2D slice:
  \[
  tensor[2, :, :, 5] \rightarrow \text{a } 256 \times 256 \text{ matrix}
  \]
- This slice is a 2D tensor of shape `(256, 256)`, which you can think of as a matrix.

Alternatively:
- Fix the first dimension (e.g., `i = 0`):
  \[
  tensor[0, :, :, :] \rightarrow \text{a } 256 \times 256 \times 16 \text{ 3D tensor}
  \]
- This 3D tensor can be viewed as a "stack" of 16 matrices of shape `(256, 256)`.

So, the tensor can be thought of as:
- A **4-element array** of **3D tensors** of shape `(256, 256, 16)`.
- Or a **4x16 array** of **2D matrices** of shape `(256, 256)` (by fixing the first and last dimensions).
- Or other combinations, depending on which dimensions you fix.

This "matrix slice" perspective is useful, especially in contexts like deep learning (e.g., a batch of images or feature maps), but it’s not the only way to conceptualize a tensor.

#### Limitations of Matrix Slices
- **Overly Restrictive**: Focusing on 2D slices may oversimplify the tensor’s structure. A 4D tensor is more than just a collection of matrices—it’s a higher-dimensional object where all dimensions interact.
- **Context-Dependent**: The meaning of each dimension depends on the application (e.g., batch size, height, width, channels in images; or time, spatial dimensions, features in your domain flow graph).
- **Indexing Complexity**: Slicing helps, but you still need to reason about all four indices to fully understand the data’s organization.

---

### 3. A Better Way to Think About Tensor Shapes
To move beyond matrix slices, here are intuitive ways to conceptualize a 4D tensor like `tensor<4x256x256x16xf32>`:

#### 1. **Nested Arrays**
Think of a tensor as a **nested hierarchy of arrays**:
- A 1D tensor (e.g., `tensor<4xf32>`) is a vector: `[a, b, c, d]`.
- A 2D tensor (e.g., `tensor<4x3xf32>`) is a matrix, or an array of vectors: `[[a1, a2, a3], [b1, b2, b3], ...]`.
- A 3D tensor (e.g., `tensor<4x3x2xf32>`) is an array of matrices.
- A 4D tensor is an **array of 3D tensors** or a **4-level nested array**.

For `tensor<4x256x256x16xf32>`:
- The outermost dimension (size 4) is an array of 4 elements.
- Each element is a 3D tensor of shape `(256, 256, 16)`.
- Each 3D tensor is an array of 256 elements, where each element is a 2D matrix of shape `(256, 16)`.
- Each 2D matrix is an array of 256 vectors of length 16.
- Each vector contains 16 `f32` values.

This nested perspective is precise and aligns with how tensors are stored in memory (often as flattened arrays with strides).

#### 2. **Geometric Interpretation**
Visualize the tensor as a **multidimensional grid** or **hypercube**:
- A 1D tensor is a line.
- A 2D tensor is a plane (grid).
- A 3D tensor is a cube.
- A 4D tensor is a "hypercube" or a grid in 4D space.

For `tensor<4x256x256x16xf32>`, imagine:
- A 4D space where each point is addressed by coordinates `(i, j, k, l)`.
- The "volume" of this space is constrained by:
  - \(0 \leq i < 4\)
  - \(0 \leq j < 256\)
  - \(0 \leq k < 256\)
  - \(0 \leq l < 16\)
- Each point `(i, j, k, l)` holds a single `f32` value.

While 4D is hard to visualize, you can project it to lower dimensions:
- Fix `i` and `l` to get a 2D grid (like a 256x256 image).
- Fix `i` to get a 3D cube (like a stack of 256x256x16 voxels).

#### 3. **Application-Specific Semantics**
The best way to understand a tensor’s shape is to map its dimensions to the problem domain. The dimensions often have specific meanings. For example:

- **In Deep** (deep learning):
  - Shape `(4, 256, 256, 16)` might represent:
    - **Batch size**: 4 (e.g., 4 images or samples).
    - **Height**: 256 pixels.
    - **Width**: 256 pixels.
    - **Channels**: 16 (e.g., feature maps in a convolutional neural network).
  - This is a batch of 4 images, each 256x256 pixels, with 16 feature channels per pixel.

- **In your domain flow graph**:
  - The dimensions might correspond to:
    - **Time steps**: 4 (e.g., 4 iterations or stages).
    - **Spatial dimensions**: 256x256 (e.g., a 2D grid for computation).
    - **Features or operators**: 16 (e.g., 16 different operator outputs or data channels).
  - Alternatively, if tied to index spaces in your SURE computations:
    - The first dimension (4) could represent different operators or configurations.
    - The 256x256 could represent the index space `{[i,j] : 0 ≤ i,j < 256}`.
    - The 16 could represent parallel threads, data slices, or variable instances in the recurrence equations.

By assigning semantic meaning, the shape becomes more intuitive. For example, if this tensor stores outputs of a 2D stencil computation, you might think of it as 4 time steps of a 256x256 grid with 16 values per grid point (e.g., different physical quantities).

#### 4. **Index Space Analogy**
Since you’re working with domain flow graphs and SUREs, think of the tensor’s shape as an **index space** (like the polyhedral sets in your earlier question):
- The shape `(4, 256, 256, 16)` defines a 4D index space `{[i, j, k, l] : 0 ≤ i < 4, 0 ≤ j < 256, 0 ≤ k < 256, 0 ≤ l < 16}`.
- Each index point `[i, j, k, l]` corresponds to a single `f32` value.
- Operations on the tensor (e.g., in your SURE) might involve dependencies between index points, like `[i, j, k, l] → [i-1, j, k, l]` for a time-step dependency.

This aligns with your project’s focus on index spaces and scheduling, making the tensor a natural representation of computation outputs over a multidimensional domain.

---

### 4. Practical Implications for Your Project
In the context of your domain flow graph and SUREs:
- **Index Space Connection**: The tensor’s shape likely corresponds to the index space of an operator’s computation. For example, if an operator computes over `{[i, j] : 0 ≤ i, j < 256}` for 4 iterations with 16 output channels, the tensor `tensor<4x256x256x16xf32>` could store the results.
- **SURE Dependencies**: The SURE defines how tensor elements depend on each other. For example, a stencil operator might define:
  \[
  tensor[i, j, k, l] = f(tensor[i-1, j, k, l], tensor[i, j-1, k, l], ...)
  \]
  The tensor’s shape constrains the valid indices for these dependencies.
- **Scheduling**: The scheduling vectors you compute (e.g., `s = [s1, s2, s3, s4]`) assign a time to each index point `[i, j, k, l]`. The tensor’s shape defines the domain over which these schedules apply, and the feasible scheduling cone ensures dependencies (e.g., `[i-1, j, k, l] → [i, j, k, l]`) are respected.
- **Storage**: The tensor is likely stored in memory as a contiguous array with strides for each dimension. For example:
  - Stride for `i`: \(256 \times 256 \times 16 = 1,048,576\) elements.
  - Stride for `j`: \(256 \times 16 = 4,096\) elements.
  - Stride for `k`: \(16\) elements.
  - Stride for `l`: \(1\) element.
  - Element at `[i, j, k, l]` is at offset:
    \[
    i \times 1,048,576 + j \times 4,096 + k \times 16 + l
    \]

- **Testing**: When testing your DFG augmentation and scheduling, you might create tensors like this to represent operator outputs. Ensure the tensor’s shape matches the index space, and verify that SURE dependencies access valid indices within the shape.

---

### 5. Visualizing the Tensor
To make the shape more concrete, here’s how you might visualize `tensor<4x256x256x16xf32>`:
- **As a 4D array**:
  - 4 "blocks" (one for each `i`).
  - Each block is a 256x256x16 "cube".
  - Each cube contains 256x256 "rows" of 16-element vectors.
- **As a data structure** (in C++):
  ```cpp
  using Tensor4D = std::vector<std::vector<std::vector<std::vector<float>>>>;
  Tensor4D tensor(4, std::vector<std::vector<std::vector<float>>>(
                     256, std::vector<std::vector<float>>(
                          256, std::vector<float>(16))));
  // Access: tensor[i][j][k][l]
  ```
  (In practice, use a flat array or a library like Eigen or PyTorch for efficiency.)

- **As a mental image**:
  - Imagine 4 stacks of 256x256 images, where each pixel in each image has 16 values (e.g., 16 colors or features).
  - Or picture a 256x256 grid evolving over 4 time steps, with 16 measurements per grid point.

---

### 6. Matrix Slices vs. Holistic View
To address your question directly:
- **Are tensors matrix slice-based?**
  - They *can* be viewed as collections of matrix slices (e.g., fixing two dimensions to get 256x256 matrices). This is common in deep learning (e.g., extracting feature maps) or when parallelizing computations (e.g., assigning 256x256 slices to threads).
  - However, this view is a projection of the full tensor. The tensor’s true structure is 4D, and operations (like SURE dependencies or scheduling) often involve all dimensions simultaneously.
- **Is there a better way?**
  - Yes, the **index space** and **application-specific** views are more powerful:
    - **Index Space**: Treat the tensor as a set of points `{[i, j, k, l]}` with associated values, matching your SURE and DFG framework.
    - **Application-Specific**: Assign meanings like time steps, spatial grid, or operator outputs to make the shape intuitive.
  - The **nested array** or **geometric** views are also useful for general understanding, especially when writing or debugging code.

---

### 7. Practical Tips for Your Project
- **Define Dimension Semantics**: Decide what each dimension (4, 256, 256, 16) represents in your DFG (e.g., iterations, grid size, channels). This will guide your SURE definitions and scheduling.
- **Use a Tensor Library**: Libraries like **Eigen**, **Armadillo**, or **PyTorch** (if you interface with Python) can manage tensor shapes efficiently and provide operations like slicing or reshaping.
- **Validate Shapes in Tests**: In your tests (from your earlier question), ensure the tensor shapes match the index spaces of operators. For example:
  ```cpp
  TEST(TensorShape, MatchesIndexSpace) {
      DFG dfg = createSingleNodeDFG("stencil", "{[i,j] : 0 <= i,j < 256}");
      Tensor4D tensor = dfg.nodes[0].output_tensor; // Hypothetical
      ASSERT_EQ(tensor.shape(), std::vector<size_t>{4, 256, 256, 16});
  }
  ```
- **Handle Slicing in SURE**: If your SURE computations involve slices (e.g., `tensor[i, :, :, l]`), ensure the slice indices are valid and respect the tensor’s shape.
- **Visualize for Debugging**: Use tools like Python’s **NumPy** or **Matplotlib** to visualize tensor slices (e.g., plot a 256x256 slice) during debugging.

---

### 8. Example in Context
Suppose your DFG represents a 2D stencil computation over a 256x256 grid for 4 time steps, with 16 output values per point (e.g., different physical quantities). The tensor `tensor<4x256x256x16xf32>` might store the results:
- **Shape**: `(4, 256, 256, 16)`.
- **Meaning**:
  - `i`: Time step (0 to 3).
  - `j, k`: Spatial coordinates on a 256x256 grid.
  - `l`: Output channel (e.g., temperature, pressure, etc.).
- **SURE**: For a stencil operator:
  \[
  tensor[i, j, k, l] = f(tensor[i-1, j-1, k, l], tensor[i-1, j+1, k, l], ...)
  \]
  The tensor’s shape constrains the valid indices (e.g., `i ≥ 1` for dependencies on `i-1`).
- **Scheduling**: You compute a scheduling vector `s = [s1, s2, s3, s4]` for index points `[i, j, k, l]`. The tensor’s shape defines the domain of computation, and the scheduling cone ensures dependencies like `[i-1, j, k, l] → [i, j, k, l]` are satisfied.

---

### 9. Conclusion
The shape of `tensor<4x256x256x16xf32>` is a 4D structure that can be viewed as:
- A **collection of matrix slices** (e.g., 256x256 matrices for fixed `i` and `l`), but this is a partial view.
- A **nested array** or **4D grid** for a general understanding.
- An **index space** `{[i, j, k, l]}` that aligns with your SURE and DFG computations.
- An **application-specific structure** (e.g., time steps, spatial grid, channels) that gives it meaning in your project.

The **index space** and **application-specific** perspectives are likely the most useful for your domain flow graph work, as they tie directly to your SURE definitions and scheduling algorithms. Avoid over-relying on matrix slices unless your operators explicitly work with 2D substructures.


## Batched Matrix multiply

To interpret a matrix multiplication (matmul) operation on 3D tensors, such as `tensor<batchSize, m, k>` times `tensor<batchSize, k, n>`, we need to understand how the operation generalizes from 2D matrix multiplication to 3D tensors. This is a common operation in deep learning, scientific computing, and potentially your domain flow graph project, where batched computations are used for efficiency or parallelism. Let’s break it down step-by-step, providing a clear and intuitive explanation of the operation, its result, and how it relates to your work with domain flow graphs and SUREs.

---

### 1. Background: 2D Matrix Multiplication
For 2D matrices:
- Let \( A \) be a matrix of shape \((m, k)\), i.e., \( m \) rows and \( k \) columns.
- Let \( B \) be a matrix of shape \((k, n)\), i.e., \( k \) rows and \( n \) columns.
- The matrix multiplication \( C = A \times B \) produces a matrix \( C \) of shape \((m, n)\).
- Each element of \( C \) is computed as:
  \[
  C[i, j] = \sum_{p=0}^{k-1} A[i, p] \cdot B[p, j]
  \]
  where:
  - \( i \) ranges from 0 to \( m-1 \) (rows of \( A \)).
  - \( j \) ranges from 0 to \( n-1 \) (columns of \( B \)).
  - \( p \) ranges from 0 to \( k-1 \) (summing over the inner dimension \( k \)).

The key requirement is that the inner dimensions match: the number of columns of \( A \) (\( k \)) equals the number of rows of \( B \) (\( k \)).

---

### 2. 3D Tensor Matrix Multiplication
Now, let’s extend this to 3D tensors:
- **Tensor \( A \)**: Shape `(batchSize, m, k)`, a 3D tensor.
- **Tensor \( B \)**: Shape `(batchSize, k, n)`, a 3D tensor.
- **Operation**: \( C = \text{matmul}(A, B) \).

#### Interpretation
A 3D tensor with a batch dimension can be thought of as a **stack of 2D matrices**:
- **Tensor \( A \)**: Contains `batchSize` matrices, each of shape \((m, k)\).
  - Think of \( A[b, :, :] \) as the \( b \)-th matrix of shape \((m, k)\), for \( b = 0 \) to `batchSize-1`.
- **Tensor \( B \)**: Contains `batchSize` matrices, each of shape \((k, n)\).
  - Think of \( B[b, :, :] \) as the \( b \)-th matrix of shape \((k, n)\).

The matrix multiplication on these 3D tensors is a **batched matrix multiplication**:
- For each batch index \( b \) (from 0 to `batchSize-1`):
  - Compute a 2D matrix multiplication between the \( b \)-th matrix of \( A \) and the \( b \)-th matrix of \( B \):
    \[
    C[b, :, :] = A[b, :, :] \times B[b, :, :]
    \]
  - Here, \( A[b, :, :] \) is an \((m, k)\) matrix, and \( B[b, :, :] \) is a \((k, n)\) matrix.
  - The result \( C[b, :, :] \) is a matrix of shape \((m, n)\).

- **Resulting Tensor \( C \)**: Shape `(batchSize, m, n)`:
  - It contains `batchSize` matrices, each of shape \((m, n)\).
  - Each element of \( C \) is computed as:
    \[
    C[b, i, j] = \sum_{p=0}^{k-1} A[b, i, p] \cdot B[b, p, j]
    \]
    where:
    - \( b \): Batch index, \( 0 \leq b < \text{batchSize} \).
    - \( i \): Row index, \( 0 \leq i < m \).
    - \( j \): Column index, \( 0 \leq j < n \).
    - \( p \): Summation index, \( 0 \leq p < k \).

#### Key Points
- **Batch Independence**: The matrix multiplications for each batch index \( b \) are independent. This makes batched matmul highly parallelizable (e.g., across GPUs or threads).
- **Dimension Matching**: The inner dimensions must match (\( k \) in \( A \)’s shape `(batchSize, m, k)` and \( B \)’s shape `(batchSize, k, n)`), just like in 2D matmul.
- **Shape Transformation**:
  - Input shapes: `(batchSize, m, k)` and `(batchSize, k, n)`.
  - Output shape: `(batchSize, m, n)`.
  - The batch dimension (`batchSize`) is preserved, while the inner dimension \( k \) is summed out.

---

### 3. Visualizing the Operation
To make this concrete, let’s visualize with example shapes, say `batchSize=4`, `m=2`, `k=3`, `n=5`:
- **Tensor \( A \)**: Shape `(4, 2, 3)`:
  - A stack of 4 matrices, each \( 2 \times 3 \).
  - Example:
    \[
    A[0, :, :] = \begin{bmatrix} a_{00} & a_{01} & a_{02} \\ a_{10} & a_{11} & a_{12} \end{bmatrix}, \quad A[1, :, :], \dots, A[3, :, :]
    \]
- **Tensor \( B \)**: Shape `(4, 3, 5)`:
  - A stack of 4 matrices, each \( 3 \times 5 \).
  - Example:
    \[
    B[0, :, :] = \begin{bmatrix} b_{00} & b_{01} & \dots & b_{04} \\ b_{10} & b_{11} & \dots & b_{14} \\ b_{20} & b_{21} & \dots & b_{24} \end{bmatrix}, \quad B[1, :, :], \dots
    \]
- **Operation**:
  - For each \( b = 0, 1, 2, 3 \):
    - Compute \( C[b, :, :] = A[b, :, :] \times B[b, :, :] \).
    - \( A[b, :, :] \) is \( 2 \times 3 \), \( B[b, :, :] \) is \( 3 \times 5 \), so \( C[b, :, :] \) is \( 2 \times 5 \).
    - Example for \( b = 0 \):
      \[
      C[0, i, j] = \sum_{p=0}^{2} A[0, i, p] \cdot B[0, p, j]
      \]
- **Result \( C \)**: Shape `(4, 2, 5)`:
  - A stack of 4 matrices, each \( 2 \times 5 \).
  - Example:
    \[
    C[0, :, :] = \begin{bmatrix} c_{00} & c_{01} & \dots & c_{04} \\ c_{10} & c_{11} & \dots & c_{14} \end{bmatrix}, \quad C[1, :, :], \dots
    \]

---

### 4. Application-Specific Interpretation
The batch dimension often has a specific meaning depending on the context. In your domain flow graph project, let’s consider possible interpretations:

#### In Deep Learning
- **Tensor \( A \)**: Shape `(batchSize, m, k)` might represent:
  - `batchSize`: Number of samples (e.g., 4 images or data points).
  - `m`: Number of output features or rows in a linear transformation.
  - `k`: Number of input features or dimensions.
  - Example: A batch of 4 data points, each transformed by a weight matrix.
- **Tensor \( B \)**: Shape `(batchSize, k, n)` might represent:
  - `batchSize`: Same batch of samples.
  - `k`: Input features (matching \( A \)).
  - `n`: Output features or dimensions.
  - Example: A batch of weight matrices or another set of features.
- **Operation**: The matmul computes a linear transformation for each sample in the batch, producing a new feature set of shape `(batchSize, m, n)`.
- Example: In a neural network layer, this could be a batched fully-connected layer or a transformation of feature maps.

#### In Your Domain Flow Graph and SURE Context
Since you’re working with domain flow graphs, index spaces, and SUREs, the 3D tensor matmul likely represents a computation over a batched index space. Let’s map it to your project:
- **Tensor \( A \)**: Shape `(batchSize, m, k)`:
  - `batchSize`: Could represent:
    - Different time steps, iterations, or operators in the DFG.
    - Parallel instances of the computation (e.g., different nodes or configurations).
  - `m, k`: Index space dimensions or feature dimensions for an operator.
  - Example: If an operator computes over an index space `{[i, j] : 0 ≤ i < m, 0 ≤ j < k}`, the tensor stores outputs for `batchSize` instances of this computation.
- **Tensor \( B \)**: Shape `(batchSize, k, n)`:
  - `batchSize`: Matches \( A \)’s batch dimension, ensuring corresponding matrices are multiplied.
  - `k, n`: Another index space or feature transformation.
  - Example: A transformation matrix or another operator’s output over `{[j, l] : 0 ≤ j < k, 0 ≤ l < n}`.
- **Result \( C \)**: Shape `(batchSize, m, n)`:
  - Represents the output of the matmul for each batch index.
  - Example: If \( A \) and \( B \) represent stencil outputs or intermediate computations, \( C \) is the combined result over the index space `{[i, l] : 0 ≤ i < m, 0 ≤ l < n}`.

- **SURE Connection**:
  - The matmul can be expressed as a SURE in your DFG. For index point `[b, i, j]` in \( C \):
    \[
    C[b, i, j] = \sum_{p=0}^{k-1} A[b, i, p] \cdot B[b, p, j]
    \]
    This is a recurrence where \( C[b, i, j] \) depends on values at indices `[b, i, p]` in \( A \) and `[b, p, j]` in \( B \). However, since the dependencies are not uniform (they involve a summation over \( p \)), this is a reduction rather than a uniform recurrence. You might approximate it as a SURE by unrolling the summation or treating it as a single operation.
  - The index space for \( C \) is:
    \[
    \{[b, i, j] : 0 \leq b < \text{batchSize}, 0 \leq i < m, 0 \leq j < n\}
    \]
  - Dependencies:
    - \( C[b, i, j] \) depends on \( A[b, i, p] \) and \( B[b, p, j] \) for all \( p \in [0, k) \).

- **Scheduling**:
  - When computing scheduling vectors for this matmul in your DFG, you need to ensure dependencies are respected. For example, \( A[b, i, p] \) and \( B[b, p, j] \) must be computed before \( C[b, i, j] \).
  - The scheduling vector \( s = [s_1, s_2, s_3] \) for index point `[b, i, j]` must satisfy constraints like:
    \[
    s \cdot ([b, i, j] - [b, i, p]) \geq 1 \quad \text{and} \quad s \cdot ([b, i, j] - [b, p, j]) \geq 1
    \]
    This ensures proper ordering in the parallel algorithm.

---

### 5. Implementation in C++
To implement this batched matmul in C++, you can:
- Use a **tensor library** like **Eigen**, **Armadillo**, or **PyTorch** (via C++ API) for efficient computation.
- Write a manual implementation for clarity or integration with your DFG.

Here’s a simplified manual implementation:

```cpp
#include <vector>

using Tensor3D = std::vector<std::vector<std::vector<float>>>;

// Batched matrix multiplication: (batchSize, m, k) x (batchSize, k, n) -> (batchSize, m, n)
Tensor3D batchedMatmul(const Tensor3D& A, const Tensor3D& B) {
    size_t batchSize = A.size();
    size_t m = A[0].size();
    size_t k = A[0][0].size();
    size_t n = B[0][0].size();
    
    // Verify shapes
    assert(B.size() == batchSize && B[0].size() == k);
    
    // Initialize result tensor C
    Tensor3D C(batchSize, std::vector<std::vector<float>>(
                   m, std::vector<float>(n, 0.0f)));
    
    // Compute batched matmul
    for (size_t b = 0; b < batchSize; ++b) {
        for (size_t i = 0; i < m; ++i) {
            for (size_t j = 0; j < n; ++j) {
                float sum = 0.0f;
                for (size_t p = 0; p < k; ++p) {
                    sum += A[b][i][p] * B[b][p][j];
                }
                C[b][i][j] = sum;
            }
        }
    }
    
    return C;
}
```

- **Notes**:
  - This is a naive implementation for clarity. Real-world implementations use optimized BLAS libraries (e.g., MKL, cuBLAS) or SIMD instructions.
  - In your DFG, you might store tensors in a flat array with strides for efficiency.
  - Add checks for shape compatibility and handle edge cases (e.g., `batchSize = 0`).

Using **Eigen** (more efficient):

```cpp
#include <Eigen/Dense>

Eigen::Tensor<float, 3> batchedMatmul(const Eigen::Tensor<float, 3>& A, 
                                     const Eigen::Tensor<float, 3>& B) {
    autoව

    auto batchSize = A.dimension(0);
    auto m = A.dimension(1);
    auto k = A.dimension(2);
    auto n = B.dimension(2);
    
    // Verify shapes
    assert(B.dimension(0) == batchSize && B.dimension(1) == k);
    
    // Initialize result tensor
    Eigen::Tensor<float, 3> C(batchSize, m, n);
    
    // Perform batched matmul
    for (int b = 0; b < batchSize; ++b) {
        Eigen::Map<const Eigen::MatrixXf> A_b(A.data() + b * m * k, m, k);
        Eigen::Map<const Eigen::MatrixXf> B_b(B.data() + b * k * n, k, n);
        Eigen::Map<Eigen::MatrixXf> C_b(C.data() + b * m * n, m, n);
        C_b = A_b * B_b;
    }
    
    return C;
}
```

- **Eigen** handles memory alignment and optimizations automatically.
- Assumes row-major storage; adjust for column-major if needed.

---

### 6. Testing the Matmul
To test this in your DFG framework (as per your earlier question), you can create test cases for the matmul operator:

```cpp
#include <gtest/gtest.h>

TEST(Matmul3D, BatchedMatmul) {
    // Create input tensors
    Tensor3D A(2, std::vector<std::vector<float>>(
                  2, std::vector<float>(3, 1.0f))); // Shape (2, 2, 3)
    Tensor3D B(2, std::vector<std::vector<float>>(
                  3, std::vector<float>(4, 1.0f))); // Shape (2, 3, 4)
    
    // Compute matmul
    Tensor3D C = batchedMatmul(A, B); // Expected shape (2, 2, 4)
    
    // Verify shape
    ASSERT_EQ(C.size(), 2);
    ASSERT_EQ(C[0].size(), 2);
    ASSERT_EQ(C[0][0].size(), 4);
    
    // Verify values (each element should be sum of 3 ones = 3.0)
    for (size_t b = 0; b < 2; ++b) {
        for (size_t i = 0; i < 2; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                ASSERT_FLOAT_EQ(C[b][i][j], 3.0f);
            }
        }
    }
}
```

- **Test Cases**:
  - **Valid Shapes**: Test various `batchSize`, `m`, `k`, `n`.
  - **Edge Cases**: Test `batchSize=1`, `m=1`, `k=0` (should handle gracefully).
  - **SURE Integration**: Augment a DFG node with this matmul operator and verify its SURE dependencies.

---

### 7. Practical Tips for Your Project
- **Index Space**: Represent the matmul’s index space in your DFG:
  \[
  \{[b, i, j] : 0 \leq b < \text{batchSize}, 0 \leq i < m, 0 \leq j < n\}
  \]
  with summation over \( p \).
- **SURE Augmentation**: Define the matmul’s SURE in your DFG node:
  - Dependencies: \( C[b, i, j] \) depends on \( A[b, i, p] \) and \( B[b, p, j] \) for all \( p \).
  - Note: The summation makes this a reduction, not a uniform recurrence. You may need to model it as a single operation or unroll the loop.
- **Scheduling**: Compute scheduling vectors ensuring \( A[b, i, p] \) and \( B[b, p, j] \) are scheduled before \( C[b, i, j] \). Use ISL to solve the constraint system:
  \[
  s \cdot ([b, i, j] - [b, i, p]) \geq 1, \quad s \cdot ([b, i, j] - [b, p, j]) \geq 1
  \]
- **Tensor Storage**: Store \( A \), \( B \), and \( C \) as flat arrays with strides for efficiency, matching your tensor<...> notation.
- **Performance**: For large tensors, use optimized libraries (e.g., Eigen, MKL) to handle the matmul, as naive loops are slow.

---

### 8. Conclusion
The matmul of 3D tensors `tensor<batchSize, m, k>` and `tensor<batchSize, k, n>` is a **batched matrix multiplication**:
- It performs a 2D matmul for each of the `batchSize` pairs of matrices.
- Input shapes: `(batchSize, m, k)` and `(batchSize, k, n)`.
- Output shape: `(batchSize, m, n)`.
- Each element \( C[b, i, j] = \sum_{p=0}^{k-1} A[b, i, p] \cdot B[b, p, j] \).

In your domain flow graph:
- The batch dimension might represent time steps, operators, or parallel instances.
- The matmul is a reduction operation, which you can model in your SURE framework.
- Scheduling ensures dependency ordering, using your polyhedral tools (e.g., ISL).

If you need help with:
- Integrating this matmul into your DFG code.
- Defining the SURE for matmul in ISL.
- Optimizing the implementation for large tensors.
- Creating more test cases tied to your operators.

please provide more details (e.g., specific operator types, tensor sizes, or DFG structure), and I can tailor the response further!


