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
			Confluence(std::string tensorSpec, std::size_t faceId, std::string epilogue = "")
				: tensorSpec{ tensorSpec }, faceId{ faceId }, epilogue{ epilogue } {
			}

			const std::string& getTensorSpec() const noexcept { return tensorSpec; }
			std::size_t getFaceId() const noexcept { return faceId; }
			// optional pointwise epilogue fused onto this face (e.g. "relu" on the
			// terminal output face of a MATMUL); empty when the face is a pure
			// tensor confluence
			bool hasEpilogue() const noexcept { return !epilogue.empty(); }
			const std::string& getEpilogue() const noexcept { return epilogue; }
		private:
			std::string tensorSpec;  // something like tensor<4x256x16xf32>
			TensorTypeInfo tensorTypeInfo; // parsed tensor type information
			std::size_t faceId;      // the face ID of the convex hull
			std::string epilogue;    // pointwise operation applied at this face, "" if none

			template<typename CCType>
			friend inline std::ostream& operator<<(std::ostream& os, const Confluence<CCType>& c);
		};

		template<typename CCType>
		inline std::ostream& operator<<(std::ostream& os, const Confluence<CCType>& c) {
			os << "Confluence: " << c.tensorSpec << " Face ID: " << c.faceId;
			if (c.epilogue.size() > 0) os << " Epilogue: " << c.epilogue;
			return os;
		}

		template<typename ConstraintCoefficientType>
		class ConfluenceSet : public std::vector<Confluence<ConstraintCoefficientType>> {
		public:
			// this-> is required: push_back lives in the dependent std::vector base
			void add(const Confluence<ConstraintCoefficientType>& c) noexcept { this->push_back(c); }
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

			// handles to the tensor-confluence faces of the matmul (i,j,k) polyhedron
			struct MatmulHullFaces {
				std::size_t aFace;         // left face: A streams in
				std::size_t bFace;         // back face: B streams in
				std::size_t bottomFace;    // k = 0 face: accumulator seed (Cin)
				std::size_t terminalFace;  // k = K-1 face: output (and fused epilogue)
			};

			// construct the matmul (i,j,k) polyhedron shared by MATMUL and
			// FUSED_MATMUL_BIAS_ACT; callers attach their confluences to the
			// returned face handles
			//
			// computational domain is m x k x n
			// system( (i, j, k) : 0 <= i < m, 0 <= j < n, 0 <= l < k)
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
			MatmulHullFaces buildMatmulHull(ConstraintCoefficientType m_,
			                                ConstraintCoefficientType n_,
			                                ConstraintCoefficientType k_) {
				hull.setDimension(3); // 3D convex hull
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
				MatmulHullFaces faces{};
				faces.aFace        = hull.add_face({ v0, v1, v2, v3 }); // left face, pointing out
				faces.bFace        = hull.add_face({ v0, v3, v4, v5 }); // back face, pointing out
				faces.bottomFace   = hull.add_face({ v0, v5, v6, v1 }); // bottom face, pointing out
				faces.terminalFace = hull.add_face({ v3, v2, v7, v4 }); // top face, pointing out
				// remaining faces do not have tensor confluences
				hull.add_face({ v1, v6, v7, v2 }); // front face
				hull.add_face({ v5, v4, v7, v6 }); // right face
				return faces;
			}

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
				outputFaces.clear();
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
			/// <param name="activation">optional pointwise activation fused onto the
			/// operator's terminal output face (e.g. "relu"); empty for none</param>
			void elaborateDomainOfComputation(const DomainFlowOperator& opType, const std::string& activation = "") noexcept
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
						std::cerr << "DomainOfComputation elaborateDomainOfComputation: invalid add/sub/mul arguments: ignoring operator" << std::endl;
						break;
					}
					if (tensor0.shape != tensor1.shape) {
						std::cerr << "DomainOfComputation elaborateDomainOfComputation: tensor shapes do not match: ignoring operator" << std::endl;
						break;
					}
					if (tensor0.shape != result.shape) {
						std::cerr << "DomainOfComputation elaborateDomainOfComputation: tensor shapes do not match: ignoring operator" << std::endl;
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
						hull.add_face({ v0, v1 });
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
						hull.add_face({ v0, v1, v2, v3 });
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
						hull.add_face({ v0, v1, v2, v3 }); // left face
						hull.add_face({ v4, v5, v6, v7 }); // right face
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

					auto faces = buildMatmulHull(m_, n_, k_);
					// A and B tensor confluences on their streaming faces
					inputFaces.add(Confluence<ConstraintCoefficientType>(getInput(0), faces.aFace));
					inputFaces.add(Confluence<ConstraintCoefficientType>(getInput(1), faces.bFace));
					// input C tensor confluence on the k = 0 accumulator-seed face
					// (only when the 3-input Cout = A*B + Cin form is used)
					if (inputs.size() == 3) {
						inputFaces.add(Confluence<ConstraintCoefficientType>(getInput(2), faces.bottomFace));
					}
					// output C tensor confluence on the terminal k = K-1 face; a fused
					// pointwise epilogue (issue #1: bias via Cin, activation via node
					// attribute) is recorded here, as it consumes the matmul result in
					// place on this face without adding iteration dimensions
					outputFaces.add(Confluence<ConstraintCoefficientType>(getOutput(0), faces.terminalFace, activation));
				}
				break;
				case DomainFlowOperator::FUSED_MATMUL_BIAS_ACT:
				{
					// dedicated fused operator (issue #2):  Y = act(A*B + bias)
					// The fusion merges domains: over D = {(i,j,k)} the matmul
					// accumulates C(i,j,k) = C(i,j,k-1) + A(i,k)*B(k,j), and the
					// epilogue is a boundary recurrence on the terminal k = K-1 face:
					// Y(i,j) = act(C(i,j,K-1) + bias(i,j)).  The only value leaving D
					// is Y -- no intermediate tensor is materialized.
					//
					// Unlike the 3-input MATMUL (where Cin seeds the accumulator on the
					// k = 0 face), the bias here enters on the *terminal* face where the
					// epilogue executes.
					if (inputs.size() != 3) {
						std::cerr << "DomainOfComputation elaborateDomainOfComputation: FUSED_MATMUL_BIAS_ACT requires operands A, B, and bias: ignoring operator" << std::endl;
						break;
					}
					TensorTypeInfo tensor0 = parseTensorType(getInput(0));
					TensorTypeInfo tensor1 = parseTensorType(getInput(1));
					if (tensor0.empty() || tensor1.empty()) {
						std::cerr << "DomainOfComputation elaborateDomainOfComputation: invalid fused matmul arguments: ignoring operator" << std::endl;
						break;
					}
					shapeAnalysisResults result;
					if (!calculateMatmulShape(tensor0.shape, tensor1.shape, result)) {
						std::cerr << "DomainOfComputation elaborateDomainOfComputation: " << result.errMsg << std::endl;
						break;
					}
					ConstraintCoefficientType m_ = result.m - 1;
					ConstraintCoefficientType k_ = result.k - 1;
					ConstraintCoefficientType n_ = result.n - 1;

					// same (i,j,k) polyhedron as MATMUL: the epilogue adds no iteration
					// dimensions -- it is pointwise on the (i,j) output face
					auto faces = buildMatmulHull(m_, n_, k_);
					// A and B stream in on their respective faces; the bottom (k = 0)
					// face is the accumulator init and carries no tensor confluence
					inputFaces.add(Confluence<ConstraintCoefficientType>(getInput(0), faces.aFace));
					inputFaces.add(Confluence<ConstraintCoefficientType>(getInput(1), faces.bFace));
					// terminal k = K-1 face: bias enters here (broadcast on the (i,j)
					// face) and Y leaves here with the activation epilogue
					inputFaces.add(Confluence<ConstraintCoefficientType>(getInput(2), faces.terminalFace));
					outputFaces.add(Confluence<ConstraintCoefficientType>(getOutput(0), faces.terminalFace, activation));
				}
				break;
				case DomainFlowOperator::FUNCTION_RETURN:
				{
					TensorTypeInfo tensorIn = parseTensorType(getInput(0));
					TensorTypeInfo tensorOut = parseTensorType(getOutput(0));
					if (tensorIn.shape != tensorOut.shape) {
						std::cerr << "DomainOfComputation elaborateDomainOfComputation: tensor shapes do not match: ignoring operator" << std::endl;
						break;
					}
					// construct the convex hull of the domain of computation
					switch (tensorOut.size()) {
					case 1:
					{
						// 1D line
						hull.setDimension(1); // 1D convex hull
						auto v0 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0 }));
						auto v1 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensorOut.shape[0] }));
						hull.add_face({ v0, v1 });
					}
					break;
					case 2:
					{
						// 2D plane
						hull.setDimension(2); // 2D convex hull
						auto v0 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, 0 }));
						auto v1 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, tensorOut.shape[1] }));
						auto v2 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensorOut.shape[0], tensorOut.shape[1] }));
						auto v3 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensorOut.shape[0], 0 }));
						hull.add_face({ v0, v1, v2, v3 });
					}
					break;
					case 3:
					{
						// 3D volume
						hull.setDimension(3); // 3D convex hull
						auto v0 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, 0 }));
						auto v1 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, tensorOut.shape[1], 0 }));
						auto v2 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensorOut.shape[0], tensorOut.shape[1], 0 }));
						auto v3 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensorOut.shape[0], 0, 0 }));
						auto v4 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, 0, tensorOut.shape[2] }));
						auto v5 = hull.add_vertex(Point<ConstraintCoefficientType>({ 0, tensorOut.shape[1], tensorOut.shape[2] }));
						auto v6 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensorOut.shape[0], tensorOut.shape[1], tensorOut.shape[2] }));
						auto v7 = hull.add_vertex(Point<ConstraintCoefficientType>({ tensorOut.shape[0], 0, tensorOut.shape[2] }));
						hull.add_face({ v0, v1, v2, v3 }); // left face
						hull.add_face({ v4, v5, v6, v7 }); // right face
					}
					break;
					}
				}
				break;
				default:
					// Unhandled DomainFlowOperator types (TODO: implement remaining operators)
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
				case DomainFlowOperator::FUSED_MATMUL_BIAS_ACT:   // same (i,j,k) index space; epilogue adds no dimensions
				{
					TensorTypeInfo tensor1 = parseTensorType(getInput(0));
					TensorTypeInfo tensor2 = parseTensorType(getInput(1));
					if (tensor1.empty() || tensor2.empty()) {
						std::cerr << "DomainFlowNode elaborateConstraintSet: invalid matmul arguments: ignoring matmul operator" << std::endl;
						break;
					}
					if (tensor1.size() != 2 || tensor2.size() != 2) {
						std::cerr << "DomainFlowNode elaborateConstraintSet: invalid matmul arguments: ignoring matmul operator" << std::endl;
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
							std::cerr << "DomainFlowNode elaborateConstraintSet: tensor are incorrect shape: ignoring matmul operator" << std::endl;
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
							std::cerr << "DomainFlowNode elaborateConstraintSet: tensor are incorrect shape: ignoring matmul operator" << std::endl;
							break;
						}
						indexSpaceShape.shape.push_back(m);
						indexSpaceShape.shape.push_back(n);
						indexSpaceShape.shape.push_back(k);
						constraints.shapeExtract(indexSpaceShape);
					}
				}
				break;
				case DomainFlowOperator::FUNCTION_RETURN:
				{
					TensorTypeInfo tensorIn = parseTensorType(getInput(0));
					TensorTypeInfo tensorOut = parseTensorType(getOutput(0));
					if (tensorIn.shape != tensorOut.shape) {
						std::cerr << "DomainOfComputation elaborateDomainOfComputation: tensor shapes do not match: ignoring operator" << std::endl;
						break;
					}
					constraints.shapeExtract(tensorOut);
				}
				break;
				default:
					// Unhandled operators fall through to the constraints.empty() check below
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
			const std::vector<Confluence<ConstraintCoefficientType>>& getOutputFaces() const noexcept { return this->outputFaces; }
			
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
				for (const auto& confluence : inputFaces) {
					confluences.add(confluence);
				}
				for (const auto& confluence : outputFaces) {
					confluences.add(confluence);
				}
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
