#if WIN32
#pragma warning(disable : 4244 4267)
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
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <vector>
#include <dfa/graph/graph.hpp>

namespace sw {
    namespace dfa {

        // Helper struct to store parsed operand information
        struct OperandInfo {
            std::string name;
            mlir::Type type;
            size_t index;
        };

        // Helper struct to store parsed attribute information
        struct AttributeInfo {
            std::string name;
            std::string valueStr;
            mlir::Attribute attr;
        };

        // Helper struct for Conv2D specific parsed attributes
        struct Conv2DAttributes {
            std::vector<int64_t> pad;
            std::vector<int64_t> stride;
            std::vector<int64_t> dilation;
        };

        // Function to extract operand information from an operation (using reference)
        std::vector<OperandInfo> parseOperands(mlir::Operation& op) {
            std::vector<OperandInfo> operands;

            for (size_t i = 0; i < op.getNumOperands(); ++i) {
                mlir::Value operand = op.getOperand(i);
                OperandInfo info;
                info.index = i;
                info.type = operand.getType();

                // Try to get operand name from its defining op if available
                if (auto definingOp = operand.getDefiningOp()) {
                    info.name = definingOp->getName().getStringRef().str();
                }
                else {
                    info.name = "block_arg_" + std::to_string(i);
                }

                operands.push_back(info);
            }

            return operands;
        }

        // Function to extract attribute information from an operation (using reference)
        std::vector<AttributeInfo> parseAttributes(mlir::Operation& op) {
            std::vector<AttributeInfo> attributes;

            for (mlir::NamedAttribute namedAttr : op.getAttrs()) {
                AttributeInfo info;
                info.name = namedAttr.getName().strref().str();
                info.attr = namedAttr.getValue();

                std::string attrStr;
                llvm::raw_string_ostream os(attrStr);
                namedAttr.getValue().print(os);
                info.valueStr = os.str();

                attributes.push_back(info);
            }

            return attributes;
        }

        // Function to extract block information from an operation (using reference)
        void parseBlocks(mlir::Operation& op, llvm::raw_ostream& os) {
            os << "Operation has " << op.getNumRegions() << " regions\n";

            for (size_t i = 0; i < op.getNumRegions(); ++i) {
                mlir::Region& region = op.getRegion(i);
                os << "  Region " << i << " has " << region.getBlocks().size() << " blocks\n";

                for (mlir::Block& block : region.getBlocks()) {
                    os << "    Block with " << block.getNumArguments() << " arguments and "
                        << block.getOperations().size() << " operations\n";

                    for (mlir::Operation& nestedOp : block.getOperations()) {
                        os << "      Operation: " << nestedOp.getName().getStringRef().str() << "\n";
                    }
                }
            }
        }

        // Function to extract Conv2D specific attributes
        Conv2DAttributes parseConv2DAttributes(mlir::tosa::Conv2DOp convOp) {
            Conv2DAttributes result;

            // Extract pad attribute
            if (auto padAttr = convOp.getPadAttr()) {
                auto padValues = padAttr.asArrayRef();
                result.pad.assign(padValues.begin(), padValues.end());
            }

            // Extract stride attribute
            if (auto strideAttr = convOp.getStrideAttr()) {
                auto strideValues = strideAttr.asArrayRef();
                result.stride.assign(strideValues.begin(), strideValues.end());
            }

            // Extract dilation attribute
            if (auto dilationAttr = convOp.getDilationAttr()) {
                auto dilationValues = dilationAttr.asArrayRef();
                result.dilation.assign(dilationValues.begin(), dilationValues.end());
            }

            return result;
        }

