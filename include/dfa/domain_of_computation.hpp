#pragma once
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <dfa/shape_analysis.hpp>
#include <dfa/tensor_spec_parser.hpp>
#include <dfa/convex_hull.hpp>
#include <dfa/constraint_set.hpp>

namespace sw {
    namespace dfa {

		// forward declarations
		template<typename Scalar> struct Hyperplane;
		struct TensorTypeInfo;

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
			Confluence(std::string tensorSpec, std::size_t faceId)
				: tensorSpec{ tensorSpec }, faceId{ faceId } {
			}
		private:
			std::string tensorSpec;  // something like tensor<4x256x16xf32>
			TensorTypeInfo tensorTypeInfo; // parsed tensor type information
			std::size_t faceId;      // the face ID of the convex hull

			template<typename ConstraintCoefficientType>
			friend inline std::ostream& operator<<(std::ostream& os, const Confluence<ConstraintCoefficientType>& c);
		};

		template<typename ConstraintCoefficientType>
		inline std::ostream& operator<<(std::ostream& os, const Confluence<ConstraintCoefficientType>& c) {
			os << "Confluence: " << c.tensorSpec << " Face ID: " << c.faceId;
			return os;
		}

		template<typename ConstraintCoefficientType>
		class ConfluenceSet : public std::vector<Confluence<ConstraintCoefficientType>> {
		public:
			void add(const Confluence<ConstraintCoefficientType>& c) noexcept { push_back(c); }
		};

		template<typename ConstraintCoefficientType>
		inline std::ostream& operator<<(std::ostream& os, const ConfluenceSet<ConstraintCoefficientType>& cs) {
			os << "ConfluenceSet:\n";
			for (const auto& c : cs) {
				os << "  " << c << '\n';
			}
			return os;
		}

		template<typename ConstraintCoefficientType = int>
		class DomainOfComputation {
			using Constraint = Hyperplane<ConstraintCoefficientType>;

		private:
			std::map<std::size_t, std::string> inputs_; // slotted string version of mlir::Type
			std::map<std::size_t, std::string> outputs_; // slotted string version of mlir::Type
			ConstraintSet<ConstraintCoefficientType> constraints_;

			ConvexHull<ConstraintCoefficientType> hull_;
			ConfluenceSet<ConstraintCoefficientType> inputFaces_;
			ConfluenceSet<ConstraintCoefficientType> outputFaces_;

		public:
			// default constructor
			DomainOfComputation() = default;
			// constructor with initializer list
			DomainOfComputation(std::initializer_list<Constraint> init_constraints)
				: inputs_{}, outputs_{}, constraints_{ init_constraints }, hull_{}, inputFaces_{}, outputFaces_{}  {}


			// modifiers
			void clear() noexcept { 
				constraints_.clear();
				inputs_.clear();
				outputs_.clear();
				inputFaces_.clear();
			}

			void addInput(std::size_t slot, const std::string& typeStr) noexcept { inputs_[slot] = typeStr; }
			void addOutput(std::size_t slot, const std::string& typeStr) noexcept { outputs_[slot] = typeStr; }
			void addConstraint(const Constraint& c) noexcept { constraints_.push_back(c); }

