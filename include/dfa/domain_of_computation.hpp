#pragma once
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <dfa/tensor_spec_parser.hpp>
#include <dfa/constraint_set.hpp>





namespace sw {
    namespace dfa {

		// forward declarations
		template<typename Scalar> struct Hyperplane;
		struct TensorTypeInfo;

		//template<typename ConstraintCoefficientType> ConstraintSet;

		// Represents a point in nD space (1D to 6D)
		template<typename ConstraintCoefficientType>
		struct Point {
			std::vector<ConstraintCoefficientType> coords; // Coordinates in arbitrary dimension

			explicit Point(const std::vector<double>& coords_ = {}) : coords(coords_) {}

			// Dimension of the point
			size_t dimension() const { return coords.size(); }

			friend std::ostream& operator<<(std::ostream& os, const Point& p) {
				os << "(";
				for (size_t i = 0; i < p.coords.size(); ++i) {
					os << p.coords[i];
					if (i < p.coords.size() - 1) os << ", ";
				}
				os << ")";
				return os;
			}
		};

		// Represents a set of points in nD space (1D to 6D)
		template<typename ConstraintCoefficientType>
		struct PointSet {
			std::vector<Point<ConstraintCoefficientType>> pointSet;
			void clear() { pointSet.clear(); }
			void add(const Point<ConstraintCoefficientType>& p) {
				pointSet.push_back(p);
			}
		};

		// Represents a face (tensor slice) defined by a variable number of vertices
		struct Face {
			std::vector<size_t> vertex_ids; // Ordered vertex IDs (tensor slice corners)

			explicit Face(const std::vector<size_t>& vertex_ids_ = {}) : vertex_ids(vertex_ids_) {}

			// Number of vertices in the face
			size_t num_vertices() const { return vertex_ids.size(); }

			// Get vertex IDs
			const std::vector<size_t>& vertices() const { return vertex_ids; }
		};

		template<typename ConstraintCoefficientType = int>
		class ConvexHull {
		public:
			using VertexId = size_t;
			using FaceId = size_t;

			ConvexHull() = default;

			// modifiers

			void clear() noexcept {
				vertices_.clear();
				faces_.clear();
				vertex_to_faces_.clear();
			}
			void setDimension(std::size_t dimension) noexcept {
				if (dimension < 1 || dimension > 6) {
					std::cerr << "Invalid dimension: " + std::to_string(dimension) + "\n";
				}
				dimension_ = dimension;
			}

			// Add a vertex and return its ID
			VertexId add_vertex(const Point<ConstraintCoefficientType>& point) {
				if (point.dimension() != dimension_) {
					std::cerr << "Point dimension does not match hull dimension\n";
				}
				vertices_.push_back(point);
				return vertices_.size() - 1;
			}

			// Add a face defined by vertex IDs (number of vertices = index extremes of the tensor slice mapped to this face)
			FaceId add_face(const std::vector<size_t>& vertex_ids) {
				// Validate number of vertices
				if (vertex_ids.size() != dimension_ - 1) {
					std::cerr << "face must have " + std::to_string(dimension_ - 1) +
						" vertices for a " + std::to_string(dimension_) + "D tensor slice\n";
					return 0;
				}

				// Validate vertex indices
				for (auto vid : vertex_ids) {
					if (vid >= vertices_.size()) {
						std::cerr << "invalid vertex index: " + std::to_string(vid) << '\n';
					}
				}

				// Check for duplicate vertices
				std::unordered_set<size_t> unique_vertices(vertex_ids.begin(), vertex_ids.end());
				if (unique_vertices.size() != vertex_ids.size()) {
					std::cerr << "degenerate face: duplicate vertices\n";
				}

				Face face(vertex_ids);
				faces_.push_back(face);
				FaceId face_id = faces_.size() - 1;

				// Update vertex-to-face adjacency
				for (auto vid : vertex_ids) {
					vertex_to_faces_[vid].insert(face_id);
				}

				return face_id;
			}

