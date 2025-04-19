#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

	std::string graphName = "oneLayerMLP";
    DomainFlowGraph mlp(graphName); // Deep Learning graph

	// model a single layer Multi Level Perceptron using a Linear operator, 
	// which consists of an input, a weights matrix, and a bias
	auto weights = DomainFlowNode(DomainFlowOperator::CONSTANT, "constant.weights").addOperand(0, "tensor<4x256x16xf32>");
	auto input = DomainFlowNode(DomainFlowOperator::FUNCTION_ARGUMENT, "inputVector").addOperand(0, "tensor<4x256xf32>");
	// matmul takes an input tensor, a weights matrix
	// batch of 4, 256 element vectors input, with a 256x16 to 16 categories
	auto matmul = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(0, "tensor<4x256xf32>").addOperand(1, "tensor<4x256x16xf32>").addResult(0, "out", "tensor<tensor<4x16xf32>");
	// sigmoid takes the output of the linear layer and applies the Sigmoid activation function
	auto sigmoid = DomainFlowNode(DomainFlowOperator::SIGMOID, "sigmoid").addOperand(0, "tensor<16xf32>").addResult(0, "out", "tensor<16xf32>");
	// output result
	auto output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "output").addOperand(0, "tensor<16xf32>");

	auto weightsId = mlp.addNode(weights);
	auto inputId = mlp.addNode(input);
	auto matmulId = mlp.addNode(matmul);
	auto sigmoidId = mlp.addNode(sigmoid);
	auto outputId = mlp.addNode(output);
	// Add edges between the nodes
	DomainFlowEdge df1(0, true, "tensor<4x256x16xf32>", 32);
	mlp.addEdge(weightsId, 0, matmulId, 1, df1);
	DomainFlowEdge df0(0, true, "tensor<4x256>", 32);
	mlp.addEdge(inputId, 0, matmulId, 0, df0);
	DomainFlowEdge df2(0, false, "tensor<4x256x16>", 32);
	mlp.addEdge(matmulId, 0, sigmoidId, 0, df2);
	DomainFlowEdge df3(0, false, "tensor<16>", 32);
	mlp.addEdge(sigmoidId, 0, outputId, 0, df3);

	// assign sources and sinks
	mlp.addSource(weightsId);
	mlp.addSource(inputId);
	mlp.addSink(outputId);

	// generate the graph order
	mlp.assignNodeDepths();

	// report on the operator statistics
	reportOperatorStats(mlp);
	reportArithmeticComplexity(mlp);

	// Save the graph to a file
	std::string dfgFilename = graphName + ".dfg";
	dfgFilename = generateDataOutputFile(std::string("workloads/dnn/") + dfgFilename);  // stick it in the data directory
	mlp.save(dfgFilename);

	std::cout << "Saved graph to: " << dfgFilename << std::endl;

    return EXIT_SUCCESS;
}
