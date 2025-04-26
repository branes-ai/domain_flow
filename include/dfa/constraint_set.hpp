#pragma once
#include <stdexcept>
#include <dfa/tensor_spec_parser.hpp>

namespace sw {
    namespace dfa {

		// forward declarations
		template<typename Scalar> struct Hyperplane;
		struct TensorTypeInfo;

		template<typename ConstraintCoefficientType = int>
		class ConstraintSet {
			using Constraint = Hyperplane<ConstraintCoefficientType>;
		public:
			// default constructor
			ConstraintSet() = default;
			// constructor with initializer list
			ConstraintSet(std::initializer_list<Constraint> init_constraints)
				: constraints(init_constraints) {}

			void add(const Constraint& c) noexcept { constraints.push_back(c); }

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

			bool empty() const noexcept { return constraints.empty(); }
			void clear() noexcept { constraints.clear(); }
			size_t size() const noexcept { return constraints.size(); }
			const std::vector<Constraint>& get_constraints() const noexcept { return constraints; }
			const Constraint& operator[](size_t index) const {
				if (index >= constraints.size()) {
					throw std::out_of_range("Index out of range");
				}
				return constraints[index];
			}
		private:
			std::vector<Hyperplane<ConstraintCoefficientType>> constraints;
		};

		inline std::ostream& operator<<(std::ostream& os, const ConstraintSet<int>& cs) {
			os << "ConstraintSet:\n";
			for (const auto& c : cs.get_constraints()) {
				os << "  " << c << '\n';
			}
			return os;
		}
    }
}
