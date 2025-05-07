#include <iostream>
#include <dfa/dfa.hpp>
#include <dfa/linalg.hpp>

int main() {
    using namespace sw::dfa;

    Matrix<int> A = { {1, 0}, {0, 1} };
    Matrix<int> B(A);
    Matrix<int> C = A * B;
    std::cout << C << '\n';

    // Matrix3d/Vecxtor3d API
	Vector3d v1(1, 2, 3);
	Matrix3d m1 = Matrix3d::identity();
	std::cout << "Matrix: " << m1 << '\n';
	Vector3d v2 = m1 * v1;
	std::cout << "Vector after transformation: " << v2 << '\n';

    // Test Case 1: Rotate x-axis to y-axis (90 degrees)
    std::cout << "=== Test Case 1: Rotate [1, 0, 0] to [0, 1, 0] ===\n";
    Vector3d source1(1.0, 0.0, 0.0);
    Vector3d target1(0.0, 1.0, 0.0);
    Matrix3d rotation1 = computeRotation(source1, target1);
    Vector3d rotated1 = rotation1 * source1;


    // Helper to print a Vector3d
    auto printVector = [](const Vector3d& v, const std::string& name) {
        std::cout << name << ": [" << std::fixed << std::setprecision(6)
            << v.x << ", " << v.y << ", " << v.z << "]" << std::endl;
    };

    // Helper to print a Matrix3d
    auto printMatrix = [](const Matrix3d& m, const std::string& name) {
        std::cout << name << ":\n";
        for (const auto& row : m.data) {
            std::cout << "[ ";
            for (double val : row) {
                std::cout << std::fixed << std::setprecision(6) << std::setw(12) << val << " ";
            }
            std::cout << "]\n";
        }
    };

    printVector(source1, "Source");
    printVector(target1, "Target");
    printVector(rotated1, "Rotated");
    printMatrix(rotation1, "Rotation Matrix");

    // Verify result
    double error1 = (rotated1 - target1).norm();
    std::cout << "Error norm: " << error1 << (error1 < 1e-6 ? " (PASS)" : " (FAIL)") << "\n\n";

    // Test Case 2: Rotate x-axis to -x-axis (180 degrees)
    std::cout << "=== Test Case 2: Rotate [1, 0, 0] to [-1, 0, 0] ===\n";
    Vector3d source2(1.0, 0.0, 0.0);
    Vector3d target2(-1.0, 0.0, 0.0);
    Matrix3d rotation2 = computeRotation(source2, target2);
    Vector3d rotated2 = rotation2 * source2;

    printVector(source2, "Source");
    printVector(target2, "Target");
    printVector(rotated2, "Rotated");
    printMatrix(rotation2, "Rotation Matrix");

    // Verify result
    double error2 = (rotated2 - target2).norm();
    std::cout << "Error norm: " << error2 << (error2 < 1e-6 ? " (PASS)" : " (FAIL)") << "\n";

    return EXIT_SUCCESS;
}
