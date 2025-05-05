#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

	std::string graphName = "matmul_linear";
	DomainFlowGraph nla(graphName); // Numerical Linear Algebra

	size_t SLOT_A = 0;
	size_t SLOT_B = 1;

	// model of a single layer Multi Level Perceptron using a Linear operator, 
	// which consists of an input, a weights matrix, and a bias.
	constexpr size_t weightsOutputSlot = 0;
	// weights are a constant tensor of 4x256x16
	auto weights = DomainFlowNode(DomainFlowOperator::CONSTANT, "constant.weights").addResult(weightsOutputSlot, "weights", "tensor<256x16xf32>");
	// function arguments are an input tensor of 4x256
	constexpr size_t inputOutputSlot = 0;
	auto input = DomainFlowNode(DomainFlowOperator::FUNCTION_ARGUMENT, "inputVector").addResult(inputOutputSlot, "arg", "tensor<4x256xf32>");
	// matmul takes an input tensor, a weights matrix
	// batch of 4, 256 element vectors input, with a 256x16 weight matrix to generate 16 categories
	constexpr size_t slot_A = 0;
	constexpr size_t slot_B = 1;
	constexpr size_t slot_O = 0;
	auto matmul = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(slot_A, "tensor<4x256xf32>").addOperand(slot_B, "tensor<256x16xf32>").addResult(slot_O, "out", "tensor<4x16xf32>");
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
	DomainFlowEdge df0(0, true, "tensor<256x16>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(weightsId, SLOT_A, matmulId, slot_B, df0);
	DomainFlowEdge df1(0, true, "tensor<4x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(inputId, inputOutputSlot, matmulId, slot_A, df1);

	DomainFlowEdge df2(0, false, "tensor<4x16>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(matmulId, slot_O, sigmoidId, sigmoidInputSlot_A, df2);
	DomainFlowEdge df3(0, false, "tensor<4x16>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(sigmoidId, sigmoidOutputSlot, outputId, outputInputSlot_A, df3);

	// generate the graph order
	nla.assignNodeDepths();

	// assign sources and sinks
	nla.addSource(weightsId);
	nla.addSource(inputId);
	nla.addSink(outputId);

	std::cout << nla << '\n';

	// report on the operator statistics
	reportOperatorStats(nla);
	reportArithmeticComplexity(nla);

	// Save the graph to a file
	std::string dfgFilename = graphName + ".dfg";
	dfgFilename = generateDataOutputFile(std::string("workloads/nla/") + dfgFilename);  // stick it in the data directory
	nla.save(dfgFilename);

	std::cout << "Saved graph to: " << dfgFilename << std::endl;
	
	return EXIT_SUCCESS;
}
