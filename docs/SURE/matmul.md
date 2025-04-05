# System of Recurrence Equations Matmul and Tensormul

Let’s expand the System of Uniform Recurrence Equations (SURE) for matrix multiplication (matmul) that you provided. We’ll first generalize the 2D case from `N x N` matrices to an `M x N` matrix multiplied by an `N x O` matrix, and then extend it to a 3D tensor multiplication in a Deep Neural Network (DNN) style, which typically involves batched matrix multiplication or tensor contractions.

### Original SURE for Matmul (N x N):
Your original example:
```
system((i,j,k) | 0 <= i,j,k < N) {
    a(i,j,k) = a(i,j-1,k);
    b(i,j,k) = b(i-1,j,k);
    c(i,j,k) = c(i,j,k-1) + a(i,j-1,k)*b(i-1,j,k);
}
```
- `a` is the first matrix (`N x N`), shifted along `j`.
- `b` is the second matrix (`N x N`), shifted along `i`.
- `c` is the output (`N x N`), accumulating along `k`.
- Result: `C(i,j) = sum_k A(i,k) * B(k,j)`.

---

### 1. Generalizing to 2D Matmul (M x N × N x O)

For an `M x N` matrix `A` multiplied by an `N x O` matrix `B`, the output `C` is `M x O`. Each element `C(i,j)` is the dot product of the `i`-th row of `A` (size `N`) and the `j`-th column of `B` (size `N`).

#### Domain:
- `i`: Rows of `A` and `C`, `0 <= i < M`.
- `j`: Columns of `B` and `C`, `0 <= j < O`.
- `k`: Columns of `A` and rows of `B`, `0 <= k < N`.
- Total domain: `(i,j,k) | 0 <= i < M, 0 <= j < O, 0 <= k < N`.

#### Variables:
- `A(i,k)`: Element of the first matrix.
- `B(k,j)`: Element of the second matrix.
- `C(i,j)`: Element of the output matrix.

#### SURE:
```
system((i,j,k) | 0 <= i < M, 0 <= j < O, 0 <= k < N) {
    a(i,j,k) = a(i,j-1,k);           // Propagate A(i,k) along j
    b(i,j,k) = b(i-1,j,k);           // Propagate B(k,j) along i
    c(i,j,k) = (k == 0) ? 
                a(i,j-1,k) * b(i-1,j,k) : 
                c(i,j,k-1) + a(i,j-1,k) * b(i-1,j,k);  // Accumulate along k
}
```

#### Boundary Conditions:
- `a(i,j,-1)` = `A(i,0)` (initial value for `k=0`, propagated along `j`).
- `b(-1,j,k)` = `B(k,j)` (initial value for `i=0`, propagated along `i`).
- `c(i,j,-1)` = 0 (initial sum before `k=0`).

#### Output:
- `C(i,j) = c(i,j,N-1)` (final accumulated value after `k` reaches `N-1`).

#### Explanation:
- `a(i,j,k)` carries `A(i,k)` across all `j` positions for a fixed `i,k`.
- `b(i,j,k)` carries `B(k,j)` across all `i` positions for a fixed `k,j`.
- `c(i,j,k)` computes the partial sum up to index `k`, with the base case at `k=0` starting the accumulation.
- The result `C(i,j)` is the dot product `sum_k A(i,k) * B(k,j)`.

---

### 2. Generalizing to 3D Tensor Multiply (DNN Style)

In DNNs, matrix multiplication often occurs in a batched form, where we multiply a 3D tensor `A` of shape `B x M x N` (batch size `B`, rows `M`, columns `N`) by a 3D tensor `B` of shape `B x N x O` (batch size `B`, rows `N`, columns `O`), producing a 3D output tensor `C` of shape `B x M x O`. Alternatively, `B` could be a shared weight tensor of shape `N x O` (no batch dimension), as in a fully connected layer. I’ll provide both variants.

#### Variant 1: Batched Matmul (B x M x N × B x N x O → B x M x O)
This is common in DNNs for batched matrix multiplications (e.g., multi-head attention or batched fully connected layers).

##### Domain:
- `b`: Batch index, `0 <= b < B`.
- `i`: Rows of `A` and `C`, `0 <= i < M`.
- `j`: Columns of `B` and `C`, `0 <= j < O`.
- `k`: Columns of `A` and rows of `B`, `0 <= k < N`.
- Total domain: `(b,i,j,k) | 0 <= b < B, 0 <= i < M, 0 <= j < O, 0 <= k < N`.

##### Variables:
- `A(b,i,k)`: Element of the first tensor.
- `B(b,k,j)`: Element of the second tensor.
- `C(b,i,j)`: Element of the output tensor.

##### SURE:
```
system((b,i,j,k) | 0 <= b < B, 0 <= i < M, 0 <= j < O, 0 <= k < N) {
    a(b,i,j,k) = a(b,i,j-1,k);           // Propagate A(b,i,k) along j
    b(b,i,j,k) = b(b,i-1,j,k);           // Propagate B(b,k,j) along i
    c(b,i,j,k) = (k == 0) ? 
                  a(b,i,j-1,k) * b(b,i-1,j,k) : 
                  c(b,i,j,k-1) + a(b,i,j-1,k) * b(b,i-1,j,k);  // Accumulate along k
}
```

