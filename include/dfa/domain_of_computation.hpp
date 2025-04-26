#pragma once
#include <stdexcept>
#include <dfa/tensor_spec_parser.hpp>
#include <dfa/constraint_set.hpp>

namespace sw {
    namespace dfa {

		// forward declarations
		template<typename Scalar> struct Hyperplane;
		struct TensorTypeInfo;

		template<typename ConstraintCoefficientType> ConstraintSet;

		template<typename ConstraintCoefficientType> 
		class Coordinate : public std::vector<ConstraintCoefficientType>;

		// a Confluence is the association of a tensor to a face of a DomainfOfComputation
		//
		// A Confluence must associate the 'corners' of a tensor with the 'corners'
		// of the face of the convex hull that describes the domain of computation.
		// This association is the information that will allow faces to be aligned
		// between two communicating operators.
		//
		template<typename ConstraintCoefficientType = int>
		class Confluence {
		public:
		private:
			std::string tensorSpec;  // something like tensor<4x256x16xf32>
		};

		template<typename ConstraintCoefficientType = int>
		class DomainOfComputation {
			using Constraint = Hyperplane<ConstraintCoefficientType>;
		public:
			// default constructor
			DomainOfComputation() = default;
			// constructor with initializer list
			DomainOfComputation(std::initializer_list<Constraint> init_constraints)
				: constraints(init_constraints) {}

			void addConstraint(const Constraint& c) noexcept { constraints.push_back(c); }


			// modifiers
			bool empty() const noexcept { return constraints.empty(); }
			void clear() noexcept { constraints.clear(); }

			// selectors
			const ConstraintSet& get_constraints() const noexcept { return constraints; }

			PointSet getConvexHull() {
			    PointSet points;

			    return points;
			}
			
		private:
			ConstraintSet constraints;
			Confluences inputFaces;
			Confluences outputFaces;
		};

    }
}
