#include <cmath>
#include <vector>
#include <stdexcept>
#include <iostream>

#include <dfa/vector.hpp>
#include <dfa/matrix.hpp>

namespace sw {
    namespace dfa {

        // ConvexHull class
		template<typename ConstraintCoefficientType = int>
        class ConvexHull {
			using CCType = ConstraintCoefficientType;
			using MatX = MatrixX<CCType>;
			using VecX = VectorX<CCType>;
			using Mat3 = Matrix3<CCType>;
			using Vec3 = Vector3<CCType>;
			using Mat4 = Matrix4<CCType>;
        public:
            ConvexHull(const MatX& A, const VecX& b, double m, double n, double k)
                : A_(A), b_(b), m_(m), n_(n), k_(k) {
                if (A_.rows() != b_.size() || A_.cols() != 3)
                    throw std::invalid_argument("Invalid constraint dimensions");
            }

            const MatX& getA() const { return A_; }
            const VecX& getB() const { return b_; }

            ConvexHull transform(const Mat3& R, const Vec3& t) const {
                MatX A_new = A_ * R.transpose();
                VecX b_new(b_.size());
                for (int i = 0; i < b_.size(); ++i)
                    b_new(i) = b_(i) - A_.row(i).dot(t);
                return ConvexHull(A_new, b_new, m_, n_, k_);
            }

            std::vector<Vec3> getVertices() const {
                return {
                    Vec3(0, 0, 0),      // v0
                    Vec3(m_, 0, 0),     // v1
                    Vec3(m_, 0, k_),    // v2
                    Vec3(0, 0, k_),     // v3
                    Vec3(0, n_, k_),    // v4
                    Vec3(0, n_, 0),     // v5
                    Vec3(m_, n_, 0),    // v6
                    Vec3(m_, n_, k_)    // v7
                };
            }

            std::vector<Vec3> getTransformedVertices(const Mat4& T) const {
                std::vector<Vec3> vertices = getVertices();
                std::vector<Vec3> transformed;
                for (const auto& v : vertices) {
                    transformed.push_back(T.transformPoint(v));
                }
                return transformed;
            }

        private:
            MatX A_;
            VecX b_;
            CCType m_, n_, k_;
        };

