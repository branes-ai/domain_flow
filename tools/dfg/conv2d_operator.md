# Conv2D operator

Let's break down the arithmetic complexity of the TOSA `conv2d` operation, focusing on the specific parameters:

```
%myConv2dOp = tosa.conv2d %arg0, %in1, %in2 {dilation = array<i64: 1, 1>, pad = array<i64: 0, 1, 0, 1>, stride = array<i64: 2, 2>} : (tensor<?x224x224x3xf32>, tensor<32x3x3x3xf32>, tensor<32xf32>) -> tensor<?x112x112x32xf32>
```

Here's a breakdown of the parameters and the resulting arithmetic operations:

**Operands:**

* **Input Tensor (%arg0):** `?x224x224x3xf32` (Unknown batch size, 224x224 spatial dimensions, 3 input channels, float32 data type)
* **Weight Tensor (%105):** `32x3x3x3xf32` (32 output channels, 3x3 kernel size, 3 input channels, float32 data type)
* **Bias Tensor (%106):** `32xf32` (32 biases, one for each output channel, float32 data type)

**Attributes:**

* **Dilation:** `array<i64: 1, 1>` (No dilation)
* **Padding:** `array<i64: 0, 1, 0, 1>` (Padding of 0 on top, 1 on right, 0 on bottom, 1 on left)
* **Stride:** `array<i64: 2, 2>` (Stride of 2 in both spatial dimensions)
* **Output Tensor (%107):** `?x112x112x32xf32` (Unknown batch size, 112x112 spatial dimensions, 32 output channels, float32 data type)

**Arithmetic Operations:**

1.  **Convolution:**
    * For each output pixel, we perform a series of multiply-accumulate (MAC) operations.
    * The kernel size is 3x3, and there are 3 input channels. Therefore, for each output pixel, we have 3x3x3 = 27 multiplications and 26 additions.
    * Since there are 32 output channels, we multiply this by 32.
    * The output spatial dimensions are 112x112.
    * Therefore, for one image in the batch, the number of MAC operations is: 112x112x32x3x3x3.
    * The total number of multiplications per image is: $112 \times 112 \times 32 \times 3 \times 3 \times 3 = 10,838,016$.
    * The total number of additions per image for the convolution is: $112 \times 112 \times 32 \times 26 = 10,436,608$.
2.  **Bias Addition:**
    * After the convolution, we add the bias to each output pixel.
    * There are 32 biases, and the output spatial dimensions are 112x112.
    * Therefore, for one image in the batch, the number of bias additions is: 112x112x32.
    * The total number of bias additions per image is: $112 \times 112 \times 32 = 401,408$.

**Total Arithmetic Complexity (per image):**

* **Multiplications:** $112 \times 112 \times 32 \times 3 \times 3 \times 3 = 10,838,016$
* **Additions:** $(112 \times 112 \times 32 \times 26) + (112 \times 112 \times 32) = 10,436,608 + 401,408 = 10,838,016$
* Total MACs: $10,838,016$
* Total additions: $10,838,016$

**Batch Size Consideration:**

* If the batch size is `B`, then the total arithmetic operations will be `B` times the above calculations.

**Padding and Stride Impact:**

* **Padding:** The padding affects the size of the input data used in the convolution, but it doesn't directly change the number of arithmetic operations per output pixel.
* **Stride:** The stride affects the size of the output spatial dimensions. A stride of 2 reduces the output dimensions by a factor of 2. This directly impacts the total number of arithmetic operations performed.

**Summary:**

The `tosa.conv2d` operation is computationally intensive due to the large number of MAC operations involved. The arithmetic complexity is primarily determined by the kernel size, the number of input and output channels, and the output spatial dimensions. The bias addition adds a relatively smaller number of additions. The batch size will multiply the total amount of computations.

# TOSA-compliant tensor

It's important to understand that TOSA (Tensor Operator Set Architecture) focuses on defining the *operations* on tensors, rather than prescribing rigid, fixed layouts for how those tensors are stored in memory. However, TOSA does operate on whole tensors, and when dealing with operations like `conv2d`, it inherently works with the concept of 4D tensors.

Here's a breakdown of how 4D tensors are generally understood within the context of TOSA and deep learning:

* **Common 4D Tensor Layout:**
    * In deep learning, and thus within the context of TOSA operations like `conv2d`, 4D tensors typically represent data with the following dimensions:
        * **Batch (N):** The number of samples in a batch.
        * **Height (H):** The vertical spatial dimension of the data.
        * **Width (W):** The horizontal spatial dimension of the data.
        * **Channels (C):** The number of feature channels.
    * Therefore, a 4D tensor is often represented as `(N, H, W, C)`.

* **TOSA and Operator Semantics:**
    * TOSA specifications define the *semantics* of operators that work on these tensors. For example, the `conv2d` operator expects input tensors that can be interpreted according to this 4D layout, so that the convolution operation can be performed correctly.
    * The TOSA specification is more focused on the operations that are performed on those tensors, and the numerical accuracy of those operations, than defining the precise memory layout. This is because TOSA is designed to be hardware-agnostic, and different hardware platforms may have different optimal memory layouts.

* **Implicit 4D Structure:**
    * When you see TOSA operations dealing with convolutional neural networks, the 4D structure is implicitly understood. The `conv2d` operator, by its very nature, operates on data that has these four dimensions.

* **Where to find more information:**
    * To get the most accurate and up to date information regarding the TOSA specifications, it is best to refer to the official TOSA specification documents found on the MLPlatform.org website. Here is the link to the git repository: [https://git.mlplatform.org/tosa/specification.git/](https://git.mlplatform.org/tosa/specification.git/)
    * Within those documents, the descriptions of the individual operators (like `conv2d`) will explain how the operators expect the input tensors to be structured.

In essence, while the TOSA specification might not have a single, explicit definition of a "4D TOSA-compliant tensor" as a standalone definition, the 4D structure is inherent in the way TOSA operators are designed to work with convolutional neural network data.

