# Matmul computation

Yes, we can express the tensor `matmul` operation with symbolic input shapes. However, we need to be careful about the dimensions to ensure the multiplication is valid.

**General Formula for Tensor Matmul**

Let's assume the following input tensors:

* `arg0`: tensor<axbxcxf32>
* `arg1`: tensor<dxexfxf32>

For the `matmul` operation to be valid, the last dimension of `arg0` (`c`) must match the second-to-last dimension of `arg1` (`e`). In other words, `c` must equal `e`.

If `c == l`, the output tensor `result` will have the shape tensor<axbxf>.

The calculation for each element of the `result` tensor can be expressed as:

```
result[i, j, f_index] = sum(arg0[i, j, k] * arg1[d_index, k, f_index]) for k = 0 to c - 1
```

where:

* $i$ ranges from $0$ to $a - 1$
* $j$ ranges from $0$ to $b - 1$
* $f\_index$ ranges from $0$ to $f - 1$
* $k$ ranges from $0$ to $c-1$, and $c == e$
* $d\_index$ ranges from $0$ to $d-1$

**Arithmetic Complexity**

* **Multiplications:** For each element in the `result` tensor (axbxf), we perform `c` multiplications. Therefore, the total number of multiplications is $a \times b \times f \times c$.
* **Additions:** For each element in the `result` tensor (axbxf), we perform `c - 1` additions. Therefore, the total number of additions is $a \times b \times f \times (c - 1)$.

**Important Considerations**

* **Dimension Compatibility:** The core requirement is that the inner dimensions must match (`c == e`). If they don't, the `matmul` operation is undefined.
* **Broadcasting:** Depending on the specific `matmul` implementation, broadcasting rules might apply to the leading dimensions (`a`, `b`, `d`). However, in the TOSA specification, broadcasting is not defined for matmul.
* **General Matrix Multiplication:** this equation is for a batched matrix multiplication, where a and b represent batch dimensions. If a = 1, and b = 1 then this equation represents a standard matrix multiplication.

**Summary**

The general formula for `tosa.matmul` with symbolic input shapes `tensor<axbxcxf32>` and `tensor<dxexfxf32>` (where `c == e`) is:

* Output shape: `tensor<axbxfxf32>`
* Calculation: $result[i, j, f\_index] = \sum_{k=0}^{c-1} arg0[i, j, k] \times arg1[d\_index, k, f\_index]$
* Multiplications: $a \times b \times f \times c$
* Additions: $a \times b \times f \times (c - 1)$

## The batch dimension of the second tensor

**Handling the `d` Dimension: the batch dimension of the second tensor**

Let's break down the `matmul` operation with `tensor<axbxc>` and `tensor<dxexf>` inputs, focusing on how the `d` dimension is processed:

1.  **Dimension Matching:** As before, `c` must equal `e`.

2.  **Output Shape:** The output shape is `tensor<axbxf>`. The `d` dimension is *not* present in the output shape because the matmul operation is performed for *each* element in the `d` dimension and the results are summed into the element.

3.  **Calculation:** The calculation is performed for *each* value of `d`. Therefore the calculation is performed `d` times.

    * For each value of `d` from 0 to `d-1`:
        * `result[i, j, f] = sum(arg0[i, j, k] * arg1[d, k, f]) for k = 0 to c - 1`
        * Where:
            * `i` ranges from 0 to `a - 1`.
            * `j` ranges from 0 to `b - 1`.
            * `f` ranges from 0 to `f - 1`.
            * `k` ranges from 0 to `c - 1`.
            * `d` ranges from 0 to `d-1`.

4.  **How `d` is Handled:**
    * The `d` dimension represents a batch dimension in the second input tensor.
    * The `matmul` operation is performed independently for each value of `d`.
    * This means that for a given `i`, `j`, and `f`, you're effectively computing `d` separate results, each using a different "slice" of the second input tensor along the `d` dimension.
    * Because the result of the calculation is not stored in a tensor that has the d dimension, the d dimension is not present in the output. The calculation is performed d times, but the result is stored in the same output tensor.
    * The calculation is performed for each batch, and the result is stored within the output tensor.

**Example: matmul(tensor<3x5x7>, tensor<2x7x9>)**

* We perform the calculation twice, once for `d = 0` and once for `d = 1`.
* The results of both calculations are stored in the same `tensor<3x5x9>` output tensor.

**Key Insight:**

* The `d` dimension collapses through a sum of the matmuls in the `d` dimension. The `matmul` operation is *repeated* for each value of `d`, and the results are accumulated within the final output tensor.

