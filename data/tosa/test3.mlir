// --- Module Definition ---
// A 'module' is the top-level container in MLIR.
module {

  // --- Function 1: Simple Add/Mul (from previous example) ---
  // Demonstrates basic structure and simple TOSA ops.
  func.func @simple_tosa_func(%arg0: tensor<1x4xf32>, %arg1: tensor<1x4xf32>) -> tensor<1x4xf32> {
      // Entry block
      // Constant tensor
      %cst = "tosa.const"() {value = dense<[[1.0, 2.0, 3.0, 4.0]]> : tensor<1x4xf32>} : () -> tensor<1x4xf32>
      // Add operation
      %add_result = "tosa.add"(%arg0, %cst) : (tensor<1x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
      // Multiply operation (float requires shift=0)
      %mul_result = "tosa.mul"(%add_result, %arg1) {shift = 0 : i8} : (tensor<1x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
      // Return result
      "func.return"(%mul_result) : (tensor<1x4xf32>) -> ()
  } // End of region for @simple_tosa_func

  // --- Function 2: Reshape Example ---
  // Demonstrates the tosa.reshape operator.
  // Takes a 1x2x4 tensor and reshapes it to 1x8.
  func.func @reshape_example(%input_tensor: tensor<1x2x4xf32>) -> tensor<1x8xf32> {
      // Reshape the input tensor. The total number of elements must remain the same (1*2*4 = 8, 1*8 = 8).
      // The new shape is specified as an attribute.
      %reshaped_output = "tosa.reshape"(%input_tensor) {new_shape = array<i64:1,8>} : (tensor<1x2x4xf32>) -> tensor<1x8xf32>
      // Return the reshaped tensor
      "func.return"(%reshaped_output) : (tensor<1x8xf32>) -> ()
  } // End of region for @reshape_example

  // --- Function 3: Conv2D Example ---
  // Demonstrates the tosa.conv2d operator.
  // Takes NHWC input, weights, and bias. Produces NHWC output.
  // Input:  1x5x5x3 (Batch=1, Height=5, Width=5, InChannels=3)
  // Weight: 8x3x3x3 (OutChannels=8, KernelH=3, KernelW=3, InChannels=3)
  // Bias:   8         (OutChannels=8)
  // Output: 1x5x5x8 (Batch=1, Height=5, Width=5, OutChannels=8) - calculated based on pad/stride/dilation
  func.func @conv2d_example(%input_map: tensor<1x5x5x3xf32>,
                           %kernel: tensor<8x3x3x3xf32>,
                           %bias: tensor<8xf32>) -> tensor<1x5x5x8xf32> {
      // Perform 2D convolution.
      // Attributes define padding, stride, and dilation.
      // Quantization attributes (input_zp, weight_zp) are often 0 for float tensors.
      %conv_output = "tosa.conv2d"(%input_map, %kernel, %bias) {
          dilation = array<i64: 1, 1>,
          pad = array<i64: 1, 1, 1, 1>, // Top, Bottom, Left, Right padding -> results in 'SAME' padding for 3x3 kernel, stride 1
          stride = array<i64: 1, 1>
      } : (tensor<1x5x5x3xf32>, tensor<8x3x3x3xf32>, tensor<8xf32>) -> tensor<1x5x5x8xf32>
      // Return the convolution result
      "func.return"(%conv_output) : (tensor<1x5x5x8xf32>) -> ()
  } // End of region for @conv2d_example


  // --- Function 4: Fully Connected Example ---
  // Demonstrates the tosa.fully_connected operator.
  // Typically used after flattening the output of convolutions.
  // Input:  1x64 (Batch=1, Features=64)
  // Weight: 10x64 (OutUnits=10, InUnits=64)
  // Bias:   10    (OutUnits=10)
  // Output: 1x10  (Batch=1, OutUnits=10)
  func.func @fully_connected_example(%flat_input: tensor<1x64xf32>,
                                     %fc_weights: tensor<10x64xf32>,
                                     %fc_bias: tensor<10xf32>) -> tensor<1x10xf32> {

      // Perform fully connected operation (essentially matrix multiplication + bias).
      // Quantization attributes are provided similarly to conv2d.
      %fc_output = "tosa.fully_connected"(%flat_input, %fc_weights, %fc_bias) {} : (tensor<1x64xf32>, tensor<10x64xf32>, tensor<10xf32>) -> tensor<1x10xf32>
      // Return the fully connected result
      "func.return"(%fc_output) : (tensor<1x10xf32>) -> ()
  } // End of region for @fully_connected_example


  // --- Function 5: Another simple function (optional placeholder) ---
  // Demonstrates that a module can contain multiple functions.
  func.func @another_func() {
    ^entry: // Block label
      "func.return"() : () -> ()
  } // End of region for @another_func

} // End of module region
