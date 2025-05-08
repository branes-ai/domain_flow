#include <cmath>
#include <vector>
#include <stdexcept>
#include <iostream>

#include <dfa/vector.hpp>
#include <dfa/matrix.hpp>

namespace sw {
    namespace dfa {


        // ConvexHull class
        template<typename ConstraintCoefficientType = double>
        class ConvexHull {
	        using CCType = ConstraintCoefficientType;
        public:
            ConvexHull(const MatrixX<CCType>& A, const VectorX<CCType>& b) : A_(A), b_(b) {
                if (A_.rows() != b_.size() || A_.cols() != 3)
                    throw std::invalid_argument("Invalid constraint dimensions");
            }
    
            const MatrixX<CCType>& getA() const { return A_; }
            const VectorX<CCType>& getB() const { return b_; }
    
            ConvexHull transform(const Matrix3<CCType>& R, const Vector3<CCType>& t) const {
                MatrixX<CCType> A_new = A_ * R.transpose(); // Rotate normals
                VectorX<CCType> b_new(b_.size());
                for (int i = 0; i < b_.size(); ++i)
                    b_new(i) = b_(i) - A_.row(i).dot(t); // Adjust offsets
                return ConvexHull(A_new, b_new);
            }

        private:
            MatrixX<CCType> A_;
            VectorX<CCType> b_;
        };

        // Transformation class
        template<typename Scalar = float>
        class Transformation {
	        using Vec3 = Vector3<Scalar>;
	        using Mat3 = Matrix3<Scalar>;
        public:
            Transformation(
                const Vec3& face_normal, const Vec3& v1, const Vec3& v2,
                const Vec3& target_normal, const Vec3& target_primary_axis
            ) {
                computeRotation(face_normal, v1, v2, target_normal, target_primary_axis);
            }
    
            Mat3 getRotation() const { return R_; }

        private:
            void computeRotation(
                Vec3 face_normal, const Vec3& v1, const Vec3& v2,
                Vec3 target_normal, Vec3 target_primary_axis
            ) {
                face_normal = face_normal.normalized();
                target_normal = target_normal.normalized();
                target_primary_axis = target_primary_axis.normalized();

                // Step 1: Align face normal with target normal
                Mat3 R1 = computeNormalAlignment(face_normal, target_normal);

                // Step 2: Align primary axis (v2 - v1) with target primary axis
                Vec3 primary_axis = (v2 - v1).normalized();
                Vec3 transformed_primary = R1 * primary_axis;

                // Project both axes onto the plane perpendicular to target_normal
                Mat3 proj = Matrix3() - outerProduct(target_normal, target_normal);
                Vec3 u1 = proj * transformed_primary;
                Vec3 u2 = proj * target_primary_axis;

                if (u1.norm() < 1e-10 || u2.norm() < 1e-10)
                    throw std::runtime_error("Primary axis cannot be parallel to target normal");

                u1 = u1.normalized();
                u2 = u2.normalized();

                // Compute in-plane rotation
                double cos_theta = u1.dot(u2);
                cos_theta = std::max(-1.0, std::min(1.0, cos_theta));
                double theta = std::acos(cos_theta);
                if (u1.cross(u2).dot(target_normal) < 0)
                    theta = -theta;

                // Rotation around target_normal by theta
                Mat3 R2 = rotationMatrix(target_normal, theta);

                R_ = R2 * R1;
            }
    
            Matrix3<Scalar> computeNormalAlignment(const Vector3<Scalar>& n, const Vector3<Scalar>& d) {
                double dot = n.dot(d);
                if (std::abs(dot - 1.0) < 1e-10) return Matrix3();
                if (std::abs(dot + 1.0) < 1e-10) {
                    Matrix3 neg;
                    for (int i = 0; i < 3; ++i)
                        neg(i, i) = -1.0;
                    return neg;
                }
        
                Vector3 axis = n.cross(d).normalized();
                double angle = std::acos(dot);
                return rotationMatrix(axis, angle);
            }
    
