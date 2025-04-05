# ResNet Block

```mlir
func.func @resnet_block(%input: tensor<?x?x?x64xf32>, %weights1: tensor<64x576xf32>, %bias1: tensor<64xf32>, %weights2: tensor<64x576xf32>, %bias2: tensor<64xf32>) -> tensor<?x?x?x64xf32> {
  %cst_0 = arith.constant 0 : index
  %cst_1 = arith.constant 1 : index
  %cst_3 = arith.constant 3 : index

  %dim0 = tensor.dim %input, %cst_0 : tensor<?x?x?x64xf32>
  %dim1 = tensor.dim %input, %cst_1 : tensor<?x?x?x64xf32>
  %dim2 = tensor.dim %input, %cst_3 : tensor<?x?x?x64xf32>

  %init_output = tensor.empty [%dim0, %dim1, %dim2, 64] : tensor<?x?x?x64xf32>

  %result = scf.for %batch = %cst_0 to %dim0 step %cst_1 iter_args(%acc = %init_output) -> (tensor<?x?x?x64xf32>) {
    %batch_input = tensor.extract_slice %input[%batch, 0, 0, 0] [%cst_1, %dim1, %dim2, 64] [1, 1, 1, 1] : tensor<?x?x?x64xf32> to tensor<1x?x?x64xf32>
    %batch_output = scf.for %row = %cst_0 to %dim1 step %cst_1 iter_args(%acc_row = %acc[%batch]) -> (tensor<?x?x64xf32>) {
      %row_input = scf.for %col = %cst_0 to %dim2 step %cst_1 iter_args(%acc_col = %acc_row[%row]) -> (tensor<?x64xf32>) {
        %input_patch1 = tensor.extract_slice %batch_input[0, %row, %col, 0] [1, 3, 3, 64] [1, 1, 1, 1] : tensor<1x?x?x64xf32> to tensor<1x3x3x64xf32>
        %reshaped_input1 = tosa.reshape %input_patch1 : tensor<1x3x3x64xf32> to tensor<1x576xf32>
        %squeezed_input1 = tosa.squeeze %reshaped_input1 {axis = array<i64: 0>} : tensor<1x576xf32> to tensor<576xf32>

        %matmul_result1 = tosa.matmul %squeezed_input1, %weights1 : tensor<576xf32>, tensor<64x576xf32> -> tensor<64xf32>
        %biased_result1 = tosa.add %matmul_result1, %bias1 : tensor<64xf32>, tensor<64xf32> -> tensor<64xf32>
        %relu1 = tosa.relu %biased_result1 : tensor<64xf32> -> tensor<64xf32>

        %input_patch2 = tensor.extract_slice %batch_input[0, %row, %col, 0] [1, 3, 3, 64] [1, 1, 1, 1] : tensor<1x?x?x64xf32> to tensor<1x3x3x64xf32>
        %reshaped_input2 = tosa.reshape %input_patch2 : tensor<1x3x3x64xf32> to tensor<1x576xf32>
        %squeezed_input2 = tosa.squeeze %reshaped_input2 {axis = array<i64: 0>} : tensor<1x576xf32> to tensor<576xf32>

        %matmul_result2 = tosa.matmul %squeezed_input2, %weights2 : tensor<576xf32>, tensor<64x576xf32> -> tensor<64xf32>
        %biased_result2 = tosa.add %matmul_result2, %bias2 : tensor<64xf32>, tensor<64xf32> -> tensor<64xf32>
        %relu2 = tosa.relu %biased_result2 : tensor<64xf32> -> tensor<64xf32>

        %add_result = tosa.add %relu1, %relu2 : tensor<64xf32>, tensor<64xf32> -> tensor<64xf32>

        %updated_acc_col = tensor.insert_slice %add_result into %acc_col[%col] [1] : tensor<64xf32> into tensor<?x64xf32>
        scf.yield %updated_acc_col : tensor<?x64xf32>
      }
      %updated_acc_row = tensor.insert_slice %row_input into %acc_row[%row] [%dim2] : tensor<?x64xf32> into tensor<?x?x64xf32>
      scf.yield %updated_acc_row : tensor<?x?x64xf32>
    }
    %updated_acc = tensor.insert_slice %batch_output into %acc[%batch] [%dim1, %dim2] : tensor<?x?x64xf32> into tensor<?x?x?x64xf32>
    scf.yield %updated_acc : tensor<?x?x?x64xf32>
  }

  return %result : tensor<?x?x?x64xf32>
}
```

**Key Improvements and Explanation:**

1.  **ResNet Block Structure:**
    * This MLIR code now models a simplified ResNet block, with two sets of matrix multiplications (approximating convolutions), ReLU activation, and an element-wise addition (the skip connection).
2.  **Two MatMul Layers:**
    * The code includes two `tosa.matmul` operations within the nested loops, mimicking two convolutional layers.
    * Each `tosa.matmul` is followed by a `tosa.add` for bias addition and a `tosa.relu` for activation.
3.  **Skip Connection:**
    * The results of the two "convolutional" layers (matmul operations) are added together using `tosa.add`, simulating the skip connection in a ResNet block.
4.  **Weights and Biases as Inputs:**
    * The function now takes `weights1`, `bias1`, `weights2`, and `bias2` as inputs, representing the weights and biases of the two "convolutional" layers.
5.  **Patch Extraction and Reshape:**
    * The same patch extraction and reshaping logic from the previous example is used to prepare the input for the `tosa.matmul` operations.
    * The reshape is to 576, because 3\*3\*64 = 576.
6.  **Looping and Output Construction:**
    * The nested `scf.for` loops and `tensor.insert_slice` operations are used to process each patch and construct the output tensor.
7.  **Variable Input Sizes:**
    * The code handles variable input sizes for the height and width dimensions of the input tensor.

**Important Notes:**

* This is still a simplified ResNet block. Real ResNet blocks may have more layers, batch normalization, and other operations.
* This code only models the core computations of a ResNet block using `tosa.matmul`.
* This code approximates a 3x3 convolution. For other kernel sizes the reshaped size would need to change.
* This implementation is far less efficient than using the optimized `tosa.conv2d` operator.
