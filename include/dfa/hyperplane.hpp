#pragma once
#include <stdexcept>

namespace sw {
    namespace dfa {

        // Enum to represent the constraint type
        enum class ConstraintType {
            LessThan,
            LessOrEqual,
            Equal,
            GreaterOrEqual,
            GreaterThan
        };


        /// <summary>
        /// Structure to represent a hyperplane constraint: a_1 * x_1 + a_2 * x_2 + ... = b
        /// </summary>
        /// <typeparam name="Scalar"></typeparam>
        template<typename Scalar = int>
        struct Hyperplane {
            std::vector<Scalar> normal; // Normal vector to the hyperplane
            Scalar rhs;                 // right hand side of the hyperplane constraint
            ConstraintType constraint;

//			Hyperplane() : normal{1,1,1}, rhs(0), constraint(ConstraintType::Equal) {}
            Hyperplane(std::initializer_list<Scalar> n, Scalar o, ConstraintType c) : normal(n), rhs(o), constraint(c) {}

            bool is_satisfied(const std::vector<Scalar>& point) const {
                double dot_product = 0;
                for (size_t i = 0; i < normal.size(); ++i) {
                    dot_product += normal[i] * point[i];
                }

                bool satisfied = false;
                switch (constraint) {
                case ConstraintType::LessThan:
                    satisfied = dot_product < rhs;
                    break;
                case ConstraintType::LessOrEqual:
                    satisfied = dot_product <= rhs;
                    break;
                case ConstraintType::Equal:
                    satisfied = dot_product == rhs;
                    break;
                case ConstraintType::GreaterOrEqual:
                    satisfied = dot_product >= rhs;
                    break;
                case ConstraintType::GreaterThan:
                    satisfied = dot_product > rhs;
                    break;
                default:
                    satisfied = false;
                    break;
                }
                return satisfied;
            }
        };
 
		template<typename ConstraintCoefficientType>
		inline std::ostream& operator<<(std::ostream& os, const Hyperplane<ConstraintCoefficientType>& hyperplane) {
            os << " { { ";
			size_t last = hyperplane.normal.size() - 1;
			for (size_t i = 0; i < last; ++i) {
				os << hyperplane.normal[i] << ", ";
			}
            os << hyperplane.normal[last] << " } ";
            std::string inequality{};
            switch (hyperplane.constraint) {
            case ConstraintType::LessThan:
                inequality = " <  ";
                break;
            case ConstraintType::LessOrEqual:
                inequality = " <= ";
                break;
            case ConstraintType::Equal:
                inequality = " == ";
                break;
            case ConstraintType::GreaterOrEqual:
                inequality = " >= ";
                break;
            case ConstraintType::GreaterThan:
                inequality = " >  ";
                break;
            default:
                inequality = "  ? ";
                break;
            }
            os << inequality << hyperplane.rhs << "}";
			return os;
		}
    }
}