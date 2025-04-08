#pragma once
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
#include <queue>

// this module should not be used alone
// we are enforcing that by not including the dfg.hpp header
// That dfg.hpp header brings all these dependencies together

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

        static constexpr bool bTrace = false;

        // Function to extract Const specific attributes
        DomainFlowNode parseConst(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {
            auto constOp = mlir::cast<mlir::tosa::ConstOp>(op);
            // Parse basic operation information
            if constexpr (bTrace) os << "TOSA Const Operation:\n";
            // Parse operands
            // constOp does not have any operands
            // Parse attributes
            std::vector<AttributeInfo> attributes = parseAttributes(op);
            if constexpr (bTrace)  os << "Attributes (" << attributes.size() << "):\n";
            //for (const auto& attr : attributes) {
            //    os << "  " << attr.name << "\n";
            //    //os << "  " << attr.name << ": " << attr.valueStr << "\n";
            //}
            // report result type
            if constexpr (bTrace) os << "Result:\n";
            if constexpr (bTrace) os << "  " << constOp.getOutput().getType() << "\n";
        
            // TODO: bring the attribute processing up to this function

            return DomainFlowNode(DomainFlowOperator::CONSTANT, "ConstOp");
        }

        // Function to extract Clamp specific attributes
        ClampAttributes parseClampAttributes(mlir::tosa::ClampOp clampOp) {
            ClampAttributes result{};

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
        DomainFlowNode parseTosaClamp(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            auto clampOp = mlir::cast<mlir::tosa::ClampOp>(op);
            // Parse Clamp specific attributes
            ClampAttributes clampAttrs = parseClampAttributes(clampOp);

            // Parse basic operation information
            if constexpr (bTrace) {
                os << "TOSA Clamp Operation:\n";

                // Parse operands
                os << "Operands:\n";
                os << "  Input: " << clampOp.getInput().getType() << "\n";


                os << "Attributes:\n";
                os << "  Min Int: " << clampAttrs.minInt << "\n";
                os << "  Max Int: " << clampAttrs.maxInt << "\n";
                os << "  Min FP: " << clampAttrs.minFp << "\n";
                os << "  Max FP: " << clampAttrs.maxFp << "\n";


                // Parse result
                os << "Result:\n";
                os << "  " << clampOp.getOutput().getType() << "\n";
            }
            
            return DomainFlowNode(DomainFlowOperator::CLAMP, "Clamp");
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
        DomainFlowNode parseTosaConv2D(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            auto convOp = mlir::cast<mlir::tosa::Conv2DOp>(op);
            // Parse Conv2D specific attributes
            Conv2DAttributes convAttrs = parseConv2DAttributes(convOp);

            // Parse basic operation information
            if constexpr (bTrace) {
                os << "TOSA Conv2D Operation:\n";

                // Parse operands
                os << "Operands:\n";
                os << "  Input: " << convOp.getInput().getType() << "\n";
                os << "  Weight: " << convOp.getWeight().getType() << "\n";
                if (convOp.getBias())
                    os << "  Bias: " << convOp.getBias().getType() << "\n";


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

                // Parse result
                os << "Result:\n";
                os << "  " << convOp.getOutput().getType() << "\n";
            }
            
			// TODO: bring the attribute processing up to this function

			return DomainFlowNode(DomainFlowOperator::CONV2D, "Conv2D");
        }

        // A specialized function to parse TOSA Conv2D operations
        DomainFlowNode parseTosaConv3D(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {
            return DomainFlowNode(DomainFlowOperator::CONV3D, "Conv3D");
        }

        // A specialized function to parse TOSA Reshape operations
        DomainFlowNode parseTosaReshape(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::RESHAPE, "Reshape");
        }
        // A specialized function to parse TOSA Transpose operations
        DomainFlowNode parseTosaTranspose(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::TRANSPOSE, "Transpose");
        }
        // A specialized function to parse TOSA DepthwiseConv2D operations
        DomainFlowNode parseTosaDepthwiseConv2D(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::DEPTHWISE_CONV2D, "DepthwiseConv2D");
        }
        // A specialized function to parse TOSA TransposeConv2D operations
        DomainFlowNode parseTosaTransposeConv2D(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::TRANSPOSE_CONV2D, "TransposeConv2D");
        }
        // A specialized function to parse TOSA FullyConnected operations
        DomainFlowNode parseTosaFullyConnected(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::FC, "FullyConnected");
        }
        // A specialized function to parse TOSA Matmul operations
        DomainFlowNode parseTosaMatmul(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::MATMUL, "Matmul");
        }
        // A specialized function to parse TOSA Add operations
        DomainFlowNode parseTosaAdd(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::ADD, "Add");
        }
        // A specialized function to parse TOSA Sub operations
        DomainFlowNode parseTosaSub(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::SUB, "Sub");
        }
        // A specialized function to parse TOSA Mul operations
        DomainFlowNode parseTosaMul(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::MUL, "Mul");
        }
        // A specialized function to parse TOSA Negate operations
        DomainFlowNode parseTosaNegate(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::NEGATE, "Negate");
        }

        // A specialized function to parse TOSA Pad operations
        DomainFlowNode parseTosaPad(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::PAD, "Pad");
        }
        // A specialized function to parse TOSA Cast operations
        DomainFlowNode parseTosaCast(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::CAST, "Cast");
        }
        // A specialized function to parse TOSA Gather operations
        DomainFlowNode parseTosaGather(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::GATHER, "Gather");
        }

        // function ops
        // A specialized function to parse TOSA Reciprocal operations
        DomainFlowNode parseTosaReciprocal(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::RECIPROCAL, "Reciprocal");
        }
        // A specialized function to parse TOSA ReduceAll operations
        DomainFlowNode parseTosaReduceAll(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::REDUCE_ALL, "ReduceAll");
        }
        // A specialized function to parse TOSA ReduceMax operations
        DomainFlowNode parseTosaReduceMax(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::REDUCE_MAX, "ReduceMax");
        }
        // A specialized function to parse TOSA ReduceMin operations
        DomainFlowNode parseTosaReduceMin(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::REDUCE_MIN, "ReduceMin");
        }
        // A specialized function to parse TOSA ReduceSum operations
        DomainFlowNode parseTosaReduceSum(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::REDUCE_SUM, "ReduceSum");
        }
        // A specialized function to parse TOSA ReduceProd operations
        DomainFlowNode parseTosaReduceProd(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::REDUCE_PROD, "ReduceProd");
        }

        // A specialized function to parse TOSA Exp operations
        DomainFlowNode parseTosaExp(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::EXP, "Exp");
        }
        // A specialized function to parse TOSA Abs operations
        DomainFlowNode parseTosaAbs(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::ABS, "Abs");
        }
        // A specialized function to parse TOSA Concat operations
        DomainFlowNode parseTosaConcat(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {

			return DomainFlowNode(DomainFlowOperator::CONCAT, "Concat");
        }


        // Parse the TOSA Op and add to the graph
        DomainFlowNode parseOperation(domain_flow_graph& gr, mlir::Operation& op, llvm::raw_ostream& os) {
            if (mlir::isa<mlir::tosa::ConstOp>(op)) {
                return parseConst(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::Conv2DOp>(op)) {
                return parseTosaConv2D(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::Conv3DOp>(op)) {
                return parseTosaConv3D(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ClampOp>(op)) {
                return parseTosaClamp(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ReshapeOp>(op)) {
                return parseTosaReshape(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::TransposeOp>(op)) {
                return parseTosaTranspose(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::DepthwiseConv2DOp>(op)) {
                return parseTosaDepthwiseConv2D(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::TransposeConv2DOp>(op)) {
                return parseTosaTransposeConv2D(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::PadOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA PadOp:\n";
                parseTosaPad(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::FullyConnectedOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA FullyConnectedOp:\n";
                parseTosaFullyConnected(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::MatMulOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA MatMulOp:\n";
                parseTosaMatmul(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::AddOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA AddOp:\n";
                parseTosaAdd(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::SubOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA SubOp:\n";
                parseTosaSub(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::MulOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA MulOp:\n";
                parseTosaMul(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::NegateOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA NegateOp:\n";
                parseTosaNegate(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ExpOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ExpOp:\n";
                parseTosaExp(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::AbsOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA AbsOp:\n";
                parseTosaAbs(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ConcatOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ConcatOp:\n";
                parseTosaConcat(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::CastOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA CastOp:\n";
                parseTosaCast(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::GatherOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA GatherOp:\n";
                parseTosaGather(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ReciprocalOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ReciprocalOp:\n";
                parseTosaReciprocal(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ReduceAllOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ReduceAllOp:\n";
                parseTosaReduceAll(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ReduceMaxOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ReduceMaxOp:\n";
                parseTosaReduceMax(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ReduceMinOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ReduceMinOp:\n";
                parseTosaReduceMin(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ReduceSumOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ReduceSumOp:\n";
                parseTosaReduceSum(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ReduceProdOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ReduceProdOp:\n";
                parseTosaReduceProd(gr, op, os);
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

            if (op.getNumRegions() > 0) {
                // Parse blocks
                parseBlocks(op, os);
            }
        }


    }
}