// --- Module Definition ---
// A 'module' is the top-level container in MLIR. It holds functions,
// global variables (like constants defined outside functions), etc.
// It implicitly contains one region, which in turn contains one block.
module {

  // --- Function Definition ('func.func') ---
  // Defines a function named '@simple_tosa_func'.
  // - '@simple_tosa_func': The function's symbol name.
  // - '(%arg0: tensor<1x4xf32>, %arg1: tensor<1x4xf32>)': Input arguments and their types.
  // - '-> tensor<1x4xf32>': The return type(s).
  // The function body is enclosed in curly braces '{}', which represents a 'region'.
  func.func @simple_tosa_func(%arg0: tensor<1x4xf32>, %arg1: tensor<1x4xf32>) -> tensor<1x4xf32> {

    // --- Block Definition ---
    // A region contains one or more blocks. Blocks start with a label (e.g., '^bb0')
    // and can optionally have block arguments (here, they correspond to the function arguments).
    // Operations within a block execute sequentially.
    // The last operation in a block must be a 'terminator' (like func.return, cf.br).
    ^bb0(%b_arg0: tensor<1x4xf32>, %b_arg1: tensor<1x4xf32>): // Block label and block arguments

      // --- TOSA Operations within the Block ---

      // 1. Define a constant tensor using tosa.const
      // %cst is the SSA value name for the result of this operation.
      %cst = "tosa.const"() {value = dense<[1.0, 2.0, 3.0, 4.0]> : tensor<1x4xf32>} : () -> tensor<1x4xf32>

      // 2. Add the first function argument (%b_arg0) and the constant (%cst)
      // Note: Block arguments %b_arg0/%b_arg1 correspond to function arguments %arg0/%arg1.
      %add_result = "tosa.add"(%b_arg0, %cst) : (tensor<1x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>

      // 3. Multiply the result of the addition (%add_result) with the second function argument (%b_arg1)
      // TOSA multiplication requires a 'shift' attribute for quantization, set to 0 for float.
      %mul_result = "tosa.mul"(%add_result, %b_arg1) {shift = 0 : i32} : (tensor<1x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>

      // --- Terminator Operation ---
      // Return the final result (%mul_result) from the function.
      // 'func.return' is the terminator for this block.
      "func.return"(%mul_result) : (tensor<1x4xf32>) -> ()

    // End of block ^bb0
  } // End of region for @simple_tosa_func

  // --- Another simple function (optional, shows multiple functions in a module) ---
  func.func @another_func() {
    // This function does nothing but return. It still has a region and a block.
    ^entry: // Block label
      "func.return"() : () -> ()
  } // End of region for @another_func

} // End of module region
