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
    auto input = DomainFlowNode(DomainFlowOperator::FUNCTION_ARGUMENT, "inputImage").addOperand("tensor<4x256x256x1xi8");
	// linear takes an input tensor, a weights matrix, and a bias vector
	// batch of 4, 256x256 gray scale images (==1 channel) with a bias vector to 16 categories
	auto linear = DomainFlowNode(DomainFlowOperator::LINEAR, "linear").addOperand("tensor<4x256x256x1xi8").addOperand("tensor<4x65536x1xf16>").addOperand("tensor<16xf16>").addResult("out", "tensor<tensor<16xf16>");
	// relu takes the output of the linear layer and applies the ReLU activation function
	auto relu = DomainFlowNode(DomainFlowOperator::RELU, "relu").addOperand("tensor<16xf16>").addResult("out", "tensor<16xf16>");
	// output result
	auto output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "output").addOperand("tensor<16xf16>");
    
    auto inputId = mlp.addNode(input);
	auto linearId = mlp.addNode(linear);
    auto reluId = mlp.addNode(relu);
	auto outputId = mlp.addNode(output);
	// Add edges between the nodes
	DomainFlowEdge df1(0, true, "tensor<4x256x256x1>", 8, { 1, 1, 1 });
	mlp.addEdge(inputId, linearId, df1);
	DomainFlowEdge df2(0, false, "tensor<4x65536x1>", 16, { 1, 1, 1 });
	mlp.addEdge(linearId, reluId, df2);
	DomainFlowEdge df3(0, false, "tensor<16>", 16, { 1, 1, 1 });
	mlp.addEdge(reluId, outputId, df3);

	// generate the graph order
	mlp.assignNodeDepths();

	// report on the operator statistics
	reportOperatorStats(mlp);

	// Save the graph to a file
	std::string dfgFilename = graphName + ".dfg";
	dfgFilename = generateDataOutputFile(std::string("workloads/dnn/") + dfgFilename);  // stick it in the data directory
	mlp.graph.save(dfgFilename);

	std::cout << "Saved graph to: " << dfgFilename << std::endl;

    return EXIT_SUCCESS;
}
