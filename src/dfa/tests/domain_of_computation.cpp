#include <iostream>
#include <iomanip>
#include <dfa/dfa.hpp>

template<typename ConstraintCoefficientType>
bool isInsideHull(const sw::dfa::ConstraintSet<ConstraintCoefficientType>& constraints, const sw::dfa::IndexPoint& point) {
	bool all_satisfied = true;
	for (const auto& constraint : constraints.get_constraints()) {
		if (!constraint.is_satisfied(point)) {
			all_satisfied = false;
			std::cout << "Constraint: " << constraints << " is NOT satisfied by IndexPoint p = " << point << '\n';
		}
	}
	return all_satisfied;
}

template<typename ConstraintCoefficientType>
bool validate(const sw::dfa::ConstraintSet<ConstraintCoefficientType>& constraints, const sw::dfa::IndexPoint& point) {
	bool isSatisfied = constraints.isInside(point);
	if (isSatisfied) {
		std::cout << "Success: IndexPoint p = " << point << " is contained in Convex Hull defined by:\n" << constraints << '\n';
	}
	else {
		std::cout << "Failure: IndexPoint p = " << point << " is NOT contained in Convex Hull defined by:\n" << constraints << '\n';
		// troubleshoot the failure
		isInsideHull(constraints, point);
	}
	return isSatisfied;
}

int main() {
    using namespace sw::dfa;
    using IndexPointType = int;
    using ConstraintCoefficientType = int;

    std::map<std::size_t, std::string> inputs, outputs;
    inputs[0] = "tensor<4x8xf32>";
    inputs[1] = "tensor<8x4xf32>";
    outputs[0] = "tensor<4x4xf32>";
    DomainOfComputation DoC(DomainFlowOperator::MATMUL, inputs, outputs);

	std::cout << DoC << '\n';

    /*
    IndexPoint p = { 2, 2, 2 };
    bool isSatisfied = validate(constraints, p); 
    if (!isSatisfied) {
	    return EXIT_FAILURE;
    }
    */

    return EXIT_SUCCESS;
}
