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

The document originally expressed this with a conditional cascade — but the
cases take *different* dependence shifts in different subdomains, so the
cascade is not actually uniform, and `I_padded` is read pointwise at every
domain point, which the Domain Flow discipline forbids (data must *flow* in
through faces, not be sampled from an oracle). The refined, executable
formulation ([`conv2d.sure`](conv2d.sure)) uses four total recurrences:

```text
N = 4;

system ((i,j,k,l) | 0 <= i < N, 0 <= j < N, -1 <= k < 2, -1 <= l < 2) {
    x(i,j,k,l) = x(i-1,j,k+1,l);                          // Ipad(i+k, j+l) anti-diagonal flow
    w(i,j,k,l) = w(i-1,j,k,l);                            // W(k,l) broadcast along +i
    p(i,j,k,l) = p(i,j,k,l-1) + x(i,j,k,l) * w(i,j,k,l);  // row sum along l
    s(i,j,k,l) = s(i,j,k-1,l) + p(i,j,k,1);               // add completed rows along k
}

input Ipad[6][6] ((i,j,k,l) | -1 <= i < 3, 0 <= j < N, -1 <= l < 2, k = 2)  : x(i,j,k,l) = Ipad[i+3][j+l+1];
input            ((i,j,k,l) | 0 <= j < N, 0 <= k < 2, -1 <= l < 2, i = -1)  : x(i,j,k,l) = Ipad[k][j+l+1];
input W[3][3]    ((i,j,k,l) | 0 <= j < N, -1 <= k < 2, -1 <= l < 2, i = -1) : w(i,j,k,l) = W[k+1][l+1];
input            ((i,j,k,l) | 0 <= i < N, 0 <= j < N, -1 <= k < 2, l = -2)  : p(i,j,k,l) = 0;
input            ((i,j,k,l) | 0 <= i < N, 0 <= j < N, -1 <= l < 2, k = -2)  : s(i,j,k,l) = 0;

output O[N][N] ((i,j,k,l) | 0 <= i < N, 0 <= j < N, 1 <= l < 2, k = 1) : O[i][j] = s(i,j,k,l);
```

### Explanation

1. **The image is a flow, not an oracle (`x`).** `x(i,j,k,l)` carries the
   padded-image value `Ipad(i+k, j+l)`. Because `(i-1) + (k+1) = i + k`, the
   uniform shift `(-1,0,+1,0)` walks an anti-diagonal along which the value
   is constant — each point simply forwards its neighbor. The walk is seeded
   from two *disjoint* halo faces that together cover every escape: the
   `k = 2` plane (rows arriving from above) and the `i = -1` plane restricted
   to `0 <= k < 2` (the remaining kernel rows). Padding is explicit data: the
   test bench binds the `(N+2) x (N+2)` image with its zero ring, so
   "zero outside the valid range" is a property of the bound tensor, not a
   conditional in the kernel.

2. **The kernel weights broadcast (`w`).** `w(i,j,k,l) = w(i-1,j,k,l)`
   propagates `W(k,l)` across the image rows from the `i = -1` face.

3. **The reduction is separable and uniform (`p`, `s`).** `p` accumulates a
   window row along `l` (seeded to zero on the `l = -2` halo face); `s` adds
   the *completed* rows along `k`, fetching each finished row sum with the
   affine tap `p(i,j,k,1)`. This replaces the conditional cascade with two
   total recurrences whose dependences are fixed everywhere — at the cost of
   carrying `s` over the full `l` extent (uniformization slack).

4. **The output leaves through the terminal `k = 1` face** with outward
   normal `(0,0,1,0)`: `O(i,j) = s(i,j,1,1)`.

### Verification

`conv2d.sure` binds a 4x4 ramp image (values 1..16) inside an explicit zero
ring and the Sobel-x kernel. The test suite verifies all 16 outputs against
a direct correlation reference, e.g. `O(0,0) = -10`, `O(1,3) = 28`. Run it:

```console
$ dfactl --sure docs/SURE/conv2d.sure
  input  Ipad -> x  normal (0,0,1,0)
  input  Ipad -> x  normal (-1,0,0,0)
  input  W -> w  normal (-1,0,0,0)
  ...
  output O <- s  normal (0,0,1,0)
```

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

```text
system ((i,j,c,k,l) | 0 <= i,j < N, 0 <= c < C, -1 <= k,l <= 1) {
    x(i,j,c,k,l) = x(i-1,j,c,k+1,l);                            // Ipad(c, i+k, j+l) flow
    w(i,j,c,k,l) = w(i-1,j,c,k,l);                              // W(c,k,l) broadcast
    p(i,j,c,k,l) = p(i,j,c,k,l-1) + x(i,j,c,k,l) * w(i,j,c,k,l);   // row sum along l
    s(i,j,c,k,l) = s(i,j,c,k-1,l) + p(i,j,c,k,1);               // rows along k
    t(i,j,c,k,l) = t(i,j,c-1,k,l) + s(i,j,c,1,l);               // channels along c
}
// output: O(i,j) = t(i,j,C-1,1,1) through the terminal c = C-1 face
```

### Explanation:
1. **Image and kernel flows (`x`, `w`)**: as in the single-channel case, but
   with the channel index carried along. `x(i,j,c,k,l)` transports
   `Ipad(c, i+k, j+l)` on the value-preserving anti-diagonal shift
   `(-1,0,0,+1,0)`; `w(i,j,c,k,l)` broadcasts `W(c,k,l)` along `+i`. The
   halo faces bind the `C x (N+2) x (N+2)` padded input and the
   `C x 3 x 3` kernel per channel.

2. **Three-stage separable reduction (`p`, `s`, `t`)**:
   - `p` accumulates a window row along `l` (shift `(0,0,0,0,-1)`), seeded
     to zero on the `l = -2` halo face;
   - `s` adds the completed rows along `k` (shift `(0,0,0,-1,0)`), reading
     each finished row sum with the affine tap `p(i,j,c,k,1)`;
   - `t` adds the completed per-channel window sums along `c` (shift
     `(0,0,-1,0,0)`), reading each channel's total with the affine tap
     `s(i,j,c,1,l)` and seeded to zero on the `c = -1` halo face.

3. **Output (`O`)**: the fully reduced value leaves through the terminal
   `c = C-1` face: `O(i,j) = t(i,j,C-1,1,1)`.

### Uniformity:

- Every recurrence has a single fixed dependence everywhere in the domain:
  `(-1,0,0,+1,0)` for `x`, `(-1,0,0,0,0)` for `w`, `(0,0,0,0,-1)` for `p`,
  `(0,0,0,-1,0)` for `s`, and `(0,0,-1,0,0)` for `t`; the stage hand-offs
  (`p` at `l = 1`, `s` at `k = 1`) are affine broadcast taps.

### Example:

For `N=2`, `C=2`, `O(0,0)` computes the sum over `c=0..1`, `k,l=-1..1` of
`Ipad(c, 0+k, 0+l) * W(c,k,l)`, with the zero ring of the bound padded
tensor supplying `Ipad(0,-1,-1) = 0` etc. -- matching a multi-channel
Conv2D with [1,1] padding.

### Notes:

- This assumes a single output channel. For `F` output channels, you’d need a filter `F x C x 3 x 3` and an additional index `f` in the SURE, with `O(f,i,j) = t(f,i,j,C-1,1,1)`.
- The traversal order here is channel-first, then row-major kernel. You could adjust to kernel-first if preferred.