##### Boundary Conditions:
- `a(b,i,j,-1)` = `A(b,i,0)` (initial value for `k=0`).
- `b(b,-1,j,k)` = `B(b,k,j)` (initial value for `i=0`).
- `c(b,i,j,-1)` = 0 (initial sum before `k=0`).

##### Output:
- `C(b,i,j) = c(b,i,j,N-1)`.

##### Explanation:
- Adds a batch dimension `b` to all variables.
- Each batch `b` computes an independent `M x N × N x O` matmul.
- Uniform dependencies remain the same as the 2D case, extended with `b`.

#### Variant 2: DNN Fully Connected Layer (B x M x N × N x O → B x M x O)
Here, `A` is a batched input tensor (`B x M x N`), and `B` is a weight matrix (`N x O`) shared across batches, common in fully connected layers.

##### Domain:
- `b`: Batch index, `0 <= b < B`.
- `i`: Rows of `A` and `C`, `0 <= i < M`.
- `j`: Columns of `B` and `C`, `0 <= j < O`.
- `k`: Columns of `A` and rows of `B`, `0 <= k < N`.
- Total domain: `(b,i,j,k) | 0 <= b < B, 0 <= i < M, 0 <= j < O, 0 <= k < N`.

##### Variables:
- `A(b,i,k)`: Element of the input tensor.
- `B(k,j)`: Element of the weight matrix (no batch dimension).
- `C(b,i,j)`: Element of the output tensor.

##### SURE:
```
system((b,i,j,k) | 0 <= b < B, 0 <= i < M, 0 <= j < O, 0 <= k < N) {
    a(b,i,j,k) = a(b,i,j-1,k);           // Propagate A(b,i,k) along j
    b(b,i,j,k) = b(b,i-1,j,k);           // Propagate B(k,j) along i
    c(b,i,j,k) = (k == 0) ? 
                  a(b,i,j-1,k) * b(b,i-1,j,k) : 
                  c(b,i,j,k-1) + a(b,i,j-1,k) * b(b,i-1,j,k);  // Accumulate along k
}
```

##### Boundary Conditions:
- `a(b,i,j,-1)` = `A(b,i,0)` (initial value for `k=0`).
- `b(b,-1,j,k)` = `B(k,j)` (initial value for `i=0`, shared across batches).
- `c(b,i,j,-1)` = 0 (initial sum before `k=0`).

##### Output:
- `C(b,i,j) = c(b,i,j,N-1)`.

##### Explanation:
- `B(k,j)` has no `b` dimension, reflecting a shared weight matrix.
- Each batch `b` multiplies its `M x N` input slice by the same `N x O` weight matrix.

---

### Notes:
- **Traversal Order**: Both 3D variants accumulate along `k`, propagate `a` along `j`, and `b` along `i`. You could adjust this (e.g., accumulate along `j`) depending on optimization needs.
- **Further Generalization**: For higher-dimensional tensors (e.g., `B x T x M x N × N x O`), add more indices and adjust dependencies accordingly.
- **DNN Context**: Variant 2 is more typical for fully connected layers, while Variant 1 suits batched operations like attention mechanisms.

## Expanding to 4D and 5D tensor multiplication

Let’s extend the matrix multiplication SURE further to 4D and 5D tensor multiplications, building on the previous examples. In higher dimensions, tensor multiplication typically involves contracting over one or more shared dimensions, akin to batched matrix multiplications or tensor contractions in Deep Neural Networks (DNNs). I’ll assume a DNN-style context where additional dimensions represent batches, heads, or sequence lengths, and I’ll define configurations that generalize naturally from the 3D cases.

---

### 4D Tensor Multiplication

For a 4D case, consider a scenario common in DNNs like multi-head attention (e.g., in Transformers), where we multiply a tensor `A` of shape `B x H x M x N` (batch size `B`, heads `H`, rows `M`, columns `N`) by a tensor `B` of shape `B x H x N x O` (batch size `B`, heads `H`, rows `N`, columns `O`). The output `C` is `B x H x M x O`, contracting over the `N` dimension.

#### Domain:
- `b`: Batch index, `0 <= b < B`.
- `h`: Head index, `0 <= h < H`.
- `i`: Rows of `A` and `C`, `0 <= i < M`.
- `j`: Columns of `B` and `C`, `0 <= j < O`.
- `k`: Columns of `A` and rows of `B`, `0 <= k < N`.
- Total domain: `(b,h,i,j,k) | 0 <= b < B, 0 <= h < H, 0 <= i < M, 0 <= j < O, 0 <= k < N`.

#### Variables:
- `A(b,h,i,k)`: Element of the first tensor.
- `B(b,h,k,j)`: Element of the second tensor.
- `C(b,h,i,j)`: Element of the output tensor.

