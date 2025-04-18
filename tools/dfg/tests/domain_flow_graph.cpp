#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;

    DomainFlowGraph dfg("my-test-graph");
	DomainFlowNode cst1 = DomainFlowNode(DomainFlowOperator::CONSTANT, "test.Constant")
		.addResult(0, "cst1", "tensor<1x4xf32>");
    DomainFlowNode cst2 = DomainFlowNode(DomainFlowOperator::CONSTANT, "test.Constant")
        .addResult(0, "cst2", "tensor<1x4xf32>");

	DomainFlowNode nodeAdd = DomainFlowNode(DomainFlowOperator::ADD, "test.Add")
        .addOperand(0, "tensor<1x4xf32>")
        .addOperand(1, "tensor<1x4xf32>")
        .addResult(0, "out", "tensor<1x4xf32>");

    DomainFlowNode cst3 = DomainFlowNode(DomainFlowOperator::CONSTANT, "test.Constant")
        .addResult(0, "cst3", "tensor<1x4xf32>");
	DomainFlowNode nodeMul = DomainFlowNode(DomainFlowOperator::MUL, "test.Mul")
        .addOperand(0, "tensor<1x4xf32>")
        .addOperand(1, "tensor<1x4xf32>")
        .addResult(0, "out", "tensor<1x4xf32>");

	// matmul takes an input tensor, a weights matrix
	DomainFlowNode weights = DomainFlowNode(DomainFlowOperator::CONSTANT, "test.Weights")
		.addResult(0, "weights", "tensor<1x4x4xf32>");
	DomainFlowNode nodeMatmul = DomainFlowNode(DomainFlowOperator::MATMUL, "test.Matmul")
        .addOperand(0, "tensor<1x4xf32>")
        .addOperand(1, "tensor<1x4x4xf32>")
        .addResult(0, "out", "tensor<1x4x4xf32>");

	// Conv2D takes an input tensor, a weights matrix, and a bias
	DomainFlowNode image = DomainFlowNode(DomainFlowOperator::CONSTANT, "test.Image")
		.addResult(0, "image", "tensor<1x224x224x3xf32>");
	DomainFlowNode kernel = DomainFlowNode(DomainFlowOperator::CONSTANT, "test.Kernel")
		.addResult(0, "kernel", "tensor<32x3x3x3xf32>");
	DomainFlowNode bias = DomainFlowNode(DomainFlowOperator::CONSTANT, "test.Bias")
		.addResult(0, "bias", "tensor<32xf32>");
    DomainFlowNode nodeConv2d = DomainFlowNode(DomainFlowOperator::CONV2D, "test.Conv2D")
        .addOperand(0, "tensor<1x224x224x3xf32>")
        .addOperand(1, "tensor<32x3x3x3xf32>")
        .addOperand(2, "tensor<32xf32>")
        .addResult(0, "out", "tensor<1x112x112x32xf32>");

	// output result
	DomainFlowNode output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "test.Output")
		.addOperand(0, "tensor<1x112x112x32xf32>")
		.addAttribute("target", "memory");


	// Add nodes to the graph
    auto cst1Id = dfg.addNode(cst1);
	auto cst2Id = dfg.addNode(cst2);
    auto addId = dfg.addNode(nodeAdd);
    auto cst3Id = dfg.addNode(cst3);
	auto mulId = dfg.addNode(nodeMul);
	auto weightsId = dfg.addNode(weights);
    auto mmId  = dfg.addNode(nodeMatmul);
	auto imageId = dfg.addNode(image);
	auto kernelId = dfg.addNode(kernel);
	auto biasId = dfg.addNode(bias);
	auto conId = dfg.addNode(nodeConv2d);
	auto outputId = dfg.addNode(output);

    // Add edges between the nodes
    DomainFlowEdge df0(0, true, "tensor<1x4xf32>", 32);
	dfg.addEdge(cst1Id, 0, addId, 0, df0);
	DomainFlowEdge df1(0, true, "tensor<1x4xf32>", 32);
	dfg.addEdge(cst2Id, 0, addId, 1, df1);
	DomainFlowEdge df2(0, false, "tensor<1x4xf32>", 32);
	dfg.addEdge(addId, 0, mulId, 0, df2);
	DomainFlowEdge df3(0, true, "tensor<1x4xf32>", 32);
	dfg.addEdge(cst3Id, 0, mulId, 1, df3);
	DomainFlowEdge df4(0, true, "tensor<1x4x4xf32>", 32); // weights matrix
	dfg.addEdge(weightsId, 0, mmId, 0, df4);
	DomainFlowEdge df5(0, false, "tensor<1x4xf32>", 32);
	dfg.addEdge(mulId, 0, mmId, 1, df5);

	DomainFlowEdge df6(0, true, "tensor<1x224x224x3xf32>", 32); // image
	dfg.addEdge(imageId, 0, conId, 0, df6);
	DomainFlowEdge df7(0, true, "tensor<32x3x3x3xf32>", 32); // kernel
	dfg.addEdge(kernelId, 0, conId, 1, df7);
	DomainFlowEdge df8(0, true, "tensor<32xf32>", 32); // bias
	dfg.addEdge(biasId, 0, conId, 2, df8);

	DomainFlowEdge df9(0, false, "tensor<1x112x112x32xf32>", 32);
	dfg.addEdge(conId, 0, outputId, 0, df9);

	// Add source and sink information to the graph
	dfg.addSource(cst1Id);
	dfg.addSource(cst2Id);
	dfg.addSource(cst3Id);
	dfg.addSource(weightsId);
	dfg.addSource(imageId);
	dfg.addSource(kernelId);
	dfg.addSource(biasId);
	dfg.addSink(outputId);

    dfg.save(std::cout);

    dfg.save("test.dfg");
    DomainFlowGraph dfg2("serialized graph");
    dfg2.load("test.dfg");
    std::cout << dfg2 << std::endl;

	if (dfg == dfg2) {
		std::cerr << "SUCCESS: Graphs are equal" << std::endl;
		return EXIT_SUCCESS;
	}

	std::cerr << "Error: Graphs are not equal" << std::endl;
	return EXIT_FAILURE;

}
