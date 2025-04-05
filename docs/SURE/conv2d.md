# System of Uniform Recurrence Equations for a Conv2D operator

To generate a System of Uniform Recurrence Equations (SURE) for a 2D convolution (Conv2D) operator with a 3x3 filter kernel and padding of [1,1], we need to define the computation in terms of uniform dependencies, similar to the matrix multiplication example you provided. A SURE expresses computations as recurrence relations over a domain, with uniform shifts in the indices.

### Problem Setup:
- **Input**: An input feature map `I` of size `N x N` (assuming a single channel for simplicity; multi-channel can be extended later).
- **Filter**: A 3x3 kernel `W` with weights `W(h,w)` where `h, w ∈ {-1, 0, 1}` (centered at (0,0)).
- **Padding**: [1,1] means 1 pixel of zero-padding on all sides, so the input is effectively treated as `(N+2) x (N+2)` with zeros outside the `N x N` region, and the output size remains `N x N` (valid convolution with padding).
- **Output**: An output feature map `O` of size `N x N`.

In a Conv2D, each output element `O(i,j)` is computed as the sum of element-wise multiplications of the 3x3 kernel with the corresponding 3x3 region of the padded input centered at `(i,j)`. We’ll express this in a SURE form by introducing auxiliary variables and uniform dependencies.

### SURE Formulation:
For Conv2D, we need to:
1. Slide the 3x3 kernel over the input.
2. Compute partial sums incrementally across the kernel window.
3. Handle padding implicitly by defining the input as zero outside the valid range.

Let’s define the domain and equations:

#### Domain:
- The computation occurs over indices `(i, j, k, l)` where:
  - `i, j` are the output coordinates: `0 <= i, j < N`.
  - `k, l` are the kernel offsets: `-1 <= k, l <= 1` (for a 3x3 kernel).
- Total domain: `(i, j, k, l) | 0 <= i, j < N, -1 <= k, l <= 1`.

#### Variables:
- `I(i+k, j+l)`: Input value at the shifted position (with padding handled implicitly).
- `W(k,l)`: Kernel weight at offset `(k,l)`.
- `P(i,j,k,l)`: Partial sum accumulating the convolution contributions.
- `O(i,j)`: Final output (can be derived from the partial sums).

#### Recurrence Equations:
We’ll accumulate the convolution sum incrementally. The key is to define uniform dependencies, meaning each variable depends on a fixed offset of itself or another variable.

Here’s the SURE:

```
system((i, j, k, l) | 0 <= i, j < N, -1 <= k, l <= 1) {
    // Input with padding: zero outside [0,N-1]
    I_padded(i, j) = (i < 0 || i >= N || j < 0 || j >= N) ? 0 : I(i, j);
    
    // Partial sum: accumulate contributions from the 3x3 window
    P(i, j, k, l) = (k == -1 && l == -1) ? 
                      I_padded(i+k, j+l) * W(k,l) :
                      P(i, j, k-1, l) + I_padded(i+k, j+l) * W(k,l)  if k > -1,
                      P(i, j, k, l-1) + I_padded(i+k, j+l) * W(k,l)  if k == -1 && l > -1;
    
    // Output: final result when k=1, l=1
    O(i, j) = P(i, j, 1, 1);
}
```

### Explanation:
1. **Input Padding (`I_padded`)**:
   - Defines the padded input: returns `I(i,j)` if within bounds `[0,N-1]`, otherwise `0`.
   - This handles the [1,1] padding requirement.

2. **Partial Sum (`P`)**:
   - `P(i,j,k,l)` represents the partial convolution sum up to kernel position `(k,l)`.
   - **Base Case**: At `(k=-1, l=-1)` (top-left of the 3x3 kernel), start with the first contribution: `I_padded(i-1,j-1) * W(-1,-1)`.
   - **Recurrence**:
     - If `k > -1`, depend on the previous `k` position: `P(i,j,k-1,l)`.
     - If `k = -1` and `l > -1`, depend on the previous `l` position: `P(i,j,k,l-1)`.
     - Add the current contribution: `I_padded(i+k,j+l) * W(k,l)`.

3. **Output (`O`)**:
   - The final output `O(i,j)` is the partial sum when the kernel has been fully traversed, i.e., `P(i,j,1,1)`.

### Uniformity:
- Dependencies are uniform because:
  - `P(i,j,k,l)` depends on `P(i,j,k-1,l)` (shift of `(0,0,-1,0)`) or `P(i,j,k,l-1)` (shift of `(0,0,0,-1)`).
  - `I_padded` and `W` are inputs with no recurrence.

### Notes:
- This assumes a row-major traversal of the kernel (left-to-right, then top-to-bottom). You could adjust the dependency order (e.g., column-major) if needed.
- For multi-channel inputs or outputs, you’d add a channel dimension and sum over it, but this is the single-channel case for simplicity.
- The `if` conditions in the SURE syntax might need adjustment based on the exact SURE formalism you’re using; some systems use guards or separate equations.

### Verification:
For `N=2`, output `O(0,0)` would compute:
- Sum over `k,l ∈ {-1,0,1}` of `I_padded(0+k,0+l) * W(k,l)`.
- With padding, `I_padded(-1,-1) = 0`, `I_padded(-1,0) = 0`, etc., matching a 3x3 conv with [1,1] padding.

# Extending it to _C_ channels

To extend the System of Uniform Recurrence Equations (SURE) for a Conv2D operator to include `C` input channels, we need to account for the fact that a 2D convolution with multiple channels sums the contributions of the 3x3 kernel across all input channels for each output position. The filter becomes a 4D tensor (`C x 3 x 3` for input channels `C`, assuming a single output channel for simplicity), and the input feature map becomes a 3D tensor (`C x N x N`).

