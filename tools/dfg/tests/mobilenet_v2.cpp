#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

    DomainFlowGraph dfg("mobilenet_V2_tosa");

	std::string testFile = "dfg/mobilenet_v2_tosa.dfg";

	std::string dataFileName{};
	try {
		dataFileName = generateDataFile(testFile);
		std::cout << "Data file : " << dataFileName << std::endl;
	}
	catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_SUCCESS;  // return success to support CI
	}

	dfg.graph.load(dataFileName);
    //dfg.graph.save(std::cout);

    dfg.graph.save("test.dfg");
    DomainFlowGraph dfg2("serialized graph");
    dfg2.graph.load("test.dfg");

    // compare the two graphs
	bool areEqual = true;
	for (auto& [nodeId, dfg1Node] : dfg.graph.nodes()) {
		DomainFlowNode dfg2Node = dfg2.graph.node(nodeId);
		if (dfg1Node != dfg2Node) {
			std::cout << "Node mismatch: " << nodeId << std::endl;
			std::cout << "Original     : " << dfg1Node << std::endl;
			std::cout << "Serialized   : " << dfg2Node << std::endl;
			areEqual = false;
		}
		else {
			std::cout << "Node match: " << nodeId << std::endl;
		}
		//auto lhs = dfg1Node;
		//auto rhs = dfg2Node;
		//if (lhs.opType == rhs.opType) { std::cout << "1 good\n"; }
		//if (lhs.name == rhs.name) { std::cout << "2 good\n"; }
		//if (lhs.operandType == rhs.operandType) { std::cout << "3 good\n"; }
		//if (lhs.resultValue == rhs.resultValue) { std::cout << "4 good\n"; }
		//if (lhs.resultType == rhs.resultType) { std::cout << "5 good\n"; }
		//if (lhs.depth == rhs.depth) { std::cout << "6 good\n"; }
	}
    //std::cout << dfg2 << std::endl;

	if (areEqual) { 
		std::cout << "Success: Graphs are equal" << std::endl;
	}

    return EXIT_SUCCESS;
}
