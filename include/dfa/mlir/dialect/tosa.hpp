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
#include <dfa/graph/graph.hpp>

namespace sw {
    namespace dfa {

        // DL Graph node type
        struct TosaOperator {
            std::string operatorName;
            int depth;   // 0 is a source
            std::vector<std::string> resultValue;   // string version of mlir::Value
            std::vector<std::string> resultType;     // string version of mlir::Type

            // Constructor to initialize the node with just a string of the operator
            TosaOperator(std::string name) : operatorName{ name }, depth{ 0 } {}
            void setDepth(int d) { depth = d; }
            void setOperator(std::string name) { this->operatorName = name; }

            void addResult(const std::string& valueStr, const std::string& typeStr) {
                resultValue.push_back(valueStr);
                resultType.push_back(typeStr);
            }
            std::string getName() const noexcept { return operatorName; }
            int getDepth() const noexcept { return depth; }
            std::string getResultValue(std::size_t idx) const noexcept { return resultValue[idx]; }
            std::string getResultType(std::size_t idx) const noexcept { return resultType[idx]; }
        };
        std::ostream& operator<<(std::ostream& ostr, const TosaOperator& op) {
            ostr << op.operatorName << " at depth " << op.depth;
            for (std::size_t idx = 0; idx < op.resultValue.size(); ++idx) {
                ostr << " -> " << op.resultValue[idx] << " of type " << op.resultType[idx];
            }
            return ostr;
        }

        // DL Graph edge type
        struct DataFlow : public graph::weighted_edge<int> { // Weighted by the data flow on this link
            int flow;
            bool stationair;  // does the flow go through a memory or not

            int weight() const noexcept override {
                return flow;
            }
            DataFlow(int flow, bool stationair = true) : flow{ flow }, stationair{ stationair } {}
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

        static constexpr bool bTrace = false;

        // Function to extract Const specific attributes
        void parseConst(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {
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

            // TODO: how do you parse the type and stick it in the graph?
            //gr.add_node("ConstOp");
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
        void parseTosaClamp(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

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
            // Add the Clamp operation to the graph
            //gr.add_node("Clamp");
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
        void parseTosaConv2D(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

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
            // Add the Conv2D operation to the graph
            //gr.add_node("Conv2D");
        }

        // A specialized function to parse TOSA Reshape operations
        void parseTosaReshape(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Reshape operation to the graph
            //gr.add_node("Reshape");
        }
        // A specialized function to parse TOSA Transpose operations
        void parseTosaTranspose(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Transpose operation to the graph
            //gr.add_node("Transpose");
        }
        // A specialized function to parse TOSA DepthwiseConv2D operations
        void parseTosaDepthwiseConv2D(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add DepthwiseConv2D operation to the graph
            //gr.add_node("DepthwiseConv2D");
        }
        // A specialized function to parse TOSA TransposeConv2D operations
        void parseTosaTransposeConv2D(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add TransposeConv2D operation to the graph
            //gr.add_node("TransposeConv2D");
        }
        // A specialized function to parse TOSA FullyConnected operations
        void parseTosaFullyConnected(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add FullyConnected operation to the graph
            //gr.add_node("FullyConnected");
        }
        // A specialized function to parse TOSA Matmul operations
        void parseTosaMatmul(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add matmul operation to the graph
            //gr.add_node("Matmul");
        }
        // A specialized function to parse TOSA Add operations
        void parseTosaAdd(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Add operation to the graph
            //gr.add_node("Add");
        }
        // A specialized function to parse TOSA Sub operations
        void parseTosaSub(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Sub operation to the graph
            //gr.add_node("Sub");
        }
        // A specialized function to parse TOSA Mul operations
        void parseTosaMul(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Mul operation to the graph
            //gr.add_node("Mul");
        }
        // A specialized function to parse TOSA Negate operations
        void parseTosaNegate(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Negate operation to the graph
            //gr.add_node("Negate");
        }

        // A specialized function to parse TOSA Pad operations
        void parseTosaPad(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Pad operation to the graph
            //gr.add_node("Pad");
        }
        // A specialized function to parse TOSA Cast operations
        void parseTosaCast(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Cast operation to the graph
            //gr.add_node("Cast");
        }
        // A specialized function to parse TOSA Gather operations
        void parseTosaGather(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Gather operation to the graph
            //gr.add_node("Gather");
        }

        // function ops
        // A specialized function to parse TOSA Reciprocal operations
        void parseTosaReciprocal(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Reciprocal operation to the graph
            //gr.add_node("Reciprocal");
        }
        // A specialized function to parse TOSA ReduceAll operations
        void parseTosaReduceAll(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add ReduceAll operation to the graph
            //gr.add_node("ReduceAll");
        }
        // A specialized function to parse TOSA ReduceMax operations
        void parseTosaReduceMax(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add ReduceMax operation to the graph
            //gr.add_node("ReduceMax");
        }
        // A specialized function to parse TOSA ReduceMin operations
        void parseTosaReduceMin(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add ReduceMin operation to the graph
            //gr.add_node("ReduceMin");
        }
        // A specialized function to parse TOSA ReduceSum operations
        void parseTosaReduceSum(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add ReduceSum operation to the graph
            //gr.add_node("ReduceSum");
        }
        // A specialized function to parse TOSA ReduceProd operations
        void parseTosaReduceProd(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add ReduceProd operation to the graph
            //gr.add_node("ReduceProd");
        }

        // A specialized function to parse TOSA Exp operations
        void parseTosaExp(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Exp operation to the graph
            //gr.add_node("Exp");
        }
        // A specialized function to parse TOSA Abs operations
        void parseTosaAbs(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Abs operation to the graph
            //gr.add_node("Abs");
        }
        // A specialized function to parse TOSA Concat operations
        void parseTosaConcat(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {

            // add Concat operation to the graph
            //gr.add_node("Concat");
        }




        // Parse the TOSA Op and add to the graph
        void parseOperation(graph::directed_graph<TosaOperator, DataFlow>& gr, mlir::Operation& op, llvm::raw_ostream& os) {
            if (mlir::isa<mlir::tosa::ConstOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ConstOp:\n";
                parseConst(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::Conv2DOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA Conv2DOp:\n";
                parseTosaConv2D(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ClampOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ClampOp:\n";
                parseTosaClamp(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::ReshapeOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA ReshapeOp:\n";
                parseTosaReshape(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::TransposeOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA TransposeOp:\n";
                parseTosaTranspose(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::DepthwiseConv2DOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA DepthwiseConv2DOp:\n";
                parseTosaDepthwiseConv2D(gr, op, os);
            }
            else if (mlir::isa<mlir::tosa::TransposeConv2DOp>(op)) {
                if constexpr (bTrace) os << "\nDetected TOSA TransposeConv2DOp:\n";
                parseTosaTransposeConv2D(gr, op, os);
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