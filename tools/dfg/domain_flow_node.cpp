#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>


void testNodeSerialization(sw::dfa::DomainFlowOperator op) {
	using namespace sw::dfa;

	auto nodeOut = DomainFlowNode(op, "test").addOperand("tensor<2x2xf32>").addOperand("tensor<2x2xf32>").addResult("result_0", "tensor<2x2xf32>");
	nodeOut.setDepth(1);
	std::cout << "NODE: " << nodeOut << '\n';
	std::stringstream ss;
	ss << nodeOut;
	DomainFlowNode nodeIn;
	ss >> nodeIn;
	std::cout << "NODE: " << nodeIn << '\n';
}

int main() {
    using namespace sw::dfa;

	// enumerate all the Domain Flow operators in our database
	bool bSuccess = true;
	for (auto op : AllDomainFlowOperators()) {
		std::cout << "Processing operator: " << op << std::endl;

		auto nodeOut = DomainFlowNode(op, "test").addOperand("tensor<2x2xf32>").addOperand("tensor<2x2xf32>").addResult("result_0", "tensor<2x2xf32>");
		nodeOut.setDepth(1);
		std::stringstream ss;
		ss << nodeOut;
		DomainFlowNode nodeIn;
		ss >> nodeIn;

		if (nodeOut != nodeIn) {
			std::cerr << "Error: Node serialization failure:\n";
			std::cerr << "  Original     : " << nodeOut << '\n';
			std::cerr << "  Deserialized : " << nodeIn << '\n';
			bSuccess = false;
		}
		
	}

	if (bSuccess) {
		std::cout << "All operators processed successfully.\n";
	}
	else {
		std::cerr << "Some operators failed to process correctly.\n";
	}

    return (bSuccess ? EXIT_SUCCESS : EXIT_FAILURE);
}