			// selectors

			const std::vector<Point<ConstraintCoefficientType>>& vertices() const { return vertices_; }
			const std::vector<Face>& faces() const { return faces_; }

			// Access face by ID
			const Face& face(FaceId face_id) const {
				if (face_id >= faces_.size()) {
					throw std::out_of_range("Invalid face ID: " + std::to_string(face_id));
				}
				return faces_[face_id];
			}

			// Access vertex coordinates by ID
			const Point<ConstraintCoefficientType>& vertex(VertexId vertex_id) const {
				if (vertex_id >= vertices_.size()) {
					throw std::out_of_range("Invalid vertex ID: " + std::to_string(vertex_id));
				}
				return vertices_[vertex_id];
			}

			// Get coordinates of vertices for a given face
			std::vector<Point<ConstraintCoefficientType>> face_coordinates(FaceId face_id) const {
				const Face& f = face(face_id);
				std::vector<Point> coords;
				coords.reserve(f.num_vertices());
				for (auto vid : f.vertices()) {
					coords.push_back(vertex(vid));
				}
				return coords;
			}

			// Get faces incident to a vertex
			const std::unordered_set<FaceId>& faces_at_vertex(VertexId vertex_id) const {
				if (vertex_id >= vertices_.size()) {
					throw std::out_of_range("Invalid vertex ID: " + std::to_string(vertex_id));
				}
				static const std::unordered_set<FaceId> empty_set;
				auto it = vertex_to_faces_.find(vertex_id);
				return it != vertex_to_faces_.end() ? it->second : empty_set;
			}

			size_t dimension() const { return dimension_; }
			size_t num_vertices() const { return vertices_.size(); }
			size_t num_faces() const { return faces_.size(); }



		private:
			size_t dimension_; // Dimension of the convex hull (1D to 6D)
			std::vector<Point<ConstraintCoefficientType>> vertices_; // List of vertex coordinates
			std::vector<Face> faces_;     // List of faces (tensor slices)
			std::unordered_map<VertexId, std::unordered_set<FaceId>> vertex_to_faces_; // Vertex-to-face adjacency
		};

		// Streaming operator for ConvexHull
		template<typename ConstraintCoefficientType = int>
		std::ostream& operator<<(std::ostream& os, const ConvexHull<ConstraintCoefficientType>& hull) {
			os << "ConvexHull (dimension: " << hull.dimension() << ")\n";

			// Output vertices
			os << "Vertices (" << hull.num_vertices() << "):\n";
			for (size_t i = 0; i < hull.num_vertices(); ++i) {
				os << "  Vertex " << i << ": " << hull.vertex(i) << "\n";
			}

			// Output faces (tensor slices)
			os << "Faces (" << hull.num_faces() << "):\n";
			for (size_t i = 0; i < hull.num_faces(); ++i) {
				const Face& face = hull.face(i);
				os << "  Face " << i << " (vertices: " << face.num_vertices() << "): [";
				const auto& vertex_ids = face.vertices();
				for (size_t j = 0; j < vertex_ids.size(); ++j) {
					os << vertex_ids[j];
					if (j < vertex_ids.size() - 1) os << ", ";
				}
				os << "]\n";
			}

			return os;
		}

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

		private:
			DomainFlowOperator opType_{ DomainFlowOperator::UNKNOWN }; // domain flow operator type
			std::map<std::size_t, std::string> inputs_; // slotted string version of mlir::Type
			std::map<std::size_t, std::string> outputs_; // slotted string version of mlir::Type
			ConstraintSet<ConstraintCoefficientType> constraints_;

			ConvexHull<ConstraintCoefficientType> hull_;
			std::vector<Confluence<ConstraintCoefficientType>> inputFaces_;
			std::vector<Confluence<ConstraintCoefficientType>> outputFaces_;

