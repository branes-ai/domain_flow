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

		// Represents a point in nD space (1D to 6D)
		template<typename ConstraintCoefficientType>
		struct Point {
			std::vector<ConstraintCoefficientType> coords; // Coordinates in arbitrary dimension

			explicit Point(const std::vector<ConstraintCoefficientType>& coords_ = {}) : coords(coords_) {}

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
				// TBD: Validate number of vertices
				// currently, we assume we can define polytopes as faces, so whatever the number of vertices the polytope has
				// we'll accept it as a face

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

		// ostream operator for ConvexHull
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
    }
}
