#include <iostream>
#include <iomanip>
#include <dfa/dfa.hpp>

template<typename ConstraintCoefficientType>
bool isInsideHull(const sw::dfa::ConstraintSet<ConstraintCoefficientType>& constraints, const sw::dfa::IndexPoint& point) {
	bool all_satisfied = true;
	for (const auto& constraint : constraints.getConstraints()) {
		if (!constraint.isSatisfied(point)) {
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

    ConstraintSet<ConstraintCoefficientType> constraints = {
		 {{1, 0, 0}, 0, ConstraintType::GreaterOrEqual}  // x >= 0
		,{{0, 1, 0}, 0, ConstraintType::GreaterOrEqual}  // y >= 0
		,{{0, 0, 1}, 0, ConstraintType::GreaterOrEqual}  // z >= 0
		,{{1, 0, 0}, 5, ConstraintType::LessOrEqual}     // x <= 5
		,{{0, 1, 0}, 5, ConstraintType::LessOrEqual}     // y <= 5
		,{{0, 0, 1}, 5, ConstraintType::LessOrEqual}     // z <= 5
		,{{0, 0, 1}, 6, ConstraintType::LessThan}        // z < 6
		,{{1, 1, 1}, 6, ConstraintType::LessOrEqual}     // x + y + z <= 6
    };

    // satisfy constraint tests
    IndexPoint p = { 2, 2, 2 };
	bool isSatisfied = validate(constraints, p); 
	if (!isSatisfied) {
		return EXIT_FAILURE;
	}

    return EXIT_SUCCESS;
}