        // A specialized function to parse TOSA Conv2D operations (using reference)
        void parseTosaConv2D(mlir::Operation& op, llvm::raw_ostream& os) {
            if (!mlir::isa<mlir::tosa::Conv2DOp>(op)) {
                os << "Error: Not a TOSA Conv2D operation\n";
                return;
            }

            auto convOp = mlir::cast<mlir::tosa::Conv2DOp>(op);

            // Parse basic operation information
            os << "TOSA Conv2D Operation:\n";

            // Parse operands
            os << "Operands:\n";
            os << "  Input: " << convOp.getInput().getType() << "\n";
            os << "  Weight: " << convOp.getWeight().getType() << "\n";
            if (convOp.getBias())
                os << "  Bias: " << convOp.getBias().getType() << "\n";

            // Parse result
            os << "Result:\n";
            os << "  " << convOp.getOutput().getType() << "\n";

            // Parse Conv2D specific attributes
            Conv2DAttributes convAttrs = parseConv2DAttributes(convOp);

            os << "Attributes:\n";
            os << "  Padding: [";
            for (size_t i = 0; i < convAttrs.pad.size(); ++i) {
                if (i > 0) os << ", ";
                os << convAttrs.pad[i];
            }
            os << "]\n";

            os << "  Stride: [";
            for (size_t i = 0; i < convAttrs.stride.size(); ++i) {
                if (i > 0) os << ", ";
                os << convAttrs.stride[i];
            }
            os << "]\n";

            os << "  Dilation: [";
            for (size_t i = 0; i < convAttrs.dilation.size(); ++i) {
                if (i > 0) os << ", ";
                os << convAttrs.dilation[i];
            }
            os << "]\n";
        }

        // Main function that demonstrates the usage of all the parsing functions (using reference)
        void parseOperation(mlir::Operation& op, llvm::raw_ostream& os) {
            os << "Parsing operation: " << op.getName().getStringRef().str() << "\n";

            // Parse operands
            std::vector<OperandInfo> operands = parseOperands(op);
            os << "Operands (" << operands.size() << "):\n";
            for (const auto& operand : operands) {
                os << "  " << operand.index << ": " << operand.name << " of type " << operand.type << "\n";
            }

            // Parse attributes
            std::vector<AttributeInfo> attributes = parseAttributes(op);
            os << "Attributes (" << attributes.size() << "):\n";
            for (const auto& attr : attributes) {
                os << "  " << attr.name << ": " << attr.valueStr << "\n";
            }

            // Parse blocks
            parseBlocks(op, os);

            // If the operation is a TOSA Conv2D, parse it specifically
            if (mlir::isa<mlir::tosa::Conv2DOp>(op)) {
                os << "\nDetected TOSA Conv2D operation, parsing specifically:\n";
                parseTosaConv2D(op, os);
            }
        }

        // Example of how to use these functions with your code pattern
        void processModule(mlir::ModuleOp module) {
            std::string output;
            llvm::raw_string_ostream os(output);

            // Walk through the operations in the module and analyze them
            for (auto func : module.getOps<mlir::func::FuncOp>()) {
                os << "Processing function: " << func.getName() << "\n";
                for (auto& op : func.getBody().getOps()) {
                    // Call our parser function instead of executeOperation
                    parseOperation(op, os);

                    // For TOSA Conv2D operations, you can also use the specialized parser
                    if (mlir::isa<mlir::tosa::Conv2DOp>(op)) {
                        os << "\nDetailed TOSA Conv2D analysis:\n";
                        parseTosaConv2D(op, os);
                    }
                }
            }

            // Print or save the output
            std::cout << output << std::endl;
        }

        // Example of how to use the above functions with a TOSA Conv2D operation
        // Note: This function wouldn't be compiled as it would need an actual IR context and module
        void exampleWithTosaConv2D() {
            /*
             This would be equivalent to the following MLIR code:

             %0 = tosa.conv2d %input, %weights, %bias {
               pad = [1, 1, 1, 1],
               stride = [1, 1],
               dilation = [1, 1]
             } : (tensor<1x32x32x3xf32>, tensor<16x3x3x3xf32>, tensor<16xf32>) -> tensor<1x32x32x16xf32>

             The usage would be:

             mlir::Operation* op = ... // Get the operation from somewhere
             std::string output;
             llvm::raw_string_ostream os(output);
             parseOperation(op, os);
             std::cout << output << std::endl;
             */
        }
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

    // Walk through the operations in the module and parse them

    std::string output;
    llvm::raw_string_ostream os(output);

    // Walk through the operations in the module and analyze them
    for (auto func : module->getOps<mlir::func::FuncOp>()) {
        os << "Processing function: " << func.getName() << "\n";
        for (auto& op : func.getBody().getOps()) {
            // Call our parser function instead of executeOperation
            sw::dfa::parseOperation(op, os);

            // For TOSA Conv2D operations, use the specialized parser to capture and interpret attributes
            if (mlir::isa<mlir::tosa::Conv2DOp>(op)) {
                os << "\nDetailed TOSA Conv2D analysis:\n";
                sw::dfa::parseTosaConv2D(op, os);
            }
        }
    }

    // Print or save the output
    std::cout << output << std::endl;

    return 0;
}
