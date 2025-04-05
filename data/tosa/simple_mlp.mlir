module {
  func.func @simple_mlp(
      %input: tensor<1x1x10xf32>,
      %weights1: tensor<1x10x20xf32>,
      %bias1: tensor<1x1x20xf32>,
      %weights2: tensor<1x20x30xf32>,
      %bias2: tensor<1x1x30xf32>,
      %weights3: tensor<1x30x5xf32>,
      %bias3: tensor<1x1x5xf32>
  ) -> tensor<1x1x5xf32> {

    // Layer 1: Matmul -> Add -> ReLU
    %matmul_result1 = "tosa.matmul"(%input, %weights1) : (tensor<1x1x10xf32>, tensor<1x10x20xf32>) -> tensor<1x1x20xf32>
    %biased_result1 = "tosa.add"(%matmul_result1, %bias1) : (tensor<1x1x20xf32>, tensor<1x1x20xf32>) -> tensor<1x1x20xf32>
    // ReLU replacement
    %constant_zero1 = "tosa.const"() {value = dense<0.0> : tensor<1x1x20xf32>} : () -> tensor<1x1x20xf32>
    %relu1 = "tosa.maximum"(%biased_result1, %constant_zero1) : (tensor<1x1x20xf32>, tensor<1x1x20xf32>) -> tensor<1x1x20xf32>

    // Layer 2: Matmul -> Add -> ReLU
    %matmul_result2 = "tosa.matmul"(%relu1, %weights2) : (tensor<1x1x20xf32>, tensor<1x20x30xf32>) -> tensor<1x1x30xf32>
    %biased_result2 = "tosa.add"(%matmul_result2, %bias2) : (tensor<1x1x30xf32>, tensor<1x1x30xf32>) -> tensor<1x1x30xf32>
    // ReLU replacement
    %constant_zero2 = "tosa.const"() {value = dense<0.0> : tensor<1x1x30xf32>} : () -> tensor<1x1x30xf32>
    %relu2 = "tosa.maximum"(%biased_result2, %constant_zero2) : (tensor<1x1x30xf32>, tensor<1x1x30xf32>) -> tensor<1x1x30xf32>

    // Layer 3: Matmul -> Add
    %matmul_result3 = "tosa.matmul"(%relu2, %weights3) : (tensor<1x1x30xf32>, tensor<1x30x5xf32>) -> tensor<1x1x5xf32>
    %biased_result3 = "tosa.add"(%matmul_result3, %bias3) : (tensor<1x1x5xf32>, tensor<1x1x5xf32>) -> tensor<1x1x5xf32>

    // Return the final result
    "func.return"(%biased_result3) : (tensor<1x1x5xf32>) -> ()
  }
}
