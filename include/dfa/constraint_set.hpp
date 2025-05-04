#pragma once
#include <stdexcept>
#include <dfa/tensor_spec_parser.hpp>

namespace sw {
    namespace dfa {

		// forward declarations
		template<typename Scalar> struct Hyperplane;
		struct TensorTypeInfo;
		struct IndexPoint;

		template<typename ConstraintCoefficientType = int>
		class ConstraintSet {
			using Constraint = Hyperplane<ConstraintCoefficientType>;
		public:
			// default constructor
			ConstraintSet() = default;
			// constructor with initializer list
			ConstraintSet(std::initializer_list<Constraint> init_constraints)
				: dimension{}, constraints(init_constraints) {
				if (constraints.empty()) {
					std::cerr << "ConstraintSet constructor: at least one constraint is required\n";
				}
				dimension = constraints[0].normal.size();
				for (const auto& c : constraints) {
					if (c.normal.size() != dimension) {
						std::cerr << "ConstraintSet constructor: all constraints must have the same dimension\n";
						dimension = 0;
						constraints.clear();
					}
				}
			}

			// modifiers
			void clear() noexcept { dimension = 0;  constraints.clear(); }

			/// <summary>
			/// Add a constraint to the set. The dimension of the constraint must match the current dimension of the set.
			/// </summary>
			/// <param name="c"></param>
			void add(const Constraint& c) noexcept {
				if (constraints.empty()) {
					dimension = c.normal.size();
				}
				else if (c.normal.size() != dimension) {
					std::cerr << "ConstraintSet add: all constraints must have the same dimension\n";
					return;
				}
				constraints.push_back(c); 
			}

			/// <summary>
			/// given a tensor type info, extract the shape values and create constraints to describe the domain.
			/// This can be used for element-wise tensor operators like Add, Subtract, Multiply, etc.
			/// </summary>
			/// <param name="tensorInfo"></param>
			void shapeExtract(const TensorTypeInfo& tensorInfo) {
				// shape is a vector of dimensions
				// we need to create two constraints for each dimension
				// one for the lower bound and one for the upper bound

				// 1D: tensor<256xf32>
				// 2D: tensor<256x256xf32>
				// 3D: tensor<256x256x256xf32>
				// 4D: tensor<256x256x256x256xf32>
				// lower bound constraints
				switch (tensorInfo.size()) {
				case 1:
					add(Constraint({ 1 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 1 }, tensorInfo.shape[0], ConstraintType::LessThan));
					break;
				case 2:
					add(Constraint({ 1, 0 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 1, 0 }, tensorInfo.shape[0], ConstraintType::LessThan));
					add(Constraint({ 0, 1 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 0, 1 }, tensorInfo.shape[1], ConstraintType::LessThan));
					break;
				case 3:
					add(Constraint({ 1, 0, 0 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 1, 0, 0 }, tensorInfo.shape[0], ConstraintType::LessThan));
					add(Constraint({ 0, 1, 0 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 0, 1, 0 }, tensorInfo.shape[1], ConstraintType::LessThan));
					add(Constraint({ 0, 0, 1 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 0, 0, 1 }, tensorInfo.shape[2], ConstraintType::LessThan));
					break;
				case 4:
					// the first dimension is typically the batch dimension

					// but we are going into 4D space so that we have all the options
					// available to us to slice and dice for the right visualization
					add(Constraint({ 1, 0, 0, 0 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 1, 0, 0, 0 }, tensorInfo.shape[0], ConstraintType::LessThan));
					add(Constraint({ 0, 1, 0, 0 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 0, 1, 0, 0 }, tensorInfo.shape[1], ConstraintType::LessThan));
					add(Constraint({ 0, 0, 1, 0 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 0, 0, 1, 0 }, tensorInfo.shape[2], ConstraintType::LessThan));
					add(Constraint({ 0, 0, 0, 1 }, 0, ConstraintType::GreaterOrEqual));
					add(Constraint({ 0, 0, 0, 1 }, tensorInfo.shape[3], ConstraintType::LessThan));
					break;
				default:
					std::cerr << "error: unsupported tensor shape size: " << tensorInfo.size() << std::endl;
					break;
				}
			}

			// selectors
			bool empty() const noexcept { return constraints.empty(); }
			bool isInside(const IndexPoint& point) const noexcept {
				for (const auto& constraint : constraints) {
					if (!constraint.is_satisfied(point)) {
						return false;
					}
				}
				return true;
			}
			size_t size() const noexcept { return constraints.size(); }
			const std::vector<Constraint>& get_constraints() const noexcept { return constraints; }
			const Constraint& operator[](size_t index) const {
				if (index >= constraints.size()) {
					throw std::out_of_range("Index out of range");
				}
				return constraints[index];
			}
			Constraint& operator[](size_t index) {
				if (index >= constraints.size()) {
					throw std::out_of_range("Index out of range");
				}
				return constraints[index];
			}
		private:
			int dimension;
			std::vector<Hyperplane<ConstraintCoefficientType>> constraints;
		};

		template<typename CCType>
		inline std::ostream& operator<<(std::ostream& os, const ConstraintSet<CCType>& cs) {
			os << "ConstraintSet:\n";
			for (const auto& c : cs.get_constraints()) {
				os << "  " << c << '\n';
			}
			return os;
		}
    }
}
