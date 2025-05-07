#pragma once
#include<iostream>
#include<iomanip>
#include<array>

namespace sw {
    namespace dfa {
        // Assuming Point and Face classes are defined with dimension(), operator[], vertices(), num_vertices()

        // 3D vector with basic operations
        struct Vector3 {
            double x, y, z;

            Vector3 operator-(const Vector3& other) const {
                return { x - other.x, y - other.y, z - other.z };
            }

            Vector3 cross(const Vector3& other) const {
                return {
                    y * other.z - z * other.y,
                    z * other.x - x * other.z,
                    x * other.y - y * other.x
                };
            }

            double dot(const Vector3& other) const {
                return x * other.x + y * other.y + z * other.z;
            }

            Vector3 normalize() const {
                double mag = std::sqrt(dot(*this));
                if (mag == 0) throw std::runtime_error("Cannot normalize zero vector");
                return { x / mag, y / mag, z / mag };
            }
        };

        // 4x4 homogeneous transformation matrix
        struct Matrix4 {
            std::array<std::array<double, 4>, 4> m = { {{0}} };

            static Matrix4 identity() {
                Matrix4 mat;
                for (size_t i = 0; i < 4; ++i) mat.m[i][i] = 1.0;
                return mat;
            }

            Matrix4 operator*(const Matrix4& other) const {
                Matrix4 result;
                for (size_t i = 0; i < 4; ++i) {
                    for (size_t j = 0; j < 4; ++j) {
                        for (size_t k = 0; k < 4; ++k) {
                            result.m[i][j] += m[i][k] * other.m[k][j];
                        }
                    }
                }
                return result;
            }

            Vector3 transform(const Vector3& point) const {
                double homo[4] = { point.x, point.y, point.z, 1.0 };
                double result[3] = { 0 };
                for (size_t i = 0; i < 3; ++i) {
                    for (size_t j = 0; j < 4; ++j) {
                        result[i] += m[i][j] * homo[j];
                    }
                }
                return { result[0], result[1], result[2] };
            }
        };

        template<typename ConstraintCoefficientType>
        class FaceAlignment {
        public:
            // Compute transformation to align source face to target face
            static Matrix4 align(
                const ConvexHull<ConstraintCoefficientType>& source_hull, size_t source_face_id,
                const ConvexHull<ConstraintCoefficientType>& target_hull, size_t target_face_id,
                const Matrix4& source_transform) {

                auto source_coords = source_hull.face_coordinates(source_face_id);
                auto target_coords = target_hull.face_coordinates(target_face_id);

                if (source_coords.size() < 3 || target_coords.size() < 3) {
                    throw std::invalid_argument("Faces must have at least 3 vertices");
                }

                // Compute centroids
                Vector3 source_centroid = compute_centroid(source_coords);
                Vector3 target_centroid = compute_centroid(target_coords);

                // Compute normals
                Vector3 source_normal = compute_normal(source_coords);
                Vector3 target_normal = compute_normal(target_coords);

                // Rotation to align normals
                Matrix4 rotation = rotation_from_normals(source_normal, target_normal);

                // Translation to align centroids
                Vector3 transformed_source_centroid = source_transform.transform(source_centroid);
                Matrix4 translation = Matrix4::identity();
                translation.m[0][3] = target_centroid.x - transformed_source_centroid.x;
                translation.m[1][3] = target_centroid.y - transformed_source_centroid.y;
                translation.m[2][3] = target_centroid.z - transformed_source_centroid.z;

                return translation * rotation * source_transform;
            }

        private:
            static Vector3 to_vector3(const Point<ConstraintCoefficientType>& point) {
                return { static_cast<double>(point[0]), static_cast<double>(point[1]), static_cast<double>(point[2]) };
            }

            static Vector3 compute_centroid(const std::vector<Point<ConstraintCoefficientType>>& coords) {
                Vector3 centroid = { 0, 0, 0 };
                for (const auto& point : coords) {
                    Vector3 v = to_vector3(point);
                    centroid.x += v.x;
                    centroid.y += v.y;
                    centroid.z += v.z;
                }
                double n = static_cast<double>(coords.size());
                return { centroid.x / n, centroid.y / n, centroid.z / n };
            }

            static Vector3 compute_normal(const std::vector<Point<ConstraintCoefficientType>>& coords) {
                Vector3 v1 = to_vector3(coords[1]) - to_vector3(coords[0]);
                Vector3 v2 = to_vector3(coords[2]) - to_vector3(coords[0]);
                return v1.cross(v2).normalize();
            }

