#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

	std::string graphName = "domain_flow";
	DomainFlowGraph dfa(graphName); // Numerical Linear Algebra

	// model a single layer Multi Level Perceptron using a Linear operator, 
	// which consists of an input, a weights matrix, and a bias
	auto input = DomainFlowNode(DomainFlowOperator::FUNCTION_ARGUMENT, "inputVector").addOperand("tensor<4x256xf32");
	// matmul takes an input tensor, a weights matrix
	// batch of 4, 256 element vectors input, with a 256x16 to 16 categories
	auto matmul = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand("tensor<4x256xf32").addOperand("tensor<4x256x16xf32>").addResult("out", "tensor<tensor<4x16xf32>");
	// relu takes the output of the linear layer and applies the ReLU activation function
	auto sigmoid = DomainFlowNode(DomainFlowOperator::SIGMOID, "sigmoid").addOperand("tensor<16xf32>").addResult("out", "tensor<16xf32>");
	// output result
	auto output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "output").addOperand("tensor<16xf32>");

	auto inputId = dfa.addNode(input);
	auto matmulId = dfa.addNode(matmul);
	auto sigmoidId = dfa.addNode(sigmoid);
	auto outputId = dfa.addNode(output);
	// Add edges between the nodes
	DomainFlowEdge df1(0, true, "tensor<4x256>", 32, { 1, 1, 1 });
	dfa.addEdge(inputId, matmulId, df1);
	DomainFlowEdge df2(0, false, "tensor<4x256x16>", 32, { 1, 1, 1 });
	dfa.addEdge(matmulId, sigmoidId, df2);
	DomainFlowEdge df3(0, false, "tensor<16>", 32, { 1, 1, 1 });
	dfa.addEdge(sigmoidId, outputId, df3);

	// generate the graph order
	dfa.assignNodeDepths();

	// report on the operator statistics
	reportOperatorStats(dfa);

	// Save the graph to a file
	std::string dfgFilename = graphName + ".dfg";
	dfgFilename = generateDataOutputFile(std::string("workloads/dfa/") + dfgFilename);  // stick it in the data directory
	dfa.graph.save(dfgFilename);

	std::cout << "Saved graph to: " << dfgFilename << std::endl;
    return EXIT_SUCCESS;
}
