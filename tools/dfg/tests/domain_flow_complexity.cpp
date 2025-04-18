#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>

int main() {
	using namespace sw::dfa;

    DomainFlowGraph dfg("my-test-graph");
 
    dfg.addNode(DomainFlowNode(DomainFlowOperator::ADD, "test.Add").addOperand(0, "tensor<4xf32>").addOperand(1, "tensor<4xf32>").addResult(0, "out", "tensor<4xf32>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::ADD, "test.Add").addOperand(0, "tensor<4xf16>").addOperand(1, "tensor<4xf16>").addResult(0, "out", "tensor<4xf16>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::ADD, "test.Add").addOperand(0, "tensor<4xf8>").addOperand(1, "tensor<4xf8>").addResult(0, "out", "tensor<4xf8>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::ADD, "test.Add").addOperand(0, "tensor<4xi32>").addOperand(1, "tensor<4xi32>").addResult(0, "out", "tensor<4xi32>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::ADD, "test.Add").addOperand(0, "tensor<4xi16>").addOperand(1, "tensor<4xi16>").addResult(0, "out", "tensor<4xi16>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::ADD, "test.Add").addOperand(0, "tensor<4xi8>").addOperand(1, "tensor<4xi8>").addResult(0, "out", "tensor<4xi8>"));

    dfg.addNode(DomainFlowNode(DomainFlowOperator::MUL, "test.Mul").addOperand(0, "tensor<1x4xf32>").addOperand(1, "tensor<1x4xf32>").addResult(0, "out", "tensor<1x4xf32>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MUL, "test.Mul").addOperand(0, "tensor<1x4xf16>").addOperand(1, "tensor<1x4xf16>").addResult(0, "out", "tensor<1x4xf16>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MUL, "test.Mul").addOperand(0, "tensor<1x4xf8>").addOperand(1, "tensor<1x4xf8>").addResult(0, "out", "tensor<1x4xf8>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MUL, "test.Mul").addOperand(0, "tensor<1x4xi32>").addOperand(1, "tensor<1x4xi32>").addResult(0, "out", "tensor<1x4xi32>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MUL, "test.Mul").addOperand(0, "tensor<1x4xi16>").addOperand(1, "tensor<1x4xi16>").addResult(0, "out", "tensor<1x4xi16>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MUL, "test.Mul").addOperand(0, "tensor<1x4xi8>").addOperand(1, "tensor<1x4xi8>").addResult(0, "out", "tensor<1x4xi8>"));

    dfg.addNode(DomainFlowNode(DomainFlowOperator::MATMUL, "test.Matmul").addOperand(0, "tensor<1x4x4xf32>").addOperand(1, "tensor<1x4x4xf32>").addResult(0, "out", "tensor<1x4x4xf32>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MATMUL, "test.Matmul").addOperand(0, "tensor<2x4x4xf16>").addOperand(1, "tensor<1x4x4xf16>").addResult(0, "out", "tensor<1x4x4xf16>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MATMUL, "test.Matmul").addOperand(0, "tensor<4x4x4xf8>").addOperand(1, "tensor<1x4x4xf8>").addResult(0, "out", "tensor<1x4x4xf8>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MATMUL, "test.Matmul").addOperand(0, "tensor<8x4x4xi32>").addOperand(1, "tensor<1x4x4xi32>").addResult(0, "out", "tensor<1x4x4xi32>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MATMUL, "test.Matmul").addOperand(0, "tensor<16x4x4xi16>").addOperand(1, "tensor<1x4x4xi16>").addResult(0, "out", "tensor<1x4x4xi16>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::MATMUL, "test.Matmul").addOperand(0, "tensor<32x4x4xi8>").addOperand(1, "tensor<1x4x4xi8>").addResult(0, "out", "tensor<1x4x4xi8>"));

    // TOSA compliant 4D tensor = (N, H, W, C)
    dfg.addNode(DomainFlowNode(DomainFlowOperator::CONV2D, "test.Conv2D").addOperand(0, "tensor<1x224x224x3xf32>").addOperand(1, "tensor<32x3x3x3xf32>").addResult(0, "out", "tensor<1x112x112x32xf32>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::CONV2D, "test.Conv2D").addOperand(0, "tensor<1x224x224x3xf16>").addOperand(1, "tensor<32x3x3x3xf16>").addResult(0, "out", "tensor<1x112x112x32xf16>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::CONV2D, "test.Conv2D").addOperand(0, "tensor<1x224x224x3xf8>").addOperand(1, "tensor<32x3x3x3xf8>").addResult(0, "out", "tensor<1x112x112x32xf8>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::CONV2D, "test.Conv2D").addOperand(0, "tensor<1x224x224x3xsi32>").addOperand(1, "tensor<32x3x3x3xsi32>").addResult(0, "out", "tensor<1x112x112x32xsi32>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::CONV2D, "test.Conv2D").addOperand(0, "tensor<1x224x224x3xsi16>").addOperand(1, "tensor<32x3x3x3xsi16>").addResult(0, "out", "tensor<1x112x112x32xsi16>"));
    dfg.addNode(DomainFlowNode(DomainFlowOperator::CONV2D, "test.Conv2D").addOperand(0, "tensor<1x224x224x3xsi8>").addOperand(1, "tensor<32x3x3x3xsi8>").addResult(0, "out", "tensor<1x112x112x32xsi8>"));

    reportOperatorStats(dfg);
    reportArithmeticComplexity(dfg);
    reportNumericalComplexity(dfg);

    return EXIT_SUCCESS;
}