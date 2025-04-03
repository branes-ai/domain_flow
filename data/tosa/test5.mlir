        module {
          func.func @simple_function(%arg0: tensor<1x1x4xf32>, %arg1: tensor<1x4x4xf32>) -> tensor<1x1x4xf32> {
            %0 = "tosa.const"() {value = dense<1.0> : tensor<1x1x4xf32>} : () -> tensor<1x1x4xf32>
            %add = "tosa.add"(%arg0, %0) : (tensor<1x1x4xf32>, tensor<1x1x4xf32>) -> tensor<1x1x4xf32>
            %2 = "tosa.matmul"(%add, %arg1) : (tensor<1x1x4xf32>, tensor<1x4x4xf32>) -> tensor<1x1x4xf32>
            return %2 : tensor<1x1x4xf32>
          }
          func.func @main() {
            %cst0 = "tosa.const"() {value = dense<[[[1.0, 2.0, 3.0, 4.0]]]> : tensor<1x1x4xf32>} : () -> tensor<1x1x4xf32>
            %cst1 = "tosa.const"() {value = dense<[[[1.0, 2.0, 3.0, 4.0],[5.0, 6.0, 7.0, 8.0],[9.0, 10.0, 11.0, 12.0],[13.0, 14.0, 15.0, 16.0]]]> : tensor<1x4x4xf32>} : () -> tensor<1x4x4xf32>
            %result = func.call @simple_function(%cst0, %cst1) : (tensor<1x1x4xf32>, tensor<1x4x4xf32>) -> tensor<1x1x4xf32>
            return
          }
        }
