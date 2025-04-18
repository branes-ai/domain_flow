#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

	std::string graphName = "matmul";
	DomainFlowGraph nla(graphName); // Numerical Linear Algebra

	// model a single layer Multi Level Perceptron using a Linear operator, 
	// which consists of an input, a weights matrix, and a bias
	auto input = DomainFlowNode(DomainFlowOperator::FUNCTION_ARGUMENT, "inputVector").addOperand("tensor<4x256xf32");
	// matmul takes an input tensor, a weights matrix
	// batch of 4, 256 element vectors input, with a 256x16 to 16 categories
	auto matmul = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand("tensor<4x256xf32").addOperand("tensor<4x256x16xf32>").addResult("out", "tensor<4x16xf32>");
	// relu takes the output of the linear layer and applies the ReLU activation function
	auto sigmoid = DomainFlowNode(DomainFlowOperator::SIGMOID, "sigmoid").addOperand("tensor<16xf32>").addResult("out", "tensor<16xf32>");
	// output result
	auto output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "output").addOperand("tensor<16xf32>").addAttribute("target", "memory");

	auto inputId = nla.addNode(input);
	auto matmulId = nla.addNode(matmul);
	auto sigmoidId = nla.addNode(sigmoid);
	auto outputId = nla.addNode(output);
	// Add edges between the nodes
	DomainFlowEdge df1(0, true, "tensor<4x256>", 32, { 1, 1, 1 });
	nla.addEdge(inputId, matmulId, df1);
	DomainFlowEdge df2(0, false, "tensor<4x256x16>", 32, { 1, 1, 1 });
	nla.addEdge(matmulId, sigmoidId, df2);
	DomainFlowEdge df3(0, false, "tensor<16>", 32, { 1, 1, 1 });
	nla.addEdge(sigmoidId, outputId, df3);

	// generate the graph order
	nla.assignNodeDepths();

	// report on the operator statistics
	reportOperatorStats(nla);

	// Save the graph to a file
	std::string dfgFilename = graphName + ".dfg";
	dfgFilename = generateDataOutputFile(std::string("workloads/nla/") + dfgFilename);  // stick it in the data directory
	nla.graph.save(dfgFilename);

	std::cout << "Saved graph to: " << dfgFilename << std::endl;    
	
	return EXIT_SUCCESS;
}
