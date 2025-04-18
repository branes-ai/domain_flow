#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

	std::string graphName = "matmul";
	DomainFlowGraph nla(graphName); // Numerical Linear Algebra

	size_t SLOT_A = 0;
	size_t SLOT_B = 1;

	// model a single layer Multi Level Perceptron using a Linear operator,
	// which consists of an input, a weights matrix, and a bias
	constexpr size_t weightsOutputSlot = 0;
	auto weights = DomainFlowNode(DomainFlowOperator::CONSTANT, "weights").addResult(weightsOutputSlot, "weights", "tensor<4x256x16xf32>");
	// weights are a constant tensor of 4x256x16
	constexpr size_t inputOutputSlot = 0;
	auto input = DomainFlowNode(DomainFlowOperator::FUNCTION_ARGUMENT, "inputVector").addOperand(inputOutputSlot, "tensor<4x256xf32");
	// matmul takes an input tensor, a weights matrix
	// batch of 4, 256 element vectors input, with a 256x16 weight matrix to generate 16 categories
	constexpr size_t matmulInputSlot_A = 0;
	constexpr size_t matmulInputSlot_B = 1;
	constexpr size_t matmulOutputSlot = 0;
	auto matmul = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(matmulInputSlot_A, "tensor<4x256xf32").addOperand(matmulInputSlot_B, "tensor<4x256x16xf32>").addResult(matmulOutputSlot, "out", "tensor<4x16xf32>");
	// sigmoid takes the output of the linear layer and applies the Sigmoid activation function
	constexpr size_t sigmoidInputSlot_A = 0;
	constexpr size_t sigmoidOutputSlot = 0;
	auto sigmoid = DomainFlowNode(DomainFlowOperator::SIGMOID, "sigmoid").addOperand(sigmoidInputSlot_A, "tensor<16xf32>").addResult(sigmoidOutputSlot, "out", "tensor<16xf32>");
	// output result
	constexpr size_t outputInputSlot_A = 0;
	auto output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "output").addOperand(outputInputSlot_A, "tensor<16xf32>").addAttribute("target", "memory");

	auto weightsId = nla.addNode(weights);
	auto inputId = nla.addNode(input);
	auto matmulId = nla.addNode(matmul);
	auto sigmoidId = nla.addNode(sigmoid);
	auto outputId = nla.addNode(output);
	// Add edges between the nodes
	DomainFlowEdge df0(0, true, "tensor<4x256x16>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(weightsId, SLOT_A, matmulId, matmulInputSlot_B, df0);
	DomainFlowEdge df1(0, true, "tensor<4x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(inputId, inputOutputSlot, matmulId, matmulInputSlot_A, df1);

	DomainFlowEdge df2(0, false, "tensor<4x256x16>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(matmulId, matmulOutputSlot, sigmoidId, sigmoidInputSlot_A, df2);
	DomainFlowEdge df3(0, false, "tensor<16>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(sigmoidId, sigmoidOutputSlot, outputId, outputInputSlot_A, df3);

	// generate the graph order
	nla.assignNodeDepths();

	// assign sources and sinks
	nla.addSource(weightsId);
	nla.addSource(inputId);
	nla.addSink(outputId);

	// report on the operator statistics
	reportOperatorStats(nla);

	// Save the graph to a file
	std::string dfgFilename = graphName + ".dfg";
	dfgFilename = generateDataOutputFile(std::string("workloads/nla/") + dfgFilename);  // stick it in the data directory
	nla.graph.save(dfgFilename);

	std::cout << "Saved graph to: " << dfgFilename << std::endl;    
	
	return EXIT_SUCCESS;
}
