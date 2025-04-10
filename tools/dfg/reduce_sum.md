# Reduce SUM operator

The arithmetic complexity of the TOSA `reduce_sum` operation in the context of MobileNetV2:

```
%204 = tosa.reduce_sum %203 {axis = 1 : i32} : (tensor<?x7x7x1280xf32>) -> tensor<?x1x7x1280xf32>
```

**Parameters:**

* **Input Tensor (%203):** `?x7x7x1280xf32` (Unknown batch size, 7x7 spatial dimensions, 1280 channels, float32 data type)
* **Axis:** `1 : i32` (Reduce along the second axis, which is the height dimension)
* **Output Tensor (%204):** `?x1x7x1280xf32` (Unknown batch size, 1x7 spatial dimensions, 1280 channels, float32 data type)

**Understanding `reduce_sum`:**

* The `reduce_sum` operator sums the elements of the input tensor along the specified axis.
* In this case, we're summing along the height dimension (axis 1). This means that for each batch, width, and channel, we'll sum the 7 elements along the height.

**Arithmetic Operations:**

1.  **Summation:**
    * For each element in the output tensor, we need to sum 7 elements from the input tensor.
    * To sum 7 elements, we need 6 additions.
    * The output tensor has dimensions `?x1x7x1280`.
    * Therefore, for one image in the batch, the number of additions is: 1x7x1280x6.
    * Total additions: $1 \times 7 \times 1280 \times 6 = 53,760$

**Total Arithmetic Complexity (per image):**

* **Additions:** 53,760

**Batch Size Consideration:**

* If the batch size is `B`, then the total arithmetic operations will be `B` times the above calculations.

**Key Points:**

* The `reduce_sum` operator primarily involves addition operations.
* The axis parameter determines the dimension along which the summation is performed.
* The output tensor has a reduced dimension along the specified axis.
* The number of additions is determined by the size of the reduced dimension minus one, multiplied by the size of the other dimensions.
