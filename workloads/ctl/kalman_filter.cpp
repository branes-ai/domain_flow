#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

	std::string graphName = "kalman_filter";
	DomainFlowGraph ctl(graphName); // model predictive control

	// model a single layer Multi Level Perceptron using a Linear operator, 
	// which consists of an input, a weights matrix, and a bias
	auto weights = DomainFlowNode(DomainFlowOperator::CONSTANT, "weights").addOperand(0, "tensor<4x256x16xf32>");
	auto input = DomainFlowNode(DomainFlowOperator::FUNCTION_ARGUMENT, "inputVector").addOperand(0, "tensor<4x256xf32");
	// matmul takes an input tensor, a weights matrix
	// batch of 4, 256 element vectors input, with a 256x16 to 16 categories
	auto matmul = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(0, "tensor<4x256xf32").addOperand(1, "tensor<4x256x16xf32>").addResult(0, "out", "tensor<tensor<4x16xf32>");
	// sigmoid takes the output of the linear layer and applies the Sigmoid activation function
	auto sigmoid = DomainFlowNode(DomainFlowOperator::SIGMOID, "sigmoid").addOperand(0, "tensor<16xf32>").addResult(0, "out", "tensor<16xf32>");
	// output result
	auto output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "output").addOperand(0, "tensor<16xf32>");

	auto weightsId = ctl.addNode(weights);
	auto inputId = ctl.addNode(input);
	auto matmulId = ctl.addNode(matmul);
	auto sigmoidId = ctl.addNode(sigmoid);
	auto outputId = ctl.addNode(output);
	// Add edges between the nodes
	DomainFlowEdge df1(0, true, "tensor<4x256x16xf32>", 32);
	ctl.addEdge(weightsId, 0, matmulId, 1, df1);
	DomainFlowEdge df0(0, true, "tensor<4x256>", 32);
	ctl.addEdge(inputId, 0, matmulId, 0, df0);
	DomainFlowEdge df2(0, false, "tensor<4x256x16>", 32);
	ctl.addEdge(matmulId, 0, sigmoidId, 0, df2);
	DomainFlowEdge df3(0, false, "tensor<16>", 32);
	ctl.addEdge(sigmoidId, 0, outputId, 0, df3);

	// assign sources and sinks
	ctl.addSource(weightsId);
	ctl.addSource(inputId);
	ctl.addSink(outputId);

	// generate the graph order
	ctl.assignNodeDepths();

	// report on the operator statistics
	reportOperatorStats(ctl);

	// Save the graph to a file
	std::string dfgFilename = graphName + ".dfg";
	dfgFilename = generateDataOutputFile(std::string("workloads/ctl/") + dfgFilename);  // stick it in the data directory
	ctl.graph.save(dfgFilename);

	std::cout << "Saved graph to: " << dfgFilename << std::endl;

    return EXIT_SUCCESS;
}
