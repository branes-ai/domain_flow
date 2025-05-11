#pragma once

#include <dfa/vector.hpp>
#include <dfa/matrix.hpp>

namespace sw {
    namespace dfa {

        // Transformation class
        template<typename Scalar = float>
        class Transformation {
            using Vec3 = Vector3<Scalar>;
            using Mat3 = Matrix3<Scalar>;
            using Mat4 = Matrix4<Scalar>;
        public:
            Transformation() : T_{} {}
            Transformation(
                const Vec3& face_normal, const Vec3& v0, const Vec3& v1, const Vec3& v2,
                const Vec3& target_normal, const Vec3& target_x_axis,
                const std::vector<Vec3>& face_vertices
            ) {
                computeTransformation(face_normal, v0, v1, v2, target_normal, target_x_axis, face_vertices);
            }

            Transformation(
                const Vec3& source_normal, const Vec3& source_v0, const Vec3& source_v1, const Vec3& source_v2,
                const std::vector<Vec3>& source_vertices,
                const Vec3& target_normal, const Vec3& target_v0, const Vec3& target_v1, const Vec3& target_v2,
                const std::vector<Vec3>& target_vertices,
                double spacer
            ) {
                computeAbuttingTransformation(source_normal, source_v0, source_v1, source_v2, source_vertices,
                    target_normal, target_v0, target_v1, target_v2, target_vertices, spacer);
            }

            Mat4 getHomogeneousMatrix() const { return T_; }
            Mat3 getRotation() const {
                Mat3 R;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        R(i, j) = T_(i, j);
                return R;
            }
            Vec3 getTranslation() const {
                return { T_(0, 3), T_(1, 3), T_(2, 3) };
            }

        private:
			void computeAnchorTransformation(
				const Vec3& face_normal, const Vec3& v0, const Vec3& v1, const Vec3& v2,
				const std::vector<Vec3>& face_vertices
			) {
				// Rotation matrix is:
                //     [ 0, 1, 0]
                // R = [ 0, 0, 1]
                //     [ 1, 0, 0]
				// Translation vector is:
				//     [ 0, 0, 0]
				T_[0][0] = 0;
				T_[0][1] = 1;
				T_[0][2] = 0;
				T_[1][0] = 0;
				T_[1][1] = 0;
				T_[1][2] = 1;
				T_[2][0] = 1;
				T_[2][1] = 0;
				T_[2][2] = 0;
				T_[3][3] = 1;
			}

            void computeTransformation(
                Vec3 face_normal, const Vec3& v0, const Vec3& v1, const Vec3& v2,
                Vec3 target_normal, Vec3 target_x_axis,
                const std::vector<Vec3>& face_vertices
            ) {
                face_normal = face_normal.normalized();
                target_normal = target_normal.normalized();
                target_x_axis = target_x_axis.normalized();

                // Local coordinate system
                Vec3 x_axis = (v0 - v1).normalized(); // v0 - v1
                Vec3 y_axis = (v2 - v1).normalized(); // v2 - v1
                Vec3 z_axis = face_normal;
                if (x_axis.cross(y_axis).dot(z_axis) < 0) {
                    y_axis = y_axis * -1.0;
                }

                Mat3 M_local;
                for (int i = 0; i < 3; ++i) {
                    M_local(i, 0) = x_axis[i];
                    M_local(i, 1) = y_axis[i];
                    M_local(i, 2) = z_axis[i];
                }

                // Target coordinate system
                Vec3 target_y_axis = target_normal.cross(target_x_axis);
                if (target_y_axis.norm() < 1e-10)
                    throw std::runtime_error("Target normal and x-axis cannot be parallel");
                target_y_axis = target_y_axis.normalized();

                Mat3 M_target;
                for (int i = 0; i < 3; ++i) {
                    M_target(i, 0) = target_x_axis[i];
                    M_target(i, 1) = target_y_axis[i];
                    M_target(i, 2) = target_normal[i];
                }

                Mat3 R = M_target * M_local.transpose();

                // Translation to match v0: (0, 0, 0) -> (0, 0, n)
                Vector3 t(0, 0, 4);

                // Build homogeneous transformation matrix
                T_ = Mat4();
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        T_(i, j) = R(i, j);
                    }
                    T_(i, 3) = t[i];
                }
            }

            void computeAbuttingTransformation(
                Vec3 source_normal, const Vec3& source_v0, const Vec3& source_v1, const Vec3& source_v2,
                const std::vector<Vec3>& source_vertices,
                Vec3 target_normal, const Vec3& target_v0, const Vec3& target_v1, const Vec3& target_v2,
                const std::vector<Vec3>& target_vertices,
                double spacer
            ) {
                source_normal = source_normal.normalized();
                target_normal = target_normal.normalized();

                // Source coordinate system (Cin)
                Vec3 source_x_axis = (source_v0 - source_v1).normalized();
                Vec3 source_y_axis = (source_v2 - source_v1).normalized();
                Vec3 source_z_axis = source_normal;
                if (source_x_axis.cross(source_y_axis).dot(source_z_axis) < 0) {
                    source_y_axis = source_y_axis * -1.0;
                }

                Mat3 M_source;
                for (int i = 0; i < 3; ++i) {
                    M_source(i, 0) = source_x_axis[i];
                    M_source(i, 1) = source_y_axis[i];
                    M_source(i, 2) = source_z_axis[i];
                }

                // Target coordinate system (Cout)
                Vec3 target_x_axis = (target_v0 - target_v1).normalized();
                Vec3 target_y_axis = (target_v2 - target_v1).normalized();
                Vec3 target_z_axis = target_normal;
                if (target_x_axis.cross(target_y_axis).dot(target_z_axis) < 0) {
                    target_y_axis = target_y_axis * -1.0;
                }

                Mat3 M_target;
                for (int i = 0; i < 3; ++i) {
                    M_target(i, 0) = target_x_axis[i];
                    M_target(i, 1) = target_y_axis[i];
                    M_target(i, 2) = target_z_axis[i];
                }

                Mat3 R = M_target * M_source.transpose();

                // Compute centroids
                Vec3 source_centroid(0, 0, 0);
                for (const auto& v : source_vertices) {
                    source_centroid = source_centroid + v;
                }
                source_centroid = source_centroid * (1.0 / source_vertices.size());

                Vec3 target_centroid(0, 0, 0);
                for (const auto& v : target_vertices) {
                    target_centroid = target_centroid + v;
                }
                target_centroid = target_centroid * (1.0 / target_vertices.size());

                // Transform target centroid
                Vec3 transformed_target_centroid = Matrix4(
                    {
                        {R(0,0), R(0,1), R(0,2), 0},
                        {R(1,0), R(1,1), R(1,2), 0},
                        {R(2,0), R(2,1), R(2,2), 0},
                        {0, 0, 0, 1}
                    }).transformPoint(target_centroid);

                // Translation: Align source centroid to target centroid + spacer
                Vec3 t = transformed_target_centroid - R * source_centroid + Vec3(0, spacer, 0);

                // Build homogeneous transformation matrix
                T_ = Mat4();
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        T_(i, j) = R(i, j);
                    }
                    T_(i, 3) = t[i];
                }
            }

            Mat4 T_;
        };

    }
}
