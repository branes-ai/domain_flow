#if WIN32
#pragma warning(disable : 4244 4267)
#endif

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include <iostream>

// Define a simple function to execute operations.
void executeOperation(mlir::Operation &op) {
    std::string opName = op.getName().getStringRef().str();
    std::cout << "Executing Operation: " << opName << "\n";

    // Enumerate operands of the operation.
    std::cout << "Operands:\n";
    for (mlir::Value operand : op.getOperands()) {
        std::cout << "  Operand: ";
        if (auto definingOp = operand.getDefiningOp()) {
            std::cout << definingOp->getName().getStringRef().str() << "\n";
        }
        else {
            std::cout << "Undefined operand (block argument or constant)\n";
        }
    }

    // Example execution logic for TOSA operations.
    if (opName == "tosa.add") {
        //std::cout << "Performing addition...\n";
        // Add logic for handling 'tosa.add' here.
    }
    else if (opName == "tosa.matmul") {
        //std::cout << "Performing matrix multiplication...\n";
        // Add logic for handling 'tosa.matmul' here.
    }
    else if (opName == "tosa.const") {
        //std::cout << "Found a TOSA constant...\n";
    } 
    else if (opName == "tosa.reshape") {
        //std::cout << "Performing a Reshape...\n";
    }
    else if (opName == "tosa.conv2d") {
        //std::cout << "Performing a Conv2D...\n";
    }
    else if (opName == "func.return") {
        //std::cout << "Performing a func return...\n";
    }
    else {
        //std::cout << "Unknown operation type.\n";
    }
}

int main(int argc, char **argv) {
    // Ensure an MLIR file is provided as input.
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <MLIR file>\n";
        return 1;
    }

    // Create an MLIR context and register the TOSA dialect.
    mlir::MLIRContext context;
    mlir::DialectRegistry registry;
    registry.insert<mlir::tosa::TosaDialect>();
    registry.insert<mlir::func::FuncDialect>();
    context.appendDialectRegistry(registry);

    // Parse the provided MLIR file.
    auto module = mlir::parseSourceFile<mlir::ModuleOp>(argv[1], &context);
    if (!module) {
        std::cerr << "Failed to parse MLIR file: " << argv[1] << "\n";
        return 1;
    }

    // Walk through the operations in the module and execute them.
    for (auto func : module->getOps<mlir::func::FuncOp>()) {
        for (auto &op : func.getBody().getOps()) {
            executeOperation(op);
        }
    }

    return 0;
}
