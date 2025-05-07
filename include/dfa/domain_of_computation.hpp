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

			template<typename CCType>
			friend inline std::ostream& operator<<(std::ostream& os, const Confluence<CCType>& c);
		};

		template<typename CCType>
		inline std::ostream& operator<<(std::ostream& os, const Confluence<CCType>& c) {
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
			std::map<std::size_t, std::string> inputs; // slotted string version of mlir::Type
			std::map<std::size_t, std::string> outputs; // slotted string version of mlir::Type
			ConstraintSet<ConstraintCoefficientType> constraints;

			ConvexHull<ConstraintCoefficientType> hull;
			ConfluenceSet<ConstraintCoefficientType> inputFaces;
			ConfluenceSet<ConstraintCoefficientType> outputFaces;

			IndexSpace<ConstraintCoefficientType> indexSpace;

		public:
			// default constructor
			DomainOfComputation() = default;
			// constructor with initializer list
			DomainOfComputation(const DomainFlowOperator& opType, 
				                const std::map<std::size_t, std::string>& inputTensors,
				                const std::map<std::size_t, std::string>& outputTensors)
				: inputs{ inputTensors }, outputs{ outputTensors }, constraints{}, hull{}, inputFaces{}, outputFaces{}, indexSpace{} 
			{
				elaborateDomainOfComputation(opType);
				elaborateConstraintSet(opType);
				instantiateIndexSpace();
			}


			// modifiers
			void clear() noexcept { 
				constraints.clear();
				inputs.clear();
				outputs.clear();
				inputFaces.clear();
			}

			void addInput(std::size_t slot, const std::string& typeStr) noexcept { inputs[slot] = typeStr; }
			void addOutput(std::size_t slot, const std::string& typeStr) noexcept { outputs[slot] = typeStr; }
			void addConstraint(const Constraint& c) noexcept { constraints.push_back(c); }

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
						hull.setDimension(1); // 1D convex hull
						auto v0 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0 }));
						auto v1 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0] }));
						auto f0 = hull.add_face({ v0, v1 });
					}
						break;
					case 2:
					{
						// 2D plane
						hull.setDimension(2); // 2D convex hull
						auto v0 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, 0 }));
						auto v1 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, tensor0.shape[1] }));
						auto v2 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], tensor0.shape[1] }));
						auto v3 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], 0 }));
						auto f0 = hull.add_face({ v0, v1, v2, v3 });
					}
						break;
					case 3:
					{
						// 3D volume
						hull.setDimension(3); // 3D convex hull
						auto v0 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, 0 }));
						auto v1 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, tensor0.shape[1], 0 }));
						auto v2 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], tensor0.shape[1], 0 }));
						auto v3 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], 0, 0 }));
						auto v4 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, tensor0.shape[2] }));
						auto v5 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, tensor0.shape[1], tensor0.shape[2] }));
						auto v6 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], tensor0.shape[1], tensor0.shape[2] }));
						auto v7 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensor0.shape[0], 0, tensor0.shape[2] }));
						auto f0 = hull.add_face({ v0, v1, v2, v3 }); // left face
						auto f1 = hull.add_face({ v4, v5, v6, v7 }); // right face
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
					if (inputs.size() == 3) {
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

					// TBD: Do we need to check the Cin tensor shape and the result Cout tensor shape?

					// computational domain is m x k x n
					// system( (i, j, k) : 0 <= i < m, 0 <= j < n, 0 <= l < k)
					hull.setDimension(3); // 3D convex hull
					//
					//        v3 +--------------+ v4
					//          /|             /|                k
					//         / |            / |                ^
					//        /  |        v7 /  |                |
					//    v2 +--------------+   |                |
					//       |   +----------|---+ v5             +-------> n
					//       |  / v0        |  /                /
					//       | /            | /                /
					//       |/             |/                m
					//       +--------------+
					//     v1             v6 
					// 
					// left face vertex sequence
					auto v0 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, 0 }));
					auto v1 = hull.add_vertex(Point<ConstraintCoefficientType>({ m_, 0, 0 }));
					auto v2 = hull.add_vertex(Point<ConstraintCoefficientType>({ m_, 0, k_ }));
					auto v3 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, k_ }));

					// right face vertex sequence
					auto v4 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, n_, k_ }));
					auto v5 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, n_, 0 }));
					auto v6 = hull.add_vertex(Point<ConstraintCoefficientType>({ m_, n_, 0 }));
					auto v7 = hull.add_vertex(Point<ConstraintCoefficientType>({ m_, n_, k_ }));


					// define the faces: right hand rule pointing out of the volume
					// A tensor confluence
					auto f0 = hull.add_face({ v0, v1, v2, v3 }); // left face, pointing out
					Confluence<ConstraintCoefficientType> confluence0(getInput(0), f0);
					// B tensor confluence
					auto f1 = hull.add_face({ v0, v3, v4, v5 }); // back face, pointing out
					// input C tensor confluence
					auto f2 = hull.add_face({ v0, v5, v6, v1 }); // bottom face, pointing out
					// output C tensor confluence
					auto f3 = hull.add_face({ v3, v2, v7, v4 }); // top face, pointing out
					// remaining faces do not have tensor confluences
					hull.add_face({ v1, v6, v7, v2 }); // front face
					hull.add_face({ v5, v4, v7, v6 }); // right face
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
				constraints.clear();
				switch (opType) {
				case DomainFlowOperator::CONSTANT:
				{
					// constant operator
					//    %out = tosa.constant 0.000000e+00 : tensor<12x6xf32>
					auto tensorInfo = parseTensorType(getOutput(0));
					constraints.shapeExtract(tensorInfo);
				}
				break;
				case DomainFlowOperator::ADD:
				case DomainFlowOperator::SUB:
				case DomainFlowOperator::MUL:
				{
					auto tensorInfo = parseTensorType(getInput(0));
					constraints.shapeExtract(tensorInfo);
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
						constraints.shapeExtract(indexSpaceShape);
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
						constraints.shapeExtract(indexSpaceShape);
					}
				}
				break;
				}

				// report on any unprocessed nodes
				if (constraints.empty()) {
					std::cerr << "DomainFlowNode generateConstraintSet: no constraints defined for this operator" << std::endl;
				}
			}

			/// <summary>
			/// Generate the index space for the domain of computation.
			/// </summary>
			void instantiateIndexSpace() noexcept {
				indexSpace.setConstraints(constraints);
				indexSpace.instantiate();
			}

			// selectors
			bool empty() const noexcept { return constraints.empty(); }
			bool isInside(const IndexPoint& p) const noexcept {
				return constraints.isInside(p);
			}
			std::string getInput(std::size_t slot) const noexcept {
				auto it = inputs.find(slot);
				if (it != inputs.end()) {
					return it->second;
				}
				return std::string{};
			}
			std::string getOutput(std::size_t slot) const noexcept {
				auto it = outputs.find(slot);
				if (it != outputs.end()) {
					return it->second;
				}
				return std::string{};
			}
			const std::map<std::size_t, std::string>& getInputs() const noexcept { return this->inputs; }
			const std::map<std::size_t, std::string>& getOutputs() const noexcept { return this->outputs; }
			const std::vector<Confluence<ConstraintCoefficientType>>& getInputFaces() const noexcept { return this->inputFaces; }
			
			// get a copy of the constraints that define the domain of computation
			const ConstraintSet<ConstraintCoefficientType>& getConstraints() const noexcept { return this->constraints; }
			const IndexSpace<ConstraintCoefficientType>& getIndexSpace() const noexcept { return this->indexSpace; }

			ConvexHull<ConstraintCoefficientType> getConvexHull() const noexcept { return this->hull; }
			// get the point set that defines the convex hull
			PointSet<ConstraintCoefficientType> getConvexHullPointSet() const noexcept {
				PointSet<ConstraintCoefficientType> points;
				for (const auto& vertex : hull.vertices()) {
					points.add(vertex);
				}
				return points;
			}
			// get the confluence set that defines the tensor confluences
			ConfluenceSet<ConstraintCoefficientType> getConfluences() const noexcept {
				ConfluenceSet<ConstraintCoefficientType> confluences;
				//for (const auto& confluence : inputFaces) {
				//	confluences.add(confluence);
				//}
				return confluences;
			}

		};

		template<typename ConstraintCoefficientType>
		inline std::ostream& operator<<(std::ostream& os, const DomainOfComputation<ConstraintCoefficientType>& doc) {
			os << "DomainOfComputation:\n";
			os << "  Inputs:\n";
			for (const auto& input : doc.getInputs()) {
				os << "    " << input.first << ": " << input.second << '\n';
			}
			os << "  Outputs:\n";
			for (const auto& output : doc.getOutputs()) {
				os << "    " << output.first << ": " << output.second << '\n';
			}
			os << "  Constraints:\n" << doc.getConstraints() << '\n';
			os << "  Convex Hull:\n" << doc.getConvexHull() << '\n';
			os << "  Confluences:\n" << doc.getConfluences() << '\n';
			os << "  Index Space:\n" << doc.getIndexSpace() << '\n';
			return os;
		}
    }
}
