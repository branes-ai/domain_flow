#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

	std::string graphName = "AtimesBtimesC_square";
	DomainFlowGraph nla(graphName); // Numerical Linear Algebra

	size_t SLOT_A = 0;
	size_t SLOT_B = 1;
	size_t SLOT_OUT = 0;

	// pipelined matmul D = A * B * C
	// using two input matmuls, which implicitely set the input values to the reduction to zero
	//
	
	// A, B, and C matrices are a constant tensor of 256x256
	auto A = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.A-matrix").addResult(SLOT_OUT, "A-matrix", "tensor<256x256xf32>");
	auto B = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.B-matrix").addResult(SLOT_OUT, "B-matrix", "tensor<256x256xf32>");
	auto C = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.C-matrix").addResult(SLOT_OUT, "C-matrix", "tensor<256x256xf32>");

	// model A * B -> * C -> D
	// first stage: A * B -> AB
	auto AtimesB = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(SLOT_A, "tensor<256x256xf32>").addOperand(SLOT_B, "tensor<256x256xf32>").addResult(SLOT_OUT, "AB", "tensor<256x256xf32>");
	// second stage: AB * C -> D
	auto D = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(SLOT_A, "tensor<256x256xf32>").addOperand(SLOT_B, "tensor<256x256xf32>").addResult(SLOT_OUT, "D", "tensor<256x256xf32>");
	
	// output result
	auto output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "output").addOperand(SLOT_A, "tensor<256x256xf32>").addAttribute("target", "memory");

	// create the nodes
	auto aId = nla.addNode(A);
	auto bId = nla.addNode(B);
	auto cId = nla.addNode(C);
	// compute stages
	auto mmStage1Id = nla.addNode(AtimesB);
	auto mmStage2Id = nla.addNode(D);
	auto outputId = nla.addNode(output);

	// Add edges between the nodes
	DomainFlowEdge A_flow(0, true, "tensor<256x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(aId, SLOT_OUT, mmStage1Id, SLOT_A, A_flow);
	DomainFlowEdge B_flow(0, true, "tensor<256x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(bId, SLOT_OUT, mmStage1Id, SLOT_B, B_flow);
	DomainFlowEdge AB_flow(0, false, "tensor<256x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(mmStage1Id, SLOT_OUT, mmStage2Id, SLOT_A, AB_flow);
	DomainFlowEdge C_flow(0, true, "tensor<256x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(cId, SLOT_OUT, mmStage2Id, SLOT_B, C_flow);
	DomainFlowEdge D_flow(0, false, "tensor<256x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(mmStage2Id, SLOT_OUT, outputId, SLOT_A, D_flow);

	// generate the graph order
	nla.assignNodeDepths();

	// assign sources and sinks
	nla.addSource(aId);
	nla.addSource(bId);
	nla.addSource(cId);
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