            static Matrix4 rotation_from_normals(const Vector3& source, const Vector3& target) {
                Vector3 axis = source.cross(target);
                double cos_theta = source.dot(target);
                double theta = std::acos(cos_theta);
                double s = std::sin(theta);
                double c = std::cos(theta);
                double t = 1 - c;

                Vector3 n = axis.normalize();
                Matrix4 mat = Matrix4::identity();
                mat.m[0][0] = t * n.x * n.x + c;
                mat.m[0][1] = t * n.x * n.y - s * n.z;
                mat.m[0][2] = t * n.x * n.z + s * n.y;
                mat.m[1][0] = t * n.x * n.y + s * n.z;
                mat.m[1][1] = t * n.y * n.y + c;
                mat.m[1][2] = t * n.y * n.z - s * n.x;
                mat.m[2][0] = t * n.x * n.z - s * n.y;
                mat.m[2][1] = t * n.y * n.z + s * n.x;
                mat.m[2][2] = t * n.z * n.z + c;

                return mat;
            }
        };

        template<typename ConstraintCoefficientType>
        class DomainFlowAlignment {
        public:
            struct FacePair {
                size_t source_hull_idx;
                size_t source_face_id;
                size_t target_hull_idx;
                size_t target_face_id;
            };

            struct Transform {
                Matrix4 matrix;
                FacePair connection;
            };

            static std::vector<Transform> align_hulls(
                const std::vector<ConvexHull<ConstraintCoefficientType>>& hulls,
                const std::vector<FacePair>& face_connections) {

                if (hulls.empty()) return {};

                // Validate inputs
                for (const auto& hull : hulls) {
                    if (hull.dimension() != 3) {
                        throw std::invalid_argument("All hulls must be 3D");
                    }
                }
                for (const auto& conn : face_connections) {
                    if (conn.source_hull_idx >= hulls.size() || conn.target_hull_idx >= hulls.size() ||
                        conn.source_face_id >= hulls[conn.source_hull_idx].num_faces() ||
                        conn.target_face_id >= hulls[conn.target_hull_idx].num_faces()) {
                        throw std::invalid_argument("Invalid hull or face index");
                    }
                }

                std::vector<Transform> transforms(hulls.size());
                std::vector<bool> visited(hulls.size(), false);
                transforms[0].matrix = Matrix4::identity();
                visited[0] = true;

                // Compute transformations
                for (const auto& conn : face_connections) {
                    if (!visited[conn.target_hull_idx]) {
                        transforms[conn.target_hull_idx].matrix = FaceAlignment<ConstraintCoefficientType>::align(
                            hulls[conn.source_hull_idx], conn.source_face_id,
                            hulls[conn.target_hull_idx], conn.target_face_id,
                            transforms[conn.source_hull_idx].matrix);
                        transforms[conn.target_hull_idx].connection = conn;
                        visited[conn.target_hull_idx] = true;
                    }
                }

                // Handle unconnected hulls
                for (size_t i = 0; i < hulls.size(); ++i) {
                    if (!visited[i]) {
                        transforms[i].matrix = Matrix4::identity();
                    }
                }

                return transforms;
            }
        };
	
		class DomainFlow {
        public:
			using ConstraintCoefficientType = int;
			using CCType = ConstraintCoefficientType;
            using VertexId = size_t;
            using FaceId = size_t;

            // Represents a pair of faces (input and output) for alignment
            struct FacePair {
                size_t source_hull_idx; // Index of the source hull
                size_t target_hull_idx; // Index of the target hull
                FaceId source_face_id;  // ID of the source face
                FaceId target_face_id;  // ID of the target face
 /*               FacePair(size_t srcHullIdx, size_t tgtHullIdx, size_t srcFaceId, size_t tgtFaceId)
                    : source_hull_idx(srcHullIdx), target_hull_idx(tgtHullIdx),
                    source_face_id(srcFaceId), target_face_id(tgtFaceId) {
                }*/
            };

            struct Transform {
                Matrix4 matrix; // 4x4 homogeneous transformation matrix
                FacePair connection;
            };

		public:
			void addConvexHull(const ConvexHull<CCType>& hull) {
				convex_hulls.push_back(hull);
			}

			void alignHulls() {
				for (auto& ch : convex_hulls) {
					std::cout << ch << '\n';
				}
			}

		private:
			std::vector<ConvexHull<CCType>> convex_hulls;

			friend inline std::ostream& operator<<(std::ostream& os, const DomainFlow& df) {
				os << "Domain Flow Space:\n";
				for (const auto& ch : df.convex_hulls) {
					os << "  " << ch << '\n';
				}
				return os;
			}
		};


    }
}
