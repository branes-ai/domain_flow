#if WIN32
#pragma warning(disable : 4244 4267 4996)
#endif

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include <iostream>

#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/Attributes.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <vector>
#include <iostream>

void executeBlockArguments(mlir::Block& block) {
    std::cout << "Block Arguments:\n";
    for (mlir::BlockArgument arg : block.getArguments()) {
        std::cout << "  Block Argument: ";
        std::string typeStr;
        llvm::raw_string_ostream rsos(typeStr);
        arg.getType().print(rsos);
        rsos.flush();
        std::cout << "Type = " << typeStr << "\n";
    }
}

void debugAttribute(mlir::Attribute attr) {
    std::string typeStr;
    llvm::raw_string_ostream rso(typeStr);
    attr.print(rso); // Print the full attribute
    rso.flush();
    std::cout << "Raw Attribute: " << typeStr << '\n';
}


// Define a simple function to execute operations.
void executeOperation(mlir::Operation &op) {
    std::string opName = op.getName().getStringRef().str();
    std::cout << "Executing Operation: " << opName << "\n";

    // General attribute processing for any operation with attributes.
    if (!op.getAttrs().empty()) {
        std::cout << "Attributes:\n";
        for (auto attr : op.getAttrs()) {
            std::cout << "  Attribute: " << attr.getName().str() << " = ";

            // Debug the raw attribute value to understand its type and structure
            debugAttribute(attr.getValue());

             // Dynamically handle various attribute types using mlir::dyn_cast<U>().
            if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(attr.getValue())) {
                std::cout << intAttr.getInt() << "\n";
            }
            else if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(attr.getValue())) {
                std::cout << floatAttr.getValueAsDouble() << "\n";
            }
            else if (auto strAttr = mlir::dyn_cast<mlir::StringAttr>(attr.getValue())) {
                std::cout << strAttr.getValue().str() << "\n";
            }
            else if (auto arrayAttr = mlir::dyn_cast<mlir::ArrayAttr>(attr.getValue())) {
                // Handle array attributes
                std::cout << "[";
                for (auto elem : arrayAttr.getValue()) {
                    if (auto intElem = mlir::dyn_cast<mlir::IntegerAttr>(elem)) {
                        std::cout << intElem.getInt() << " ";
                    }
                    else if (auto floatElem = mlir::dyn_cast<mlir::FloatAttr>(elem)) {
                        std::cout << floatElem.getValueAsDouble() << " ";
                    }
                    else {
                        std::cout << "<unknown> ";
                    }
                }
                std::cout << "]\n";
            }
            else if (auto denseAttr = mlir::dyn_cast<mlir::DenseElementsAttr>(attr.getValue())) {
                // Handle dense elements attributes (e.g., used for `value` in `tosa.const`)
                std::cout << "DenseElementsAttr {";
                for (auto elem : denseAttr.getValues<mlir::APFloat>()) {
                    std::cout << elem.convertToDouble() << " ";
                }
                std::cout << "}\n";
            }
            else if (auto denseIntAttr = mlir::dyn_cast<mlir::DenseIntElementsAttr>(attr.getValue())) {
                // Handle dense integer elements attributes
                std::cout << "DenseIntElementsAttr {";
                for (auto elem : denseIntAttr.getValues<mlir::APInt>()) {
                    std::cout << elem.getSExtValue() << " ";
                }
                std::cout << "}\n";
            }
            else if (auto dictAttr = mlir::dyn_cast<mlir::DictionaryAttr>(attr.getValue())) {
                // Get a specific named attribute
                mlir::Attribute dilationsAttr = dictAttr.get("dilations");
                // Then process this attribute as shown above
            } {
                std::cout << "<unknown attribute type>\n";
            }
        }
    }

    // Enumerate operands of the operation.
    if (!op.getOperands().empty()) {
        std::cout << "Operands:\n";
        for (mlir::Value operand : op.getOperands()) {
            std::cout << "  Operand: ";

            if (auto definingOp = operand.getDefiningOp()) {
                // Operand is defined by another operation.
                std::cout << definingOp->getName().getStringRef().str();

                // Print the type of the operand.
                std::string typeStr;
                llvm::raw_string_ostream rso(typeStr);
                operand.getType().print(rso); // Print the type of the operand
                rso.flush();
                std::cout << " (Type = " << typeStr << ")";
            }
            else if (auto blockArg = mlir::dyn_cast<mlir::BlockArgument>(operand)) {
                // Operand is a BlockArgument; print its type.
                std::string typeStr;
                llvm::raw_string_ostream rso(typeStr);
                blockArg.getType().print(rso);
                rso.flush();
                std::cout << "Block Argument with Type = " << typeStr;
            }
            else {
                // Catch-all for undefined values (this case shouldn't occur often).
                std::cout << "Undefined operand or constant";
            }

            std::cout << "\n";
        }
    }

    // Example: Enumerate block arguments for operations inside blocks.
    for (mlir::Region& region : op.getRegions()) {
        for (mlir::Block& block : region.getBlocks()) {
            executeBlockArguments(block);
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
        // Get the result type of the constant operation
        if (op.getNumResults() > 0) {
            mlir::Value result = op.getResult(0); // Constants usually have a single result
            std::string typeStr;
            llvm::raw_string_ostream rso(typeStr);
            result.getType().print(rso); // Print the result type
            rso.flush();
            std::cout << "Constant Result Type = " << typeStr << "\n";
        }
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
