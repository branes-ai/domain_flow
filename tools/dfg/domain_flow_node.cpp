#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;

	{
		auto op = DomainFlowOperator::MATMUL;
		std::cout << "Operator: " << op << '\n';
		std::stringstream ss;
		ss << op;
		DomainFlowOperator op2;
		ss >> op2;
		std::cout << "Operator: " << op2 << '\n';
	}

	{
		// Test the DomainFlowNode class
		DomainFlowNode nodeOut(DomainFlowOperator::MATMUL, "test.matmul");
		nodeOut.addOperand("tensor<2x2xf32>")
			.addOperand("tensor<2x2xf32>")
			.addResult("result_0", "tensor<2x2xf32>")
			.setDepth(1);
		std::cout << "NODE: " << nodeOut << '\n';
		std::stringstream ss;
		ss << nodeOut;
		DomainFlowNode nodeIn;
		ss >> nodeIn;
		std::cout << "NODE: " << nodeIn << '\n';
	}

    return EXIT_SUCCESS;
}