		public:
			// default constructor
			DomainOfComputation() = default;
			// constructor with initializer list
			DomainOfComputation(std::initializer_list<Constraint> init_constraints)
				: inputs_{}, outputs{}, constraints_{ init_constraints }, hull_{}, inputFaces_{}, outputFaces_{}  {}


			// modifiers
			void clear() noexcept { 
				constraints_.clear();
				inputs_.clear();
				outputs_.clear();
				inputFaces_.clear();
			}

			void setOperator(DomainFlowOperator opType) noexcept { opType_ = opType; }
			void addInput(std::size_t slot, const std::string& typeStr) noexcept { inputs_[slot] = typeStr; }
			void addOutput(std::size_t slot, const std::string& typeStr) noexcept { outputs_[slot] = typeStr; }
			void addConstraint(const Constraint& c) noexcept { constraints_.push_back(c); }

			// create the domain of	computation (DoC) given the current constraints
			void createDomainOfComputation() noexcept 
			{
				switch (opType_) {
				case DomainFlowOperator::ADD:
				case DomainFlowOperator::SUB:
				case DomainFlowOperator::MUL:
				{
					addInput(0, parseTensorType(getInput(0)));
					addInput(1, parseTensorType(getInput(1)));
					addOutput(0, parseTensorType(getOutput(0)));
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
					TensorTypeInfo result = parseTensorType(getOutput(0));

					if (tensor0.empty() || tensor1.empty()) {
						std::cerr << "DomainOfComputation createDomainOfComputation: invalid matmul arguments: ignoring matmul operator" << std::endl;
						break;
					}

					//shapeAnalysisResults result;
					//if (!calculateMatmulComplexity(tensor0.shape, tensor1.shape, result)) {
					//	std::cerr << "DomainOfComputation createDomainOfComputation: " << result.errMsg << std::endl;
					//	break;
					//}

					// all validated, so add the inputs and outputs
					addInput(0, tensor0);
					addInput(1, tensor1);
					if (inputs.size() == 2) addInput(2, tensor2);
					addOutput(0, result);

					ConstraintCoefficientType m_ = result.m - 1;
					ConstraintCoefficientType k_ = result.k - 1;
					ConstraintCoefficientType n_ = result.n - 1;

					// computational domain is m x k x n
					// system( (i, j, k) : 0 <= i < m, 0 <= j < n, 0 <= l < k)
					hull_.setDimension(3); // 3D convex hull
					// left face vertex sequence
					auto v0 = hull_.add_vertex(Point(0, 0, 0));
					auto v1 = hull_.add_vertex(Point(0, 0, k_));
					auto v2 = hull_.add_vertex(Point(m_, 0, k_));
					auto v3 = hull_.add_vertex(Point(m_, 0, 0));

					// right face vertex sequence
					auto v4 = hull_.add_vertex(Point(0, n_, 0));
					auto v5 = hull_.add_vertex(Point(0, n_, k_));
					auto v6 = hull_.add_vertex(Point(m_, n_, k_));
					auto v7 = hull_.add_vertex(Point(m_, n_, 0));

					// define the faces
					// A tensor confluence
					auto f0 = hull_.add_face({ v0, v1, v2, v3 }); // left face
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

			// selectors
			bool empty() const noexcept { return constraints.empty(); }

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
			const ConstraintSet<ConstraintCoefficientType>& constraints() const noexcept { return this->constraints_; }

			// get the point set that defines the convex hull
			PointSet<ConstraintCoefficientType> getConvexHull() const noexcept {
			    PointSet<ConstraintCoefficientType> points;
				for (const auto& vertex : hull_.vertices()) {
					points.add(vertex);
				}
			    return points;
			}

		};

		template<typename ConstraintCoefficientType>
		std::ostream& operator<<(std::ostream& os, const DomainOfComputation<ConstraintCoefficientType>& doc) {
		}
    }
}
