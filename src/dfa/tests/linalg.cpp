#include <iostream>
#include <dfa/dfa.hpp>
#include <dfa/linalg.hpp>

int main() {
    using namespace sw::dfa;

    MatrixX<int> A = { {1, 0}, {0, 1} };
    MatrixX<int> B(A);
    MatrixX<int> C = A * B;
    std::cout << C << '\n';

    // Matrix3/Vecxtor3 API
	Vector3<int> v1(1, 2, 3);
	Matrix3<int> m1 = Matrix3<int>::identity();
	std::cout << "Matrix: " << m1 << '\n';
	Vector3<int> v2 = m1 * v1;
	std::cout << "Vector after transformation: " << v2 << '\n';

    std::cout << '\n';

    {
        // Test Case 1: Rotate x-axis to y-axis (90 degrees)
        std::cout << "=== Test Case 1: Rotate [1, 0, 0] to [0, 1, 0] ===\n";
        Vector3<double> src(1.0, 0.0, 0.0);
        Vector3<double> tgt(0.0, 1.0, 0.0);
        Matrix3<double> rotate = computeRotationFromTwoVectors(src, tgt);
        std::cout << "Rotation matrix:\n" << rotate << "\n";
        Vector3<double> rotated = rotate * src;
        std::cout << "Rotated vector: " << rotated << "\n";

        // Verify result
        double error = (rotated - tgt).norm();
        std::cout << "Error norm: " << error << (error < 1e-6 ? " (PASS)" : " (FAIL)") << "\n\n";
    }

    {
        // Test Case 2: Rotate z-axis to y-axis (90 degrees)
        std::cout << "=== Test Case 1: Rotate [0, 0, 1] to [0, 1, 0] ===\n";
        Vector3<double> src(0.0, 0.0, 1.0);
        Vector3<double> tgt(0.0, 1.0, 0.0);
        Matrix3<double> rotate = computeRotationFromTwoVectors(src, tgt);
        std::cout << "Rotation matrix:\n" << rotate << "\n";
        Vector3<double> rotated = rotate * src;
		std::cout << "Rotated vector: " << rotated << "\n";

        // Verify result
        double error = (rotated - tgt).norm();
        std::cout << "Error norm: " << error << (error < 1e-6 ? " (PASS)" : " (FAIL)") << "\n\n";
    }

    {
        // Test Case 3: Rotate x-axis to -x-axis (180 degrees)
        std::cout << "=== Test Case 2: Rotate [1, 0, 0] to [-1, 0, 0] ===\n";
        Vector3<double> src(1.0, 0.0, 0.0);
        Vector3<double> tgt(-1.0, 0.0, 0.0);
        Matrix3<double> rotate = computeRotationFromTwoVectors(src, tgt);
        std::cout << "Rotation matrix:\n" << rotate << "\n";
        Vector3<double> rotated = rotate * src;
        std::cout << "Rotated vector: " << rotated << "\n";

        // Verify result
        double error = (rotated - tgt).norm();
        std::cout << "Error norm: " << error << (error < 1e-6 ? " (PASS)" : " (FAIL)") << "\n";
    }

    return EXIT_SUCCESS;
}
