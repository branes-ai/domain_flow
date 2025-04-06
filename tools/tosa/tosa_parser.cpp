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
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <vector>
#include <graph/graph.hpp>

namespace sw {
    namespace dfa {

        // DL Graph node type
        struct TosaOperator {
            std::string name;
            unsigned depth;   // 0 is a source

            // Constructor to initialize the node with just a string of the operator
            TosaOperator(std::string name) : name{ name }, depth{ 0 } {}
        };
        std::ostream& operator<<(std::ostream& ostr, const TosaOperator& op) {
            return ostr << op.name << " at depth " << op.depth;
        }

        // DL Graph edge type
        struct DataFlow : public graph::weighted_edge<int> { // Weighted by the data flow on this link
            int flow;
            bool stationair;  // does the flow go through a memory or not

            int weight() const noexcept override {
                return flow;
            }
            DataFlow(int flow, bool stationair) : flow{ flow }, stationair{ stationair } {}
            ~DataFlow() {}
        };

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

        // Helper struct for Clamp specific parsed attributes
        struct ClampAttributes {
            uint64_t minInt, maxInt;
            float minFp, maxFp;
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


        // Function to extract Const specific attributes
        void parseConst(mlir::Operation& op, llvm::raw_ostream& os) {
	        auto constOp = mlir::cast<mlir::tosa::ConstOp>(op);
	        // Parse basic operation information
	        os << "TOSA Const Operation:\n";
	        // Parse operands
	        // constOp does not have any operands
	        // Parse attributes
            std::vector<AttributeInfo> attributes = parseAttributes(op);
            os << "Attributes (" << attributes.size() << "):\n";
            //for (const auto& attr : attributes) {
            //    os << "  " << attr.name << "\n";
            //    //os << "  " << attr.name << ": " << attr.valueStr << "\n";
            //}

            // Parse result
            os << "Result:\n";
            os << "  " << constOp.getOutput().getType() << "\n";
		}

        // Function to extract Clamp specific attributes
        ClampAttributes parseClampAttributes(mlir::tosa::ClampOp clampOp) {
            ClampAttributes result;

            // Extract min_int attribute
            if (auto minIntAttr = clampOp.getMinIntAttr()) {
                result.minInt = minIntAttr.getValue().getSExtValue();
            }

            // Extract max_int attribute
            if (auto maxIntAttr = clampOp.getMaxIntAttr()) {
                result.maxInt = maxIntAttr.getValue().getSExtValue();
            }

            // Extract min_fp attribute
            if (auto minFpAttr = clampOp.getMinFpAttr()) {
                result.minFp = minFpAttr.getValue().convertToDouble();
            }

            // Extract max_fp attribute
            if (auto maxFpAttr = clampOp.getMaxFpAttr()) {
                result.maxFp = maxFpAttr.getValue().convertToDouble();
            }

            return result;
        }

