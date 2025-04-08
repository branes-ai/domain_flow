#pragma once
#if WIN32
#pragma warning(disable : 4244 4267 4996)
#endif

#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/Attributes.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <vector>
#include <queue>

#include <dfa/dfg.hpp>
#include <dfa/mlir/dialect/tosa.hpp>

namespace sw {
    namespace dfa {


        // Assign depth values to nodes based on their maximum distance from inputs
        void assignNodeDepths(domain_flow_graph& gr, llvm::raw_string_ostream& os) {
			constexpr bool bTrace = false; // Set to true for detailed tracing
            auto nodeDepths = calculateNodeDepths(gr);
            // Store depth values in the TosaOperator objects
            for (int i = 0; i < gr.nrNodes(); ++i) {
                // Access node data and set depth
                DomainFlowNode& op = gr.node(i);
                op.setDepth(nodeDepths[i]);
                if constexpr (bTrace) os << "Node " << i << " final depth: " << nodeDepths[i] << "\n";
            }
        }


		// Function to process the MLIR module and build the domain flow graph
        void processModule(DomainFlowGraph& dfg, mlir::ModuleOp& module) {
            domain_flow_graph& gr = dfg.graph;
			constexpr bool bTrace = false; // Set to true for detailed tracing

            std::string output;
            llvm::raw_string_ostream os(output);

            // Map to store operation to node ID mapping
            std::map<mlir::Operation*, int> opToNodeId;

			// For each function in the module, build the domain flow graph
            for (auto func : module.getOps<mlir::func::FuncOp>()) {
                os << "Processing function: " << func.getName() << "\n";

                // Handle function arguments as potential input nodes
                for (unsigned i = 0; i < func.getNumArguments(); ++i) {
                    std::string argName = "arg" + std::to_string(i);
                    int nodeId = gr.add_node(DomainFlowNode(DomainFlowOperator::FUNCTION_ARGUMENT, argName));
                    opToNodeId[nullptr] = nodeId; // Special case for function arguments
                }

                // First pass: Create nodes for all operations
                for (auto& op : func.getBody().getOps()) {                  
					auto graphNode = parseOperation(gr, op, os); // Parse the operation

					int nrOperands = op.getNumOperands();
                    for (int idx = 0; idx < nrOperands; ++idx) {
                        mlir::Value operand = op.getOperand(idx);
                        mlir::Type operandType = operand.getType();
                        // Convert operandType (Type) to string
                        std::string typeStr;
                        llvm::raw_string_ostream typeOs(typeStr);
                        operandType.print(typeOs);  // Print the type (e.g., f32, i32, tensor<...>)
                        typeOs.flush();  // Ensure the string is populated
                        graphNode.addOperand(typeStr);
                    }

                    // Enumerate all results (outputs) of the operation
                    int nrResults = op.getNumResults();
                    for (int idx = 0; idx < nrResults; ++idx) {

                        // mlir::Value::print() outputs a more verbose representation, 
                        // often including the operation that defines the value, rather 
                        // than just the SSA name (e.g., %0 = "op"() instead of just %0). 
                        // To get only the SSA symbolic name, MLIR doesn't provide a 
                        // direct method to extract just the name as a string, but we 
                        // can work around this by leveraging the fact that SSA values 
                        // are typically represented as % followed by an identifier (like %0, %1, etc.) in the IR.

                        // Since print() gives us more than we want, and there’s no built - in API to directly 
                        // get just the SSA name as a string, we can either :
                        // 
                        // 1. Use result.getName() if you're in a context where values have been assigned debug names (rare in most IRs), or
                        // 2. Generate the SSA name ourselves based on the result index and operation context, or
                        // 3. Parse the output of print() to extract the SSA name.
                        // The simplest and most reliable approach here, given our use case, is to construct 
                        // the SSA name manually using the result index, since MLIR’s default SSA names are 
                        // predictable(% 0, % 1, etc.) within the operation’s scope.
                        // However, if you need the exact name as it appears in the IR(accounting for potential custom naming), 
                        // we’ll need to parse the output.

                        mlir::Value result = op.getResult(idx);
                        mlir::Type resultType = result.getType();

                        // Convert result (Value) to string
                        std::string valueStr;
                        //llvm::raw_string_ostream valueOs(valueStr);
                        //result.print(valueOs);  // Print the SSA value (e.g., %0, %1, etc.)
                        //valueOs.flush();  // Ensure the string is populated
						valueStr = std::string("result_") + std::to_string(idx); // Use the index as a simple identifier

                        // Convert resultType (Type) to string
                        std::string typeStr;
                        llvm::raw_string_ostream typeOs(typeStr);
                        resultType.print(typeOs);  // Print the type (e.g., f32, i32, tensor<...>)
                        typeOs.flush();  // Ensure the string is populated

                        // Add the string representations to the graph node
                        graphNode.addResult(valueStr, typeStr);
                        // Create a name for this result
                        //std::string resultName = opName + "_result" + std::to_string(idx);

                    }
                    
                    int nodeId = gr.add_node(graphNode);
                    opToNodeId[&op] = nodeId;
                    if constexpr (bTrace) os << "Created node: " << graphNode.getName() << " with ID: " << nodeId << "\n";
                }

                // Second pass: Create edges based on operand-result relationships
                for (auto& op : func.getBody().getOps()) {
                    int srcNodeId = opToNodeId[&op];
                    std::string opName = op.getName().getStringRef().str();
                    if constexpr (bTrace) os << "Processing edges for operation: " << opName << "\n";

                    // For each operand, find its defining operation and add an edge
                    for (unsigned i = 0; i < op.getNumOperands(); ++i) {
                        mlir::Value operand = op.getOperand(i);

                        // Get the defining operation of the operand
                        if (auto definingOp = operand.getDefiningOp()) {
                            // The edge direction is from defining op to current op (data flows from producer to consumer)
                            int destNodeId = srcNodeId;
                            int srcDefiningNodeId = opToNodeId[definingOp];

                            DomainFlowEdge flow(1);

                            // Add edge: from defining op to current op
                            gr.add_edge(srcDefiningNodeId, destNodeId, flow);

                            std::string definingOpName = definingOp->getName().getStringRef().str();
                            if constexpr (bTrace) os << "  Added edge: " << definingOpName << " -> " << opName
                                << " (NodeIDs: " << srcDefiningNodeId << " -> " << destNodeId << ")\n";
                        }
                        else if (operand.isa<mlir::BlockArgument>()) {
                            // Handle block arguments (function inputs)
                            auto blockArg = operand.cast<mlir::BlockArgument>();
                            int argIdx = blockArg.getArgNumber();
                            std::string argName = "arg" + std::to_string(argIdx);

                            // Use the special node ID we created for arguments
                            int srcArgNodeId = opToNodeId[nullptr];  // This is simplified - in reality you might want to map each arg separately
                            int destNodeId = srcNodeId;

                            DomainFlowEdge flow(1);

                            // Add edge: from argument to current op
                            gr.add_edge(srcArgNodeId, destNodeId, flow);

                            if constexpr (bTrace) os << "  Added edge from function argument " << argIdx << " to " << opName
                                << " (NodeIDs: " << srcArgNodeId << " -> " << destNodeId << ")\n";
                        }
                        else {
                            if constexpr (bTrace) os << "  Operand " << i << " has no defining operation (might be a constant or external input)\n";
                        }
                    }
                }
            
                // Handle function results by finding return operations and their operands
                for (auto& returnOp : llvm::make_early_inc_range(func.getOps<mlir::func::ReturnOp>())) {
                    // Return operations don't have names like other operations, so use the operation name directly
                    std::string returnName = "func.return";
                    if constexpr (bTrace) os << "Processing return operation: " << returnName << "\n";

                    // Create result nodes
                    for (unsigned i = 0; i < returnOp.getNumOperands(); ++i) {
                        std::string resultName = "result" + std::to_string(i);
                        int resultNodeId = gr.add_node(DomainFlowNode(resultName));
                        if constexpr (bTrace) os << "Created result node: " << resultName << " with ID: " << resultNodeId << "\n";

                        // Get the operand that is being returned
                        mlir::Value resultValue = returnOp.getOperand(i);

                        // Get the defining operation of this result value
                        if (auto definingOp = resultValue.getDefiningOp()) {
                            int definingOpNodeId = opToNodeId[definingOp];

                            // Create an edge from the defining op to the result node
                            DomainFlowEdge flow(1);
                            gr.add_edge(definingOpNodeId, resultNodeId, flow);

                            std::string definingOpName = definingOp->getName().getStringRef().str();
                            if constexpr (bTrace) os << "  Added edge: " << definingOpName << " -> " << resultName
                                << " (NodeIDs: " << definingOpNodeId << " -> " << resultNodeId << ")\n";
                        }
                        else if (resultValue.isa<mlir::BlockArgument>()) {
                            // Handle block arguments that are directly returned
                            auto blockArg = resultValue.cast<mlir::BlockArgument>();
                            int argIdx = blockArg.getArgNumber();
                            std::string argName = "arg" + std::to_string(argIdx);

                            // Find the corresponding argument node ID
                            int argNodeId = -1;
                            for (auto& entry : opToNodeId) {
                                if (entry.first == nullptr) {  // This is a simplification - you'd need better tracking for multiple args
                                    argNodeId = entry.second;
                                    break;
                                }
                            }

                            if (argNodeId != -1) {
                                // Create an edge from the argument node to the result node
                                DomainFlowEdge flow(1);
                                gr.add_edge(argNodeId, resultNodeId, flow);

                                if constexpr (bTrace) os << "  Added edge from function argument " << argIdx << " to result " << i
                                    << " (NodeIDs: " << argNodeId << " -> " << resultNodeId << ")\n";
                            }
                        }
                    }
                }

				// at this point, the graph structure is built, 
                // and we can order the nodes according to the distance from the inputs
				assignNodeDepths(gr, os);
            }

            // Print the graph construction trace
            std::cout << output << std::endl;
        }

    }
}