            Matrix3<Scalar> rotationMatrix(const Vector3<Scalar>& axis, double angle) const {
                double c = std::cos(angle), s = std::sin(angle);
                double t = 1 - c;
                Vector3<Scalar> a = axis.normalized();
                Matrix3<Scalar> R;
                R(0, 0) = c + a[0] * a[0] * t;
                R(0, 1) = a[0] * a[1] * t - a[2] * s;
                R(0, 2) = a[0] * a[2] * t + a[1] * s;
                R(1, 0) = a[1] * a[0] * t + a[2] * s;
                R(1, 1) = c + a[1] * a[1] * t;
                R(1, 2) = a[1] * a[2] * t - a[0] * s;
                R(2, 0) = a[2] * a[0] * t - a[1] * s;
                R(2, 1) = a[2] * a[1] * t + a[0] * s;
                R(2, 2) = c + a[2] * a[2] * t;
                return R;
            }
    
            Matrix3<Scalar> outerProduct(const Vector3<Scalar>& u, const Vector3<Scalar>& v) const {
                Matrix3<Scalar> result;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        result(i, j) = u[i] * v[j];
                return result;
            }
    
            Matrix3<Scalar> R_;
        };

    }
}
int main() {
    try {
		using namespace sw::dfa;

        using Scalar = float;
		using Vec3 = Vector3<Scalar>;
		using Mat3 = Matrix3<Scalar>;
        Scalar m = 8;
        Scalar n = 4;
        Scalar k = 6;
        // tensor A = m x k
        // tensor B = k x n
        // tensor Cout = m x n
        MatrixX<Scalar> A({
            { 1,  0,  0},  // x <= m
            {-1,  0,  0},  // -x <= -1 -> x >= 0
            { 0,  1,  0},  // y <= n
            { 0, -1,  0},  // -y <= -1 -> y >= 0
            { 0,  0,  1},  // z <= k
            { 0,  0, -1}   // -z <= -1 -> z >= 0
        });
        VectorX<Scalar> b({m, -1, n, -1, k, -1});
        
        ConvexHull hull(A, b);
        
        // Face to align: Cout at the top of the cube (z = k), normal [0, 0, 1]
        Vec3 face_normal(0, 0, 1);
        
        // Two vertices on the face to define primary axis
        // v1 = (0, 0, k) == Cout(0,0)
        // v2 = (0, n, k) == Cout(0,n)
        Vec3 v1(0, 0, k);
        Vec3 v2(0, n, k);
        
        // Target direction for the normal (y-axis)
        Vec3 target_normal(0, 1, 0);
        
        // Target direction for the primary axis (n-axis to global z-axis)
        Vec3 target_primary_axis(0, 0, 1);
        
        // Translation
        Vec3 translation(1, 1, 1);
        
        // Compute transformation
        Transformation transform(face_normal, v1, v2, target_normal, target_primary_axis);
        Mat3 R = transform.getRotation();
        
        // Apply transformation
        ConvexHull<Scalar> transformed_hull = hull.transform(R, translation);
        
        // Output results
        std::cout << "Original A:\n";
        for (int i = 0; i < A.rows(); ++i) {
            for (int j = 0; j < A.cols(); ++j) {
                double val = A(i, j);
                std::cout << (std::abs(val) < 1e-6 ? 0 : val) << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\nOriginal b:\n";
        for (int i = 0; i < b.size(); ++i)
            std::cout << b(i) << "\n";
        std::cout << "\nRotation matrix:\n";
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                double val = R(i, j);
                std::cout << (std::abs(val) < 1e-6 ? 0 : val) << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\nTransformed A:\n";
        for (int i = 0; i < transformed_hull.getA().rows(); ++i) {
            for (int j = 0; j < transformed_hull.getA().cols(); ++j) {
                double val = transformed_hull.getA()(i, j);
                std::cout << (std::abs(val) < 1e-6 ? 0 : val) << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\nTransformed b:\n";
        for (int i = 0; i < transformed_hull.getB().size(); ++i)
            std::cout << transformed_hull.getB()(i) << "\n";

        // Verify vertex transformation
        Vector3<float> v1_transformed = R * v1 + translation;
        std::cout << "\nTransformed v1 (Cout(0,0)): ";
        for (int i = 0; i < 3; ++i)
            std::cout << (std::abs(v1_transformed[i]) < 1e-6 ? 0 : v1_transformed[i]) << " ";
        std::cout << "\n";

        Vector3<float> v2_transformed = R * v2 + translation;
        std::cout << "Transformed v2 (Cout(0,n)): ";
        for (int i = 0; i < 3; ++i)
            std::cout << (std::abs(v2_transformed[i]) < 1e-6 ? 0 : v2_transformed[i]) << " ";
        std::cout << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}