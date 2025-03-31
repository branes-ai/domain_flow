// Example TOSA MLIR file
module {
  func.func @simple_function(%arg0: tensor<1x1x4xf32>, %arg1: tensor<1x4x4xf32>) -> tensor<1x1x4xf32> {
    %cst1 = "tosa.const"() {value = dense<1.0> : tensor<1x1x4xf32>} : () -> tensor<1x1x4xf32>
    %add = "tosa.add"(%arg0, %cst1) : (tensor<1x1x4xf32>, tensor<1x1x4xf32>) -> tensor<1x1x4xf32>
    %matmul = "tosa.matmul"(%add, %arg1) : (tensor<1x1x4xf32>, tensor<1x4x4xf32>) -> tensor<1x1x4xf32>
    return %matmul : tensor<1x1x4xf32>
  }
}
