#if WIN32
#pragma warning(disable : 4244 4267 4996)
#endif
#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h" // Include the header for ModuleOp
#include "mlir/IR/SymbolTable.h" // Include SymbolTable header
#include "mlir/Support/IndentedOstream.h" // For printing types
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/SourceMgr.h"
#include <iostream>
#include <string>

// Helper function to print Type as string
std::string getTypeString(mlir::Type type) {
    std::string typeStr;
    llvm::raw_string_ostream os(typeStr);
    type.print(os);
    return os.str();
}

int main() {
    mlir::MLIRContext context;
    context.loadDialect<mlir::tosa::TosaDialect>();
    context.loadDialect<mlir::func::FuncDialect>();

    // Example MLIR string
    std::string mlirString = R"(
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
      )";

    // Parse the MLIR string
    llvm::SourceMgr sourceMgr;
    mlir::OwningOpRef<mlir::ModuleOp> module =
        mlir::parseSourceString<mlir::ModuleOp>(mlirString, &context);

    if (!module) {
        std::cerr << "Failed to parse MLIR string." << std::endl;
        return 1;
    }

    // Get the function
    mlir::func::FuncOp func = module->lookupSymbol<mlir::func::FuncOp>("simple_function");
    if (!func) {
        std::cerr << "Function 'simple_function' not found." << std::endl;
        return 1;
    }

    // Find the call operation
    module->walk([&](mlir::func::CallOp callOp) {
        if (callOp.getCallee() == "simple_function") {
            // Get the operands of the call
            for (mlir::Value operand : callOp.getOperands()) {
                std::string operandName;
                llvm::raw_string_ostream rso(operandName);
				operand.print(rso);
                //mlir::OpPrintingFlags flags;
                //operand.printAsOperand(rso, flags);
                std::cout << "Function call argument: " << operandName << std::endl;
            }
        }
    });

    // Iterate over block arguments
    // produces: 
    // Block argument : <block argument> of type 'tensor<1x1x4xf32>' at index : 0
    // Block argument : <block argument> of type 'tensor<1x4x4xf32>' at index : 1
    for (auto arg : func.getArguments()) {
        std::string argName;
        llvm::raw_string_ostream rso(argName);
        arg.print(rso);
        std::cout << "Block argument: " << argName << std::endl;
    }
    // Iterate over block arguments
    for (auto arg : func.getArguments()) {
        // Get the arg number
        unsigned argNum = arg.getArgNumber();
        std::string symbolicName = "%arg" + std::to_string(argNum);        // Construct the symbolic name
        std::string typeStr = getTypeString(arg.getType());        // Get the type
        std::cout << "Synthesized block argument: " << symbolicName << " of type '" << typeStr << "' at index: " << argNum << std::endl;
    }

    // Iterate over the operations in the function
    func.walk([&](mlir::Operation* op) {
        if (auto addOp = llvm::dyn_cast<mlir::tosa::AddOp>(op)) {
            // Get the result of the tosa.add operation
            mlir::Value result = addOp.getResult();

            // Get the symbolic name of the result
            std::string resultName;
            llvm::raw_string_ostream rso(resultName);
            //result.print(rso);
            mlir::OpPrintingFlags flags;
            result.printAsOperand(rso, flags);

            std::cout << "tosa.add result: " << resultName << std::endl;
        }
        else if (auto matmulOp = llvm::dyn_cast<mlir::tosa::MatMulOp>(op)) {
            // Iterate through the operands of tosa.matmul operation
            for (mlir::Value operand : matmulOp.getOperands()) {
                std::string operandName;
                // Check if the operand is a block argument
                if (auto blockArg = operand.dyn_cast<mlir::BlockArgument>()) {
                    unsigned argNum = blockArg.getArgNumber();
                    operandName = "%arg" + std::to_string(argNum);
                }
                else {
                    // For non-block arguments, use the original print method
                    llvm::raw_string_ostream rso(operandName);
                    mlir::OpPrintingFlags flags;
                    //flags.printGenericOpForm(false);
					flags.useLocalScope();
                    operand.printAsOperand(rso, flags);
                }
                std::cout << "tosa.matmul operand: " << operandName << std::endl;
            }
        }
        });

    return 0;
}