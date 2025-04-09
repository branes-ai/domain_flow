#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;


    DomainFlowGraph dfg("my-test-graph");
	DomainFlowNode nodeAdd = DomainFlowNode(DomainFlowOperator::ADD, "test.Add").addOperand("tensor<4xf32>").addResult("out", "tensor<4xf32>");
	DomainFlowNode nodeMul = DomainFlowNode(DomainFlowOperator::MUL, "test.Mul").addOperand("tensor<1x4xf32>").addResult("out", "tensor<1x4xf32>");
	DomainFlowNode nodeMatmul = DomainFlowNode(DomainFlowOperator::MATMUL, "test.Matmul").addOperand("tensor<1x4x4xf32>").addOperand("tensor<1x4x4xf32>").addResult("out", "tensor<1x4x4xf32>");
    
    dfg.addNode(nodeAdd);
	dfg.addNode(nodeMul);
    dfg.addNode(nodeMatmul);

    dfg.graph.save(std::cout);

    dfg.graph.save("test.dfg");
    DomainFlowGraph dfg2("serialized graph");
    dfg2.graph.load("test.dfg");
    std::cout << dfg2 << std::endl;

    return EXIT_SUCCESS;
}