        // A specialized function to parse TOSA Clamp operation
        void parseTosaClamp(mlir::Operation& op, llvm::raw_ostream& os) {

            auto clampOp = mlir::cast<mlir::tosa::ClampOp>(op);

            // Parse basic operation information
            os << "TOSA Clamp Operation:\n";

            // Parse operands
            os << "Operands:\n";
            os << "  Input: " << clampOp.getInput().getType() << "\n";
            
            // Parse Clamp specific attributes
            ClampAttributes clampAttrs = parseClampAttributes(clampOp);

            os << "Attributes:\n";
            os << "  Min Int: " << clampAttrs.minInt << "\n";
            os << "  Max Int: " << clampAttrs.maxInt << "\n";
            os << "  Min FP: " << clampAttrs.minFp << "\n";
            os << "  Max FP: " << clampAttrs.maxFp << "\n";

            // Parse result
            os << "Result:\n";
            os << "  " << clampOp.getOutput().getType() << "\n";
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

        // A specialized function to parse TOSA Conv2D operations
        void parseTosaConv2D(mlir::Operation& op, llvm::raw_ostream& os) {

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

		// A specialized function to parse TOSA Reshape operations
		void parseTosaReshape(mlir::Operation& op, llvm::raw_ostream& os) {
        }
		// A specialized function to parse TOSA Transpose operations
		void parseTosaTranspose(mlir::Operation& op, llvm::raw_ostream& os) {
		}
        // A specialized function to parse TOSA DepthwiseConv2D operations
        void parseTosaDepthwiseConv2D(mlir::Operation& op, llvm::raw_ostream& os) {
        }
		// A specialized function to parse TOSA TransposeConv2D operations
		void parseTosaTransposeConv2D(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA FullyConnected operations
		void parseTosaFullyConnected(mlir::Operation& op, llvm::raw_ostream& os) {
		}
        // A specialized function to parse TOSA Add operations
        void parseTosaAdd(mlir::Operation& op, llvm::raw_ostream& os) {
        }
        // A specialized function to parse TOSA Sub operations
        void parseTosaSub(mlir::Operation& op, llvm::raw_ostream& os) {
        }
		// A specialized function to parse TOSA Mul operations
		void parseTosaMul(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA Negate operations
		void parseTosaNegate(mlir::Operation& op, llvm::raw_ostream& os) {
		}

		// A specialized function to parse TOSA Pad operations
		void parseTosaPad(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA Cast operations
		void parseTosaCast(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA Gather operations
		void parseTosaGather(mlir::Operation& op, llvm::raw_ostream& os) {
		}

        // function ops
		// A specialized function to parse TOSA Reciprocal operations
		void parseTosaReciprocal(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA ReduceAll operations
		void parseTosaReduceAll(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA ReduceMax operations
		void parseTosaReduceMax(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA ReduceMin operations
		void parseTosaReduceMin(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA ReduceSum operations
		void parseTosaReduceSum(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA ReduceProd operations
		void parseTosaReduceProd(mlir::Operation& op, llvm::raw_ostream& os) {
		}

		// A specialized function to parse TOSA Exp operations
		void parseTosaExp(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA Abs operations
		void parseTosaAbs(mlir::Operation& op, llvm::raw_ostream& os) {
		}
		// A specialized function to parse TOSA Concat operations
		void parseTosaConcat(mlir::Operation& op, llvm::raw_ostream& os) {
		}




        // Parse the TOSA Op and add to the graph
        void parseOperation(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {
            if (mlir::isa<mlir::tosa::ConstOp>(op)) {
                os << "\nDetected TOSA ConstOp:\n";
                parseConst(op, os);
            }
            else if (mlir::isa<mlir::tosa::Conv2DOp>(op)) {
                os << "\nDetected TOSA Conv2DOp:\n";
                parseTosaConv2D(op, os);
            }
            else if (mlir::isa<mlir::tosa::ClampOp>(op)) {
                os << "\nDetected TOSA ClampOp:\n";
                parseTosaClamp(op, os);
            }
			else if (mlir::isa<mlir::tosa::ReshapeOp>(op)) {
				os << "\nDetected TOSA ReshapeOp:\n";
				parseTosaReshape(op, os);
			}
			else if (mlir::isa<mlir::tosa::TransposeOp>(op)) {
				os << "\nDetected TOSA TransposeOp:\n";
				parseTosaTranspose(op, os);
			}
            else if (mlir::isa<mlir::tosa::DepthwiseConv2DOp>(op)) {
                os << "\nDetected TOSA DepthwiseConv2DOp:\n";
                parseTosaDepthwiseConv2D(op, os);
            }
            else if (mlir::isa<mlir::tosa::TransposeConv2DOp>(op)) {
                os << "\nDetected TOSA TransposeConv2DOp:\n";
                parseTosaTransposeConv2D(op, os);
            }
            else if (mlir::isa<mlir::tosa::PadOp>(op)) {
                os << "\nDetected TOSA PadOp:\n";
                parseTosaPad(op, os);
            }
			else if (mlir::isa<mlir::tosa::FullyConnectedOp>(op)) {
				os << "\nDetected TOSA FullyConnectedOp:\n";
				parseTosaFullyConnected(op, os);
			}
			else if (mlir::isa<mlir::tosa::AddOp>(op)) {
				os << "\nDetected TOSA AddOp:\n";
				parseTosaAdd(op, os);
			}
			else if (mlir::isa<mlir::tosa::SubOp>(op)) {
				os << "\nDetected TOSA SubOp:\n";
				parseTosaSub(op, os);
			}
            else if (mlir::isa<mlir::tosa::MulOp>(op)) {
                os << "\nDetected TOSA MulOp:\n";
                parseTosaMul(op, os);
            }
			else if (mlir::isa<mlir::tosa::NegateOp>(op)) {
				os << "\nDetected TOSA NegateOp:\n";
				parseTosaNegate(op, os);
			}
            else if (mlir::isa<mlir::tosa::ExpOp>(op)) {
                os << "\nDetected TOSA ExpOp:\n";
                parseTosaExp(op, os);
			}
            else if (mlir::isa<mlir::tosa::AbsOp>(op)) {
                os << "\nDetected TOSA AbsOp:\n";
                parseTosaAbs(op, os);
			}
			else if (mlir::isa<mlir::tosa::ConcatOp>(op)) {
				os << "\nDetected TOSA ConcatOp:\n";
				parseTosaConcat(op, os);
			}
			else if (mlir::isa<mlir::tosa::CastOp>(op)) {
				os << "\nDetected TOSA CastOp:\n";
				parseTosaCast(op, os);
			}
			else if (mlir::isa<mlir::tosa::GatherOp>(op)) {
				os << "\nDetected TOSA GatherOp:\n";
				parseTosaGather(op, os);
			}
			else if (mlir::isa<mlir::tosa::ReciprocalOp>(op)) {
				os << "\nDetected TOSA ReciprocalOp:\n";
				parseTosaReciprocal(op, os);
			}
			else if (mlir::isa<mlir::tosa::ReduceAllOp>(op)) {
				os << "\nDetected TOSA ReduceAllOp:\n";
				parseTosaReduceAll(op, os);
			}
			else if (mlir::isa<mlir::tosa::ReduceMaxOp>(op)) {
				os << "\nDetected TOSA ReduceMaxOp:\n";
				parseTosaReduceMax(op, os);
			}
			else if (mlir::isa<mlir::tosa::ReduceMinOp>(op)) {
				os << "\nDetected TOSA ReduceMinOp:\n";
				parseTosaReduceMin(op, os);
			}
			else if (mlir::isa<mlir::tosa::ReduceSumOp>(op)) {
				os << "\nDetected TOSA ReduceSumOp:\n";
				parseTosaReduceSum(op, os);
			}
			else if (mlir::isa<mlir::tosa::ReduceProdOp>(op)) {
				os << "\nDetected TOSA ReduceProdOp:\n";
				parseTosaReduceProd(op, os);
			}

            else {
				os << "\nDetected generic TOSA operation:\n";
                std::string operatorName = op.getName().getStringRef().str();
                os << "Parsing operation: " << operatorName << "\n";
                gr.add_node(operatorName);

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
                    os << "  " << attr.name << "\n";
                    //os << "  " << attr.name << ": " << attr.valueStr << "\n";
                }
            }

            // Parse blocks
            parseBlocks(op, os);

        }

        // Example of how to use these functions with your code pattern
        void processModule(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::ModuleOp module) {
            std::string output;
            llvm::raw_string_ostream os(output);

            // Walk through the operations in the module and analyze them
            for (auto func : module.getOps<mlir::func::FuncOp>()) {
                os << "Processing function: " << func.getName() << "\n";
                for (auto& op : func.getBody().getOps()) {
                    // Call our parser function instead of executeOperation
                    parseOperation(gr, op, os);

                    // For TOSA Clamp operations, you can also use the specialized parser
                    if (mlir::isa<mlir::tosa::ClampOp>(op)) {
                        os << "\nDetailed TOSA Conv2D analysis:\n";
                        parseTosaClamp(op, os);
                    }

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
    sw::graph::directed_graph<sw::dfa::TosaOperator, sw::dfa::DataFlow> gr; // Deep Learning graph
    for (auto func : module->getOps<mlir::func::FuncOp>()) {
        os << "Processing function: " << func.getName() << "\n";
        for (auto& op : func.getBody().getOps()) {
            sw::dfa::parseOperation(gr, op, os);
			op.dumpPretty();
        }
    }

    // Print or save the output
    std::cout << output << std::endl;

    // Print the graph
    std::cout << gr << std::endl;

    return 0;
}