			/// <summary>
			/// elaborate the domain of computation for an operator.
			/// The domain flow operator needs to be associated with a specific parallel algorithm.
			/// The operand and result tensor types will determine the span of the domain of computation.
			/// elaborateDomainOfComputation will create the convex hull of the domain of computation
			/// and associate the tensor confluences with the faces of the convex hull.
			/// </summary>
			/// TODO: all the remaining DomainFlowOperator types
			/// <returns>no return type</returns>
			void elaborateDomainOfComputation(const DomainFlowOperator& opType) noexcept 
			{
				switch (opType) {
				case DomainFlowOperator::ADD:
				case DomainFlowOperator::SUB:
				case DomainFlowOperator::MUL:
				{
					TensorTypeInfo tensor0 = parseTensorType(getInput(0));
					TensorTypeInfo tensor1 = parseTensorType(getInput(1));
					TensorTypeInfo result = parseTensorType(getOutput(0));
					if (tensor0.empty() || tensor1.empty()) {
						std::cerr << "DomainOfComputation createDomainOfComputation: invalid add/sub/mul arguments: ignoring operator" << std::endl;
						break;
					}
					if (tensor0.shape != tensor1.shape) {
						std::cerr << "DomainOfComputation createDomainOfComputation: tensor shapes do not match: ignoring operator" << std::endl;
						break;
					}
					if (tensor0.shape != result.shape) {
						std::cerr << "DomainOfComputation createDomainOfComputation: tensor shapes do not match: ignoring operator" << std::endl;
						break;
					}

					// computational domain is batchSize3 x batchSize_2 x batchSize_1 x m x n
					// construct the convex hull of the domain of computation
					switch (tensor0.size()) {
					case 1:
					{
						// 1D line 
						hull_.setDimension(1); // 1D convex hull
						auto v0 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0 }));
						auto v1 = hull_.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0] }));
						auto f0 = hull_.add_face({ v0, v1 });
					}
						break;
					case 2:
					{
						// 2D plane
						hull_.setDimension(2); // 2D convex hull
						auto v0 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, 0 }));
						auto v1 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, tensor0.shape[1] }));
						auto v2 = hull_.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], tensor0.shape[1] }));
						auto v3 = hull_.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], 0 }));
						auto f0 = hull_.add_face({ v0, v1, v2, v3 });
					}
						break;
					case 3:
					{
						// 3D volume
						hull_.setDimension(3); // 3D convex hull
						auto v0 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, 0 }));
						auto v1 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, tensor0.shape[1], 0 }));
						auto v2 = hull_.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], tensor0.shape[1], 0 }));
						auto v3 = hull_.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], 0, 0 }));
						auto v4 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, tensor0.shape[2] }));
						auto v5 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, tensor0.shape[1], tensor0.shape[2] }));
						auto v6 = hull_.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], tensor0.shape[1], tensor0.shape[2] }));
						auto v7 = hull_.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], 0, tensor0.shape[2] }));
						auto f0 = hull_.add_face({ v0, v1, v2, v3 }); // left face
						auto f1 = hull_.add_face({ v4, v5, v6, v7 }); // right face
					}
						break;
					}
				}
				break;
				case DomainFlowOperator::MATMUL:
				{
					TensorTypeInfo tensor0 = parseTensorType(getInput(0));
					TensorTypeInfo tensor1 = parseTensorType(getInput(1));
					TensorTypeInfo tensor2; // in case we have an input C matrix
					if (inputs_.size() == 3) {
						tensor2 = parseTensorType(getInput(2));
					}
					TensorTypeInfo tensorOut = parseTensorType(getOutput(0));

					if (tensor0.empty() || tensor1.empty()) {
						std::cerr << "DomainOfComputation createDomainOfComputation: invalid matmul arguments: ignoring matmul operator" << std::endl;
						break;
					}

					shapeAnalysisResults result;
					if (!calculateMatmulShape(tensor0.shape, tensor1.shape, result)) {
						std::cerr << "DomainOfComputation createDomainOfComputation: " << result.errMsg << std::endl;
						break;
					}

					ConstraintCoefficientType m_ = result.m - 1;
					ConstraintCoefficientType k_ = result.k - 1;
					ConstraintCoefficientType n_ = result.n - 1;

					// TBD: Do we check the result tensor shape?

					// computational domain is m x k x n
					// system( (i, j, k) : 0 <= i < m, 0 <= j < n, 0 <= l < k)
					hull_.setDimension(3); // 3D convex hull
					// left face vertex sequence
					auto v0 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, 0 }));
					auto v1 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, k_ }));
					auto v2 = hull_.add_vertex(Point<ConstraintCoefficientType>({ m_, 0, k_ }));
					auto v3 = hull_.add_vertex(Point<ConstraintCoefficientType>({ m_, 0, 0 }));

					// right face vertex sequence
					auto v4 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, n_, 0 }));
					auto v5 = hull_.add_vertex(Point<ConstraintCoefficientType>({ 0, n_, k_ }));
					auto v6 = hull_.add_vertex(Point<ConstraintCoefficientType>({ m_, n_, k_ }));
					auto v7 = hull_.add_vertex(Point<ConstraintCoefficientType>({ m_, n_, 0 }));

					// define the faces
					// A tensor confluence
					auto f0 = hull_.add_face({ v0, v1, v2, v3 }); // left face
					Confluence<ConstraintCoefficientType> confluence0(getInput(0), f0);
					// B tensor confluence
					auto f1 = hull_.add_face({ v0, v4, v5, v1 }); // back face
					// input C tensor confluence
					auto f2 = hull_.add_face({ v0, v3, v7, v4 }); // bottom face
					// output C tensor confluence
					auto f3 = hull_.add_face({ v1, v2, v6, v5 }); // top face
					// remaining faces do not have tensor confluences
					hull_.add_face({ v2, v3, v7, v6 }); // front face
					hull_.add_face({ v4, v5, v6, v7 }); // right face
				}
				break;
				}
			}

			/// <summary>
			/// Interpret the DomainFlowOperator and construct the constraint set
			/// defining the domain of computation for the operator.
			/// </summary>
			void elaborateConstraintSet(const DomainFlowOperator& opType) noexcept 
			{
				// generate the constraints that define the domain of computation for the operator
				constraints_.clear();
				switch (opType) {
				case DomainFlowOperator::CONSTANT:
				{
					// constant operator
					//    %out = tosa.constant 0.000000e+00 : tensor<12x6xf32>
					auto tensorInfo = parseTensorType(getOutput(0));
					constraints_.shapeExtract(tensorInfo);
				}
				break;
				case DomainFlowOperator::ADD:
				case DomainFlowOperator::SUB:
				case DomainFlowOperator::MUL:
				{
					auto tensorInfo = parseTensorType(getInput(0));
					constraints_.shapeExtract(tensorInfo);
				}
				break;
				case DomainFlowOperator::MATMUL:
				{
					TensorTypeInfo tensor1 = parseTensorType(getInput(0));
					TensorTypeInfo tensor2 = parseTensorType(getInput(1));
					if (tensor1.empty() || tensor2.empty()) {
						std::cerr << "DomainFlowNode generateConstraintSet: invalid matmul arguments: ignoring matmul operator" << std::endl;
						break;
					}
					if (tensor1.size() != 2 || tensor2.size() != 2) {
						std::cerr << "DomainFlowNode generateConstraintSet: invalid matmul arguments: ignoring matmul operator" << std::endl;
						break;
					}
					TensorTypeInfo indexSpaceShape;
					// computational domain is m x k x n
					// system( (i, j, k) : 0 <= i < m, 0 <= j < n, 0 <= l < k)
					indexSpaceShape.elementType = tensor1.elementType;
					// tensor<m, k> * tensor<k, n> -> tensor<m, n>  -> index space is m x n x k
					if (tensor1.size() == 2 && tensor2.size() == 2) {
						int m = tensor1.shape[0];
						int k = tensor1.shape[1];
						int k1 = tensor2.shape[0];
						int n = tensor2.shape[1];
						if (k != k1) {
							std::cerr << "DomainFlowNode generateConstraintSet: tensor are incorrect shape: ignoring matmul operator" << std::endl;
							break;
						}
						indexSpaceShape.shape.push_back(m);
						indexSpaceShape.shape.push_back(n);
						indexSpaceShape.shape.push_back(k);
						constraints_.shapeExtract(indexSpaceShape);
					}
					// tensor<batchSize, m, k> * tensor<batchSize, k, n> -> tensor<batchSize, m, n>
					if (tensor1.size() == 3 && tensor2.size() == 3) {
						int m = tensor1.shape[0];
						int k = tensor1.shape[1];
						int k1 = tensor2.shape[0];
						int n = tensor2.shape[1];
						if (k != k1) {
							std::cerr << "DomainFlowNode generateConstraintSet: tensor are incorrect shape: ignoring matmul operator" << std::endl;
							break;
						}
						indexSpaceShape.shape.push_back(m);
						indexSpaceShape.shape.push_back(n);
						indexSpaceShape.shape.push_back(k);
						constraints_.shapeExtract(indexSpaceShape);
					}
				}
				break;
				}

				// report on any unprocessed nodes
				if (constraints_.empty()) {
					std::cerr << "DomainFlowNode generateConstraintSet: no constraints defined for this operator" << std::endl;
				}
			}

			// selectors
			bool empty() const noexcept { return constraints_.empty(); }

			std::string getInput(std::size_t slot) const noexcept {
				auto it = inputs_.find(slot);
				if (it != inputs_.end()) {
					return it->second;
				}
				return std::string{};
			}
			std::string getOutput(std::size_t slot) const noexcept {
				auto it = outputs_.find(slot);
				if (it != outputs_.end()) {
					return it->second;
				}
				return std::string{};
			}
			const std::map<std::size_t, std::string>& inputs() const noexcept { return this->inputs_; }
			const std::map<std::size_t, std::string>& outputs() const noexcept { return this->outputs_; }
			const std::vector<Confluence<ConstraintCoefficientType>>& inputFaces() const noexcept { return this->inputFaces_; }
			
			// get a copy of the constraints that define the domain of computation
			ConstraintSet<ConstraintCoefficientType> constraints() const noexcept { return this->constraints_; }

			// get the point set that defines the convex hull
			ConvexHull<ConstraintCoefficientType> convexHull() const noexcept {
				return hull_;
			}
			PointSet<ConstraintCoefficientType> convexHullPointSet() const noexcept {
				PointSet<ConstraintCoefficientType> points;
				for (const auto& vertex : hull_.vertices()) {
					points.add(vertex);
				}
				return points;
			}
			// get the confluence set that defines the tensor confluences
			ConfluenceSet<ConstraintCoefficientType> confluences() const noexcept {
				ConfluenceSet<ConstraintCoefficientType> confluences;
				//for (const auto& confluence : inputFaces_) {
				//	confluences.add(confluence);
				//}
				return confluences;
			}

		};

		template<typename ConstraintCoefficientType>
		std::ostream& operator<<(std::ostream& os, const DomainOfComputation<ConstraintCoefficientType>& doc) {
		}
    }
}
