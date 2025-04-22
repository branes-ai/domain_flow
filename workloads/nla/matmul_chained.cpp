#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

	std::string graphName = "matmul_chained";
	DomainFlowGraph nla(graphName); // Numerical Linear Algebra

	size_t SLOT_A = 0;
	size_t SLOT_B = 1;
	size_t SLOT_C = 2;

	// model a chain of matrix mulitplications, the output matrix of each matmul feeds into the C input of the next
	// we are going to set up a set of A and B matrices for each layer in the chain as constants.
	// 
	// 
	// all matrices are 256x256, but at each stage the output is upsampled to the next precision floating-point
	size_t SLOT_OUTPUT = 0;
	auto A1 = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.A1").addResult(SLOT_OUTPUT, "A1", "tensor<256x256xf8>");
	auto B1 = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.B1").addResult(SLOT_OUTPUT, "B1", "tensor<256x256xf8>");
	auto A2 = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.A2").addResult(SLOT_OUTPUT, "A2", "tensor<256x256xf16>");
	auto B2 = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.B2").addResult(SLOT_OUTPUT, "B2", "tensor<256x256xf16>");
	auto A3 = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.A3").addResult(SLOT_OUTPUT, "A3", "tensor<256x256xf32>");
	auto B3 = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.B3").addResult(SLOT_OUTPUT, "B3", "tensor<256x256xf32>");
	auto A4 = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.A4").addResult(SLOT_OUTPUT, "A4", "tensor<256x256xf64>");
	auto B4 = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.B4").addResult(SLOT_OUTPUT, "B4", "tensor<256x256xf64>");


	auto mm1 = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(SLOT_A, "tensor<256x256xf8>").addOperand(SLOT_B, "tensor<256x256xf8>").addResult(0, "C1", "tensor<256x256xf8>");  // 8-bit matmul with implicit C=0
	auto mm2 = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(SLOT_A, "tensor<256x256xf16>").addOperand(SLOT_B, "tensor<256x256xf16>").addOperand(SLOT_C, "tensor<256x256xf16>").addResult(0, "C2", "tensor<256x256xf16>");
	auto mm3 = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(SLOT_A, "tensor<256x256xf32>").addOperand(SLOT_B, "tensor<256x256xf32>").addOperand(SLOT_C, "tensor<256x256xf32>").addResult(0, "C3", "tensor<256x256xf32>");
	auto mm4 = DomainFlowNode(DomainFlowOperator::MATMUL, "matmul").addOperand(SLOT_A, "tensor<256x256xf64>").addOperand(SLOT_B, "tensor<256x256xf64>").addOperand(SLOT_C, "tensor<256x256xf64>").addResult(0, "C4", "tensor<256x256xf64>");

	auto output = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "output").addOperand(0, "tensor<256x256xf64>").addAttribute("target", "memory").addResult(0, "C4", "tensor<256x256xf64>");

	// create the nodes
	auto a1Id = nla.addNode(A1);
	auto b1Id = nla.addNode(B1);
	auto a2Id = nla.addNode(A2);
	auto b2Id = nla.addNode(B2);
	auto a3Id = nla.addNode(A3);
	auto b3Id = nla.addNode(B3);
	auto a4Id = nla.addNode(A4);
	auto b4Id = nla.addNode(B4);
	auto mm1Id = nla.addNode(mm1);
	auto mm2Id = nla.addNode(mm2);
	auto mm3Id = nla.addNode(mm3);
	auto mm4Id = nla.addNode(mm4);
	auto outputId = nla.addNode(output);

	// Add edges between the nodes
	DomainFlowEdge A1_flow(0, true, "tensor<256x256>", 8, 0, 0, { 1, 1, 1 });
	nla.addEdge(a1Id, SLOT_OUTPUT, mm1Id, SLOT_A, A1_flow);
	DomainFlowEdge B1_flow(0, true, "tensor<256x256>", 8, 0, 0, { 1, 1, 1 });
	nla.addEdge(b1Id, SLOT_OUTPUT, mm1Id, SLOT_B, B1_flow);

	// second stage
	DomainFlowEdge C1_flow(0, false, "tensor<256x256>", 8, 0, 0, { 1, 1, 1 });
	nla.addEdge(mm1Id, SLOT_OUTPUT, mm2Id, SLOT_C, C1_flow);  // implicit conversion to f16 inside the matmul

	DomainFlowEdge A2_flow(0, true, "tensor<256x256>", 16, 0, 0, { 1, 1, 1 });
	nla.addEdge(a2Id, SLOT_OUTPUT, mm2Id, SLOT_A, A2_flow);
	DomainFlowEdge B2_flow(0, true, "tensor<256x256>", 16, 0, 0, { 1, 1, 1 });
	nla.addEdge(b2Id, SLOT_OUTPUT, mm2Id, SLOT_B, B2_flow);

	// third stage
	DomainFlowEdge C2_flow(0, false, "tensor<256x256>", 16, 0, 0, { 1, 1, 1 });
	nla.addEdge(mm2Id, SLOT_OUTPUT, mm3Id, SLOT_C, C2_flow);  // implicit conversion to f32 inside the matmul

	DomainFlowEdge A3_flow(0, true, "tensor<256x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(a3Id, SLOT_OUTPUT, mm3Id, SLOT_A, A3_flow);
	DomainFlowEdge B3_flow(0, true, "tensor<256x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(b3Id, SLOT_OUTPUT, mm3Id, SLOT_B, B3_flow);

	// fourth stage
	DomainFlowEdge C3_flow(0, false, "tensor<256x256>", 32, 0, 0, { 1, 1, 1 });
	nla.addEdge(mm3Id, SLOT_OUTPUT, mm4Id, SLOT_C, C3_flow);  // implicit conversion to f64 inside the matmul
	DomainFlowEdge A4_flow(0, true, "tensor<256x256>", 64, 0, 0, { 1, 1, 1 });
	nla.addEdge(a4Id, SLOT_OUTPUT, mm4Id, SLOT_A, A4_flow);
	DomainFlowEdge B4_flow(0, true, "tensor<256x256>", 64, 0, 0, { 1, 1, 1 });
	nla.addEdge(b4Id, SLOT_OUTPUT, mm4Id, SLOT_B, B4_flow);

	// connect the output of the last matmul to the output node
	DomainFlowEdge C4_flow(0, false, "tensor<256x256>", 64, 0, 0, { 1, 1, 1 });
	nla.addEdge(mm4Id, SLOT_OUTPUT, outputId, 0, C4_flow);

	// generate the graph order
	nla.assignNodeDepths();

	// assign sources and sinks
	nla.addSource(a1Id);
	nla.addSource(b1Id);
	nla.addSource(a2Id);
	nla.addSource(b2Id);
	nla.addSource(a3Id);
	nla.addSource(b3Id);
	nla.addSource(a4Id);
	nla.addSource(b4Id);
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
