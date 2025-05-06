#include <iostream>
#include <iomanip>
#include <string>
#include <dfa/dfa.hpp>

namespace sw {
    namespace dfa {

        inline DomainFlowGraph CreateMatmulDFG(const std::string& name,
		const std::string& tensorSpecA,
            	const std::string& tensorSpecB,
		const std::string& tensorSpecCin,  // if Cin is empty, it is a constant 0
		const std::string& tensorSpecCout) {

			const size_t SLOT_A = 0;
			const size_t SLOT_B = 1;
			const size_t SLOT_C = 2;
			const size_t SLOT_O = 0;

			const size_t sizeInBitsAElement = 32; // assuming 32-bit floating point: needs to be derived from tensorSpecA
			const size_t sizeInBitsBElement = 32; // assuming 32-bit floating point: needs to be derived from tensorSpecB
			const size_t sizeInBitsCinElement = 32; // assuming 32-bit floating point: needs to be derived from tensorSpecCin
			const size_t sizeInBitsCoutElement = 32; // assuming 32-bit floating point: needs to be derived from tensorSpecCout

			// Create a DomainFlowGraph object
			DomainFlowGraph dfg(name);

			bool twoInputMatmul = tensorSpecCin.empty();
			std::string tensorSpecC = tensorSpecCout;

			// Create nodes
			auto constA = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.A").addResult(SLOT_O, "A", tensorSpecA);
			auto constB = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.B").addResult(SLOT_O, "B", tensorSpecB);

			auto constCin = DomainFlowNode(DomainFlowOperator::CONSTANT, "const.Cin").addResult(SLOT_O, "Cin", tensorSpecC);
			auto matmul = DomainFlowNode(DomainFlowOperator::MATMUL, (twoInputMatmul ? "2-input MATMUL" : "3-input MATMUL"));
			auto Cout = DomainFlowNode(DomainFlowOperator::FUNCTION_RETURN, "Cout").addOperand(0, tensorSpecCout).addAttribute("target", "memory").addResult(0, "Cout", tensorSpecCout);

			auto aId = dfg.addNode(constA);
			auto bId = dfg.addNode(constB);
			auto cinId = dfg.addNode(constCin);
			if (twoInputMatmul) {
				matmul
					.addOperand(SLOT_A, tensorSpecA)
					.addOperand(SLOT_B, tensorSpecB)
					.addResult(SLOT_O, "Cout", tensorSpecCout);
			}
			else {
				matmul
					.addOperand(SLOT_A, tensorSpecA)
					.addOperand(SLOT_B, tensorSpecB)
					.addOperand(SLOT_C, tensorSpecCin)
					.addResult(SLOT_O, "Cout", tensorSpecCout);
			}
			auto matmulId = dfg.addNode(matmul);
			auto coutId = dfg.addNode(Cout);

			// Add edges between the nodes
			DomainFlowEdge A_flow(0, true, tensorSpecA, sizeInBitsAElement, 0, 0, { 1, 1, 1 });
			dfg.addEdge(aId, SLOT_O, matmulId, SLOT_A, A_flow);
			DomainFlowEdge B_flow(0, true, tensorSpecB, sizeInBitsBElement, 0, 0, { 1, 1, 1 });
			dfg.addEdge(bId, SLOT_O, matmulId, SLOT_B, B_flow);
			if (!twoInputMatmul) {
				DomainFlowEdge C_flow(0, false, tensorSpecCin, sizeInBitsCinElement, 0, 0, { 1, 1, 1 });
				dfg.addEdge(cinId, SLOT_O, matmulId, SLOT_C, C_flow);
			}
			DomainFlowEdge Cout_flow(0, false, tensorSpecCout, sizeInBitsCoutElement, 0, 0, { 1, 1, 1 });
			dfg.addEdge(matmulId, SLOT_O, coutId, 0, Cout_flow);

			// assign sources and sinks
			dfg.addSource(aId);
			dfg.addSource(bId);
			if (!twoInputMatmul) {
				dfg.addSource(cinId);
			}
			dfg.addSink(coutId);

			// assign the graph order
			dfg.assignNodeDepths();

			return dfg;
        }

    }
}