#### SURE:
```
system((b,h,i,j,k) | 0 <= b < B, 0 <= h < H, 0 <= i < M, 0 <= j < O, 0 <= k < N) {
    a(b,h,i,j,k) = a(b,h,i,j-1,k);           // Propagate A(b,h,i,k) along j
    b(b,h,i,j,k) = b(b,h,i-1,j,k);           // Propagate B(b,h,k,j) along i
    c(b,h,i,j,k) = (k == 0) ? 
                    a(b,h,i,j-1,k) * b(b,h,i-1,j,k) : 
                    c(b,h,i,j,k-1) + a(b,h,i,j-1,k) * b(b,h,i-1,j,k);  // Accumulate along k
}
```

#### Boundary Conditions:
- `a(b,h,i,j,-1)` = `A(b,h,i,0)` (initial value for `k=0`).
- `b(b,h,-1,j,k)` = `B(b,h,k,j)` (initial value for `i=0`).
- `c(b,h,i,j,-1)` = 0 (initial sum before `k=0`).

#### Output:
- `C(b,h,i,j) = c(b,h,i,j,N-1)`.

#### Explanation:
- Adds a head dimension `h` to the 3D batched matmul.
- Each `(b,h)` pair computes an independent `M x N × N x O` matmul.
- This mirrors multi-head attention where each head processes a separate matrix multiplication per batch.

#### Alternative 4D Interpretation:
If `B` were a weight tensor `H x N x O` (shared across batches), the SURE would adjust `b(b,h,i,j,k)` to depend only on `(h,k,j)`, similar to the 3D fully connected case. Let me know if you’d prefer that variant!

---

### 5D Tensor Multiplication

For a 5D case, let’s model a scenario like batched, multi-head, sequence-based multiplication, common in Transformer models with sequence data. Multiply `A` of shape `B x T x H x M x N` (batch size `B`, sequence length `T`, heads `H`, rows `M`, columns `N`) by `B` of shape `B x T x H x N x O` (batch size `B`, sequence length `T`, heads `H`, rows `N`, columns `O`). The output `C` is `B x T x H x M x O`, contracting over `N`.

#### Domain:
- `b`: Batch index, `0 <= b < B`.
- `t`: Sequence/time index, `0 <= t < T`.
- `h`: Head index, `0 <= h < H`.
- `i`: Rows of `A` and `C`, `0 <= i < M`.
- `j`: Columns of `B` and `C`, `0 <= j < O`.
- `k`: Columns of `A` and rows of `B`, `0 <= k < N`.
- Total domain: `(b,t,h,i,j,k) | 0 <= b < B, 0 <= t < T, 0 <= h < H, 0 <= i < M, 0 <= j < O, 0 <= k < N`.

#### Variables:
- `A(b,t,h,i,k)`: Element of the first tensor.
- `B(b,t,h,k,j)`: Element of the second tensor.
- `C(b,t,h,i,j)`: Element of the output tensor.

#### SURE:
```
system((b,t,h,i,j,k) | 0 <= b < B, 0 <= t < T, 0 <= h < H, 0 <= i < M, 0 <= j < O, 0 <= k < N) {
    a(b,t,h,i,j,k) = a(b,t,h,i,j-1,k);           // Propagate A(b,t,h,i,k) along j
    b(b,t,h,i,j,k) = b(b,t,h,i-1,j,k);           // Propagate B(b,t,h,k,j) along i
    c(b,t,h,i,j,k) = (k == 0) ? 
                      a(b,t,h,i,j-1,k) * b(b,t,h,i-1,j,k) : 
                      c(b,t,h,i,j,k-1) + a(b,t,h,i,j-1,k) * b(b,t,h,i-1,j,k);  // Accumulate along k
}
```

#### Boundary Conditions:
- `a(b,t,h,i,j,-1)` = `A(b,t,h,i,0)` (initial value for `k=0`).
- `b(b,t,h,-1,j,k)` = `B(b,t,h,k,j)` (initial value for `i=0`).
- `c(b,t,h,i,j,-1)` = 0 (initial sum before `k=0`).

#### Output:
- `C(b,t,h,i,j) = c(b,t,h,i,j,N-1)`.

#### Explanation:
- Adds a sequence dimension `t` to the 4D case.
- Each `(b,t,h)` triple computes an independent `M x N × N x O` matmul.
- This could represent attention over a sequence, where each time step and head processes a separate matrix multiplication per batch.

#### Alternative 5D Interpretation:
If `B` were a weight tensor `H x N x O` (shared across batch and sequence), `b(b,t,h,i,j,k)` would depend only on `(h,k,j)`. Another option is a different contraction, e.g., `B x T x H x M x N × T x H x N x O`, contracting over `T, N`. Let me know if you’d like a specific variant!

---

### General Observations:
- **Pattern**: Each additional dimension (e.g., `h`, `t`) adds an index to the domain and variables, but the core matmul pattern (`i,j,k` with accumulation over `k`) remains unchanged.
- **Uniformity**: Dependencies are uniform: `a` shifts by `(0,0,0,0,-1,0)`, `b` by `(0,0,0,-1,0,0)`, `c` by `(0,0,0,0,0,-1)`.
- **DNN Context**: These align with batched, multi-head, or sequence-based operations in DNNs.
