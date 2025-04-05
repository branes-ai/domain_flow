# 2.3.7. MATMUL TOSA Matmul spec

Performs two dimensional matrix multiplications.

## Precision Requirements

- Integer results must be exact.

For floating-point values, the following rules apply:

  - Subnormal `bf16_t`, `fp16_t`, and `fp32_t` input values may be flushed to zero before calculation.
  - Each output can be expressed as a dot product of two input vectors.
  - The dot product must meet the Dot product accuracy requirements.

## Arguments:

| Argument | Type    | Name | Shape   | Rank | Description |
|----------|---------|------|---------|------|-------------|
| Input    | T<in_t> |  A   | [N,H,C] | 3    | Input tensor A, N matrices of size HxC |
| Input    | T<in_t> |  B   | [N,C,W] | 3    | Input tensor B, N matrices of size CxW |
| Input    | T<in_t> | A_zp | [1]     | 1    | Input tensor A zero point. Must be zero for non-int8 types |
| Input    | T<in_t> | B_zp | [1]     | 1    | Input tensor B zero point. Must be zero for non-int8 types |
| Output   | T<out_t> | output | [N,H,W] | 3 | Output tensor, N matrices of size HxW |

## Compile Time Constant Status:

### Argument Information
| Argument | CTC Enabled Profile(s) | CTC Disabled Extension(s) |
|----------|-------------------------|---------------------------|
| A_zp     | PRO-INT, PRO-FP         | EXT-DYNAMIC              |
| B_zp     | PRO-INT, PRO-FP         | EXT-DYNAMIC              |

### Supported Data Types
| Profile/Extension | Mode                        | in_t         | out_t        |
|--------------------|-----------------------------|--------------|--------------|
| PRO-FP            | fp16 with fp16 accumulate  | fp16_t       | fp16_t       |
| PRO-FP            | fp16 with fp32 accumulate  | fp16_t       | fp32_t       |
| PRO-FP            | fp32 with fp32 accumulate  | fp32_t       | fp32_t       |
| PRO-INT           | signed 8x8 with int32 accumulate | i8_t    | i32_t        |
| EXT-BF16          | bf16 with fp32 accumulate  | bf16_t       | fp32_t       |
| EXT-FP8E4M3       | fp8e4m3 with fp16 accumulate | fp8e4m3_t | fp16_t        |
| EXT-FP8E5M2       | fp8e5m2 with fp16 accumulate | fp8e5m2_t | fp16_t        |
| EXT-INT16         | signed 16x16 with int48 accumulate | i16_t | i48_t        |


## Operation Function:

```code
ERROR_IF(is_same<in_t,i8_t> && (A_zp != 0 || B_zp != 0)); // Zero point only for i8_t
for_each(0 <= n < N, 0 <= h < H, 0 <= w < W) {
    out_t acc = 0;
    for_each(0 <= c < C) {
        out_t value1 = static_cast<out_t>(tensor_read<in_t>(A, [N,H,C], [n,h,c]));
        out_t value2 = static_cast<out_t>(tensor_read<in_t>(B, [N,C,W], [n,c,w]));
        value1 = apply_sub_s<out_t>(value1, static_cast<out_t>(A_zp));
        value2 = apply_sub_s<out_t>(value2, static_cast<out_t>(B_zp));
        acc = apply_add_s<out_t>(acc, apply_mul_s<out_t>(value1 * value2));
    }
    tensor_write<out_t>(output, [N,H,W], [n,h,w], acc);
}
```