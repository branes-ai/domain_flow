#pragma once
#include <stdexcept>

// Enum to represent the constraint type
enum class ConstraintType {
    LessThan,
    LessOrEqual,
    Equal,
    GreaterOrEqual,
    GreaterThan
};

// Structure to represent a hyperplane constraint
template<typename Scalar = int>
struct Hyperplane {
    std::vector<Scalar> normal; // Normal vector to the hyperplane
    Scalar offset;              // Offset of the hyperplane
    ConstraintType constraint;

    Hyperplane(std::initializer_list<Scalar> n, Scalar o, ConstraintType c) : normal(n), offset(o), constraint(c) {}

    bool is_satisfied(const std::vector<Scalar>& point) const {
        double dot_product = 0;
        for (size_t i = 0; i < normal.size(); ++i) {
            dot_product += normal[i] * point[i];
        }

		bool satisfied = false;
        switch (constraint) {
	    case ConstraintType::LessThan:
		    satisfied = dot_product < offset;
            break;
        case ConstraintType::LessOrEqual:
            satisfied = dot_product <= offset;
            break;
	    case ConstraintType::Equal:
            satisfied = dot_product == offset;
            break;
        case ConstraintType::GreaterOrEqual:
            satisfied = dot_product >= offset;
            break;
	    case ConstraintType::GreaterThan:
            satisfied = dot_product > offset;
            break;
        default:
            satisfied = false;
            break;
        }
        return satisfied;
    }
};