# Depthwise Conv2D operator

The arithmetic complexity of the TOSA `depthwise_conv2d` operation:

```
%109 = tosa.depthwise_conv2d %108, %16, %104 {dilation = array<i64: 1, 1>, pad = array<i64: 1, 1, 1, 1>, stride = array<i64: 1, 1>} : (tensor<?x112x112x32xf32>, tensor<3x3x32x1xf32>, tensor<32xf32>) -> tensor<?x112x112x32xf32>
```

**Operands:**

* **Input Tensor (%108):** `?x112x112x32xf32` (Unknown batch size, 112x112 spatial dimensions, 32 input channels, float32 data type)
* **Weight Tensor (%16):** `3x3x32x1xf32` (3x3 kernel size, 32 input channels, 1 output channel multiplier, float32 data type)
* **Bias Tensor (%104):** `32xf32` (32 biases, one for each output channel, float32 data type)

**Attributes:**

* **Dilation:** `array<i64: 1, 1>` (No dilation)
* **Padding:** `array<i64: 1, 1, 1, 1>` (Padding of 1 on all sides)
* **Stride:** `array<i64: 1, 1>` (Stride of 1 in both spatial dimensions)

**Results:**

* **Output Tensor (%109):** `?x112x112x32xf32` (Unknown batch size, 112x112 spatial dimensions, 32 output channels, float32 data type)

**Key Differences from Regular `conv2d`:**

* **Depthwise Convolution:** Unlike a regular `conv2d`, which combines input channels, a `depthwise_conv2d` applies a separate convolution to each input channel.
* **Weight Tensor:** the weight tensor for a depthwise convolution has the format of `kernel_height` x `kernel_width` x `input_channels` x `channel_multiplier`. In this case the channel multiplier is 1.

**Arithmetic Operations:**

1.  **Depthwise Convolution:**
    * For each input channel, we perform a 2D convolution with a kernel of 3x3.
    * For each output pixel, we perform 3x3 = 9 multiplications and 8 additions.
    * Since there are 32 input channels, we multiply this by 32.
    * Since the stride is 1, and the padding is 1, the output spatial dimensions are the same as the input spatial dimensions, which is 112x112.
    * Therefore, for one image in the batch, the number of MAC operations is: 112x112x32x3x3.
    * Total Multiplications: $112 \times 112 \times 32 \times 9 = 3,612,672$
    * Total Additions during convolution: $112 \times 112 \times 32 \times 8 = 3,211,264$
2.  **Bias Addition:**
    * After the convolution, we add the bias to each output pixel.
    * There are 32 biases, and the output spatial dimensions are 112x112.
    * Therefore, for one image in the batch, the number of bias additions is: 112x112x32.
    * Total additions for bias: $112 \times 112 \times 32 = 401,408$

**Total Arithmetic Complexity (per image):**

* **Multiplications:** $112 \times 112 \times 32 \times 9 = 3,612,672$
* **Additions:** $(112 \times 112 \times 32 \times 8) + (112 \times 112 \times 32) = 3,211,264 + 401,408 = 3,612,672$
* Total MACs: 3,612,672
* Total Additions: 3,612,672

**Batch Size Consideration:**

* If the batch size is `B`, then the total arithmetic operations will be `B` times the above calculations.

**Key Differences Summarized:**

* **Computational Cost:** A `depthwise_conv2d` is generally less computationally expensive than a regular `conv2d` because it doesn't perform cross-channel combinations.
* **Weight Tensor Shape:** The weight tensor shape is different, reflecting the per-channel operation.
* **Functionality:** A depthwise convolution focuses on spatial feature extraction within each channel, while a regular convolution combines spatial and channel-wise features.