### Updated Problem Setup:
- **Input**: An input feature map `I` of size `C x N x N` (C channels, N x N spatial dimensions).
- **Filter**: A 3x3 kernel `W` of size `C x 3 x 3` (C input channels, 3x3 spatial dimensions, assuming 1 output channel).
- **Padding**: [1,1] zero-padding on the spatial dimensions, so the effective input size is `C x (N+2) x (N+2)`, and the output size remains `N x N`.
- **Output**: An output feature map `O` of size `N x N` (single output channel for now; multi-output channels can be added later).

The convolution now computes each output `O(i,j)` as the sum over all channels `c` and kernel positions `(k,l)` of the products `I(c,i+k,j+l) * W(c,k,l)`.

### SURE Formulation with Channels:

#### Domain:
- Indices: `(i, j, c, k, l)` where:
  - `i, j`: Output spatial coordinates, `0 <= i, j < N`.
  - `c`: Input channel index, `0 <= c < C`.
  - `k, l`: Kernel offsets, `-1 <= k, l <= 1` (3x3 kernel).
- Total domain: `(i, j, c, k, l) | 0 <= i, j < N, 0 <= c < C, -1 <= k, l <= 1`.

#### Variables:
- `I(c,i+k,j+l)`: Input value at channel `c` and shifted spatial position `(i+k,j+l)`.
- `W(c,k,l)`: Kernel weight for channel `c` at offset `(k,l)`.
- `P(i,j,c,k,l)`: Partial sum accumulating contributions across channels and kernel positions.
- `O(i,j)`: Final output when all channels and kernel positions are summed.

#### Recurrence Equations:
We’ll accumulate the sum incrementally across channels and kernel positions with uniform dependencies.

```
system((i, j, c, k, l) | 0 <= i, j < N, 0 <= c < C, -1 <= k, l <= 1) {
    // Input with padding: zero outside [0,N-1] spatially, valid for all channels
    I_padded(c, i, j) = (i < 0 || i >= N || j < 0 || j >= N) ? 0 : I(c, i, j);
    
    // Partial sum: accumulate contributions across channels and 3x3 window
    P(i, j, c, k, l) = 
        // Base case: start of channel 0, top-left of kernel
        (c == 0 && k == -1 && l == -1) ? 
            I_padded(c, i+k, j+l) * W(c, k, l) :
        // Same channel, previous k position
        (k > -1) ? 
            P(i, j, c, k-1, l) + I_padded(c, i+k, j+l) * W(c, k, l) :
        // k = -1, previous l position
        (k == -1 && l > -1) ? 
            P(i, j, c, k, l-1) + I_padded(c, i+k, j+l) * W(c, k, l) :
        // Previous channel, end of kernel
        (c > 0 && k == -1 && l == -1) ? 
            P(i, j, c-1, 1, 1) + I_padded(c, i+k, j+l) * W(c, k, l) :
        0;  // Default case (shouldn’t occur within domain)
    
    // Output: final result when c=C-1, k=1, l=1
    O(i, j) = P(i, j, C-1, 1, 1);
}
```

### Explanation:
1. **Input Padding (`I_padded`)**:
   - Extends to channels: `I_padded(c,i,j)` is `I(c,i,j)` if `(i,j)` is within `[0,N-1]`, otherwise `0`.
   - Padding applies only spatially, not across channels.

2. **Partial Sum (`P`)**:
   - `P(i,j,c,k,l)` accumulates the convolution sum up to channel `c` and kernel position `(k,l)`.
   - **Base Case**: At `(c=0, k=-1, l=-1)`, start with `I_padded(0,i-1,j-1) * W(0,-1,-1)`.
   - **Recurrence**:
     - Within a channel:
       - If `k > -1`: Depend on `P(i,j,c,k-1,l)` (shift `(0,0,0,-1,0)`).
       - If `k = -1` and `l > -1`: Depend on `P(i,j,c,k,l-1)` (shift `(0,0,0,0,-1)`).
     - Across channels:
       - If `c > 0` and `(k=-1, l=-1)`: Depend on `P(i,j,c-1,1,1)` (shift `(0,0,-1,0,0)`), which is the final sum of the previous channel.
     - Add the current contribution: `I_padded(c,i+k,j+l) * W(c,k,l)`.

3. **Output (`O`)**:
   - `O(i,j)` is the partial sum when all channels and kernel positions are traversed: `P(i,j,C-1,1,1)`.

### Uniformity:
- Dependencies are uniform with fixed shifts:
  - `(0,0,0,-1,0)` for `k`.
  - `(0,0,0,0,-1)` for `l`.
  - `(0,0,-1,0,0)` for `c`.

### Example:
For `N=2`, `C=2`, `O(0,0)` computes:
- Sum over `c=0..1`, `k=-1..1`, `l=-1..1` of `I_padded(c,0+k,0+l) * W(c,k,l)`.
- `I_padded(0,-1,-1) = 0`, `I_padded(1,0,0) = I(1,0,0)`, etc., matching a multi-channel Conv2D with [1,1] padding.

### Notes:
- This assumes a single output channel. For `F` output channels, you’d need a filter `F x C x 3 x 3` and an additional index `f` in the SURE, with `O(f,i,j) = P(f,i,j,C-1,1,1)`.
- The traversal order here is channel-first, then row-major kernel. You could adjust to kernel-first if preferred.

