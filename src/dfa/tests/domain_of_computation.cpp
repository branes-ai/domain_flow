#include <iostream>
#include <iomanip>
#include <dfa/dfa.hpp>

template<typename ConstraintCoefficientType>
bool isInsideHull(const sw::dfa::ConstraintSet<ConstraintCoefficientType>& constraints, const sw::dfa::IndexPoint& point) {
	bool all_satisfied = true;
	for (const auto& constraint : constraints.get_constraints()) {
		if (!constraint.is_satisfied(point)) {
			all_satisfied = false;
			std::cout << "IndexPoint p = " << point << " failed constraint : " << constraint << '\n';
		}
	}
	return all_satisfied;
}

int main() {
    using namespace sw::dfa;
    using IndexPointType = int;
    using ConstraintCoefficientType = int;

	std::cout << "test 1: set up a Domain of Computation for a matmul operator\n";
    std::map<std::size_t, std::string> inputs, outputs;
    inputs[0] = "tensor<4x8xf32>";
    inputs[1] = "tensor<8x4xf32>";
    outputs[0] = "tensor<4x4xf32>";
    DomainOfComputation DoC(DomainFlowOperator::MATMUL, inputs, outputs);
	std::cout << DoC << '\n';

	std::cout << "test 2: set up a DomainFlowNode as a matmul operator with operands and results\n";
	DomainFlowNode node(DomainFlowOperator::MATMUL, "matmul_doc_test");
	node.addOperand(0, "tensor<4x8xf32>");
	node.addOperand(1, "tensor<8x4xf32>");
	node.addResult(0, "value_string_of_tensor_values", "tensor<4x4xf32>");
	std::cout << node << '\n';
	std::cout << "Constraints :\n" << node.getConstraints() << '\n';

	std::cout << "test 3: setup the ConvexHull and IndexSpace\n";
	node.instantiateDomain();
	node.instantiateIndexSpace();

	std::cout << "Test 4: interact with the index space by testing an index point is inside the domain of computation\n";
	const int NR_TESTS = 2; // one inside, one outside of the DoC
    IndexPoint p = { 2, 2, 7 };
	for (int i = 0; i < NR_TESTS; ++i) {
		bool isSatisfied = node.isInside(p);
		if (isSatisfied) {
			std::cout << "Success: IndexPoint p = " << p << " is contained in the Domain of Computation\n";
		}
		else {
			auto constraints = node.getConstraints();
			std::cout << "Failure: IndexPoint p = " << p << " is NOT contained in Domain of Computation defined by:\n" << constraints << '\n';
			// troubleshoot the failure
			isInsideHull(constraints, p);
		}
		p[2]++;
	}

    return EXIT_SUCCESS;
}
