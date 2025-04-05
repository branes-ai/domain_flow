module {
func.func @resnet_block(%input: tensor<?x?x?x64xf32>, %weights1: tensor<64x576xf32>, %bias1: tensor<64xf32>, %weights2: tensor<64x576xf32>, %bias2: tensor<64xf32>) -> tensor<?x?x?x64xf32> {
  %cst_0_i64 = "tosa.const"() {value = dense<0> : tensor<i64>} : () -> tensor<i64>
  %cst_1_i64 = "tosa.const"() {value = dense<1> : tensor<i64>} : () -> tensor<i64>
  %cst_3_i64 = "tosa.const"() {value = dense<3> : tensor<i64>} : () -> tensor<i64>
  %cst_64_i64 = "tosa.const"() {value = dense<64> : tensor<i64>} : () -> tensor<i64>

  %dim0 = tosa.dim %input, %cst_0_i64 : tensor<?x?x?x64xf32> -> tensor<i64>
  %dim1 = tosa.dim %input, %cst_1_i64 : tensor<?x?x?x64xf32> -> tensor<i64>
  %dim2 = tosa.dim %input, %cst_3_i64 : tensor<?x?x?x64xf32> -> tensor<i64>

  %empty_shape = tosa.concat %dim0, %dim1, %dim2, %cst_64_i64 {axis = 0 : i64} : tensor<i64>, tensor<i64>, tensor<i64>, tensor<i64> -> tensor<4xi64>
  %init_output = tosa.create_tensor %empty_shape, %cst_0_i64 : tensor<4xi64>, tensor<i64> -> tensor<?x?x?x64xf32>

  %batch_loop = tosa.while_loop %cst_0_i64, %init_output : tensor<i64>, tensor<?x?x?x64xf32> -> tensor<i64>, tensor<?x?x?x64xf32> {
    %batch_loop_cond = func.func @batch_loop_cond(%batch_idx: tensor<i64>, %acc: tensor<?x?x?x64xf32>) -> tensor<i1> {
      %batch_cond = tosa.less %batch_idx, %dim0 : tensor<i64>, tensor<i64> -> tensor<i1>
      tosa.return %batch_cond : tensor<i1>
    }
    %batch_loop_body = func.func @batch_loop_body(%batch_idx: tensor<i64>, %acc: tensor<?x?x?x64xf32>) -> tensor<i64>, tensor<?x?x?x64xf32> {
      %slice_shape = tosa.concat %cst_1_i64, %dim1, %dim2, %cst_64_i64 {axis = 0 : i64} : tensor<i64>, tensor<i64>, tensor<i64>, tensor<i64> -> tensor<4xi64>
      %slice_start = tosa.concat %batch_idx, %cst_0_i64, %cst_0_i64, %cst_0_i64 {axis = 0 : i64} : tensor<i64>, tensor<i64>, tensor<i64>, tensor<i64> -> tensor<4xi64>
      %batch_input = tosa.slice %input, %slice_start, %slice_shape : tensor<?x?x?x64xf32>, tensor<4xi64>, tensor<4xi64> -> tensor<1x?x?x64xf32>

      %row_loop = tosa.while_loop %cst_0_i64, %acc[%batch_idx] : tensor<i64>, tensor<?x?x64xf32> -> tensor<i64>, tensor<?x?x64xf32> {
        %row_loop_cond = func.func @row_loop_cond(%row_idx: tensor<i64>, %acc_row: tensor<?x?x64xf32>) -> tensor<i1> {
          %row_cond = tosa.less %row_idx, %dim1 : tensor<i64>, tensor<i64> -> tensor<i1>
          tosa.return %row_cond : tensor<i1>
        }
        %row_loop_body = func.func @row_loop_body(%row_idx: tensor<i64>, %acc_row: tensor<?x?x64xf32>) -> tensor<i64>, tensor<?x?x64xf32> {
          %col_loop = tosa.while_loop %cst_0_i64, %acc_row[%row_idx] : tensor<i64>, tensor<?x64xf32> -> tensor<i64>, tensor<?x64xf32> {
            %col_loop_cond = func.func @col_loop_cond(%col_idx: tensor<i64>, %acc_col: tensor<?x64xf32>) -> tensor<i1> {
              %col_cond = tosa.less %col_idx, %dim2 : tensor<i64>, tensor<i64> -> tensor<i1>
              tosa.return %col_cond : tensor<i1>
            }
            %col_loop_body = func.func @col_loop_body(%col_idx: tensor<i64>, %acc_col: tensor<?x64xf32>) -> tensor<i64>, tensor<?x64xf32> {
              %patch_shape = tosa.concat %cst_1_i64, %cst_3_i64, %cst_3_i64, %cst_64_i64 {axis = 0 : i64} : tensor<i64>, tensor<i64>, tensor<i64>, tensor<i64> -> tensor<4xi64>
              %patch_start = tosa.concat %cst_0_i64, %row_idx, %col_idx, %cst_0_i64 {axis = 0 : i64} : tensor<i64>, tensor<i64>, tensor<i64>, tensor<i64> -> tensor<4xi64>
              %input_patch = tosa.slice %batch_input, %patch_start, %patch_shape : tensor<1x?x?x64xf32>, tensor<4xi64>, tensor<4xi64> -> tensor<1x3x3x64xf32>
              %reshape_shape = tosa.concat %cst_1_i64, %cst_576_i64 {value = dense<576> : tensor<i64>} {axis = 0 : i64}: tensor<i64>, tensor<i64> -> tensor<2xi64>
              %reshaped_input = tosa.reshape %input_patch, %reshape_shape : tensor<1x3x3x64xf32>, tensor<2xi64> -> tensor<1x576xf32>
              %squeezed_input = tosa.squeeze %reshaped_input, %cst_0_i64 : tensor<1x576xf32>, tensor<i64> -> tensor<576xf32>

              %matmul_result1 = tosa.matmul %squeezed_input, %weights1 : tensor<576xf32>, tensor<64x576xf32> -> tensor<64xf32>
              %biased_result1 = tosa.add %matmul_result1, %bias1 : tensor<64xf32>, tensor<64xf32> -> tensor<64xf32>
              %relu1 = tosa.relu %biased_result1 : tensor<64xf32> -> tensor<64xf32>

              %matmul_result2 = tosa.matmul %squeezed_input, %weights2 : tensor<576xf32>, %weights2 : tensor<576xf32>, tensor<64x576xf32> -> tensor<64xf32>
              %biased_result2 = tosa.add %matmul_result2, %bias2 : tensor<64xf32>, tensor<64xf32> -> tensor<64xf32>
              %relu2 = tosa.relu %biased_result2 : tensor<64xf32> -> tensor<64xf32>

              %add_result = tosa.add %relu1, %relu2 : tensor<64xf32>, tensor<64xf32> -> tensor<64xf32>

              %updated_acc_col = tosa.insert %acc_col, %add_result, %col_idx : tensor<?x64xf32>, tensor<64xf32>, tensor<i64> -> tensor<?x64xf32>
              %next_col_idx = tosa.add %col_idx, %cst_1_i64 : tensor<i64>, tensor<i64> -> tensor<i64>
              tosa.return %next_col_idx, %updated_acc_col : tensor<i64>, tensor<?x64xf32>
            }
            tosa.return %col_loop : tensor<i64>, tensor<?x64xf32>
          }
          %updated_acc_row = tosa.insert %acc_row, %col_loop[1], %row_idx : tensor<?x?x64xf32>, tensor<?x64xf32>, tensor<i64> -> tensor<?x?x64xf32>
          %next_row_idx = tosa.add %row_idx, %cst_1_i64 : tensor<i64>, tensor<i64> -> tensor<i64>
          tosa.return %next_row_idx, %updated_acc_row : tensor<i64>, tensor<?x?x64xf32>
        }
        tosa.return %row_loop : tensor<i64>, tensor<?x?x64xf32>
      }
      %updated_acc = tosa.insert %acc, %row_loop[1], %batch_idx : tensor<?x?x?x64xf32>, tensor<?x?x64xf32>, tensor<i64> -> tensor<?x?x?x64xf32>
      %next_batch_idx = tosa.add %batch_idx, %cst_1_i64 : tensor<i64>, tensor<i64> -> tensor<i64>
      tosa.return %next_batch_idx, %updated_acc: tensor<i64>, tensor<?x?x?x64xf32>
    }
    tosa.return %batch_loop : tensor<i64>, tensor<?x?x?x64xf32>
  }
  tosa.return %batch_loop[1] : tensor<?x?x?x64xf32>
}
}