        // Transformation class
		template<typename Scalar = float>
        class Transformation {
			using Vec3 = Vector3<Scalar>;
			using Mat3 = Matrix3<Scalar>;
			using Mat4 = Matrix4<Scalar>;
        public:
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
int main()
try {
    using namespace sw::dfa;

    using Scalar = float;
    using Vec3 = Vector3<Scalar>;
	using VecX = VectorX<Scalar>;
    using Mat3 = Matrix3<Scalar>;
	using Mat4 = Matrix4<Scalar>;
	using MatX = MatrixX<Scalar>;
    Scalar m = 8;
    Scalar n = 4;
    Scalar k = 6;
	Scalar spacer = 1.0;

    // tensor A = m x k
    // tensor B = k x n
    // tensor Cout = m x n
    // Prism 1 (Cout on top)
    MatX A1({
        {-1,  0,  0},  // -x <= -1 -> x >= 0
        { 1,  0,  0},  // x <= m
        { 0, -1,  0},  // -y <= -1 -> y >= 0
        { 0,  1,  0},  // y <= n
        { 0,  0, -1},  // -z <= -1 -> z >= 0
        { 0,  0,  1}   // z <= k
        });
    VecX b1({ -1, m, -1, n, -1, k });
    ConvexHull hull1(A1, b1, m, n, k);

    // Prism 2 (Cin on bottom)
    MatX A2({
        {-1,  0,  0},  // -x <= -1 -> x >= 0
        { 1,  0,  0},  // x <= m
        { 0, -1,  0},  // -y <= -1 -> y >= 0
        { 0,  1,  0},  // y <= n
        { 0,  0, -1},  // -z <= -1 -> z >= 0
        { 0,  0,  1}   // z <= k
        });
    VecX b2({ -1, m, -1, n, -1, k });
    ConvexHull hull2(A2, b2, m, n, k);

    // Prism 1: Cout face at z = k, normal [0, 0, 1]
    Vec3 cout_normal(0, 0, 1);
    Vec3 cout_v0(8, 0, 6);    // v2
    Vec3 cout_v1(0, 0, 6);    // v3, Cout(0,0)
    Vec3 cout_v2(0, 4, 6);    // v4
    std::vector<Vec3> cout_vertices = {
        Vec3(8, 0, 6),   // v2
        Vec3(0, 0, 6),   // v3
        Vec3(0, 4, 6),   // v4
        Vec3(8, 4, 6)    // v7
    };

    // Align Cout to y-axis
    Vec3 target_cout_normal(0, 1, 0);
    Vec3 target_cout_x_axis(1, 0, 0);
    Transformation transform1(cout_normal, cout_v0, cout_v1, cout_v2,
        target_cout_normal, target_cout_x_axis, cout_vertices);
    Mat4 T1 = transform1.getHomogeneousMatrix();
    Mat3 R1 = transform1.getRotation();
    Vec3 t1 = transform1.getTranslation();
    ConvexHull transformed_hull1 = hull1.transform(R1, t1);

    // Prism 2: Cin face at z = 0, normal [0, 0, -1]
    Vec3 cin_normal(0, 0, -1);
    Vec3 cin_v0(m, 0, 0);
    Vec3 cin_v1(0, 0, 0);
    Vec3 cin_v2(0, n, 0);
    std::vector<Vec3> cin_vertices = {
        Vec3(0, 0, 0),   // v0
        Vec3(m, 0, 0),   // v1
        Vec3(m, n, 0),   // v6
        Vec3(0, n, 0)    // v5
    };

    // Align Cin to abut Cout
    Transformation transform2(cin_normal, cin_v0, cin_v1, cin_v2, cin_vertices,
        cout_normal, cout_v0, cout_v1, cout_v2, cout_vertices, spacer);
    Mat4 T2 = transform2.getHomogeneousMatrix();
    Mat3 R2 = transform2.getRotation();
    Vector3 t2 = transform2.getTranslation();
    ConvexHull transformed_hull2 = hull2.transform(R2, t2);

    // Output results
    auto printMatrix = [](const MatX& A) {
        for (int i = 0; i < A.rows(); ++i) {
            for (int j = 0; j < A.cols(); ++j) {
                double val = A(i, j);
                std::cout << (std::abs(val) < 1e-6 ? 0 : val) << " ";
            }
            std::cout << "\n";
        }
        };

    auto printVector = [](const VecX& b) {
        for (int i = 0; i < b.size(); ++i) {
            double val = b(i);
            std::cout << (std::abs(val) < 1e-6 ? 0 : val) << "\n";
        }
        };

    auto printVertices = [](const std::vector<Vec3>& vertices, const std::string& label) {
        std::cout << "\n" << label << ":\n";
        for (size_t i = 0; i < vertices.size(); ++i) {
            std::cout << "Vertex " << i << ": ";
            for (int j = 0; j < 3; ++j) {
                double val = vertices[i][j];
                std::cout << (std::abs(val) < 1e-6 ? 0 : val) << " ";
            }
            std::cout << "\n";
        }
        };

    std::cout << "Prism 1 Transformed A:\n";
    printMatrix(transformed_hull1.getA());
    std::cout << "\nPrism 1 Transformed b:\n";
    printVector(transformed_hull1.getB());
    printVertices(transformed_hull1.getTransformedVertices(T1), "Prism 1 Transformed Vertices");

    std::cout << "\nPrism 2 Transformed A:\n";
    printMatrix(transformed_hull2.getA());
    std::cout << "\nPrism 2 Transformed b:\n";
    printVector(transformed_hull2.getB());
    printVertices(transformed_hull2.getTransformedVertices(T2), "Prism 2 Transformed Vertices");

    // Verify key vertices
    Vec3 cout_v1_transformed = T1.transformPoint(cout_v1);
    std::cout << "\nPrism 1 Cout v1 (Cout(0,0)): ";
    for (int i = 0; i < 3; ++i)
        std::cout << (std::abs(cout_v1_transformed[i]) < 1e-6 ? 0 : cout_v1_transformed[i]) << " ";
    std::cout << "\n";

    Vec3 cin_v1_transformed = T2.transformPoint(cin_v1);
    std::cout << "Prism 2 Cin v1 (Cin(0,0)): ";
    for (int i = 0; i < 3; ++i)
        std::cout << (std::abs(cin_v1_transformed[i]) < 1e-6 ? 0 : cin_v1_transformed[i]) << " ";
    std::cout << "\n";

	return EXIT_SUCCESS;
}
catch (const std::exception& e) {
 
    
    return EXIT_FAILURE;
}