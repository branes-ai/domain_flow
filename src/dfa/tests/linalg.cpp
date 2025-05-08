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

    // Test Case 1: Rotate x-axis to y-axis (90 degrees)
    std::cout << "=== Test Case 1: Rotate [1, 0, 0] to [0, 1, 0] ===\n";
    Vector3<double> source1(1.0, 0.0, 0.0);
    Vector3<double> target1(0.0, 1.0, 0.0);
    Matrix3<double> rotation1 = Matrix3<double>::identity(); // computeRotation(source1, target1);
    Vector3<double> rotated1 = rotation1 * source1;


    // Verify result
    double error1 = (rotated1 - target1).norm();
    std::cout << "Error norm: " << error1 << (error1 < 1e-6 ? " (PASS)" : " (FAIL)") << "\n\n";

    // Test Case 2: Rotate x-axis to -x-axis (180 degrees)
    std::cout << "=== Test Case 2: Rotate [1, 0, 0] to [-1, 0, 0] ===\n";
    Vector3<double> source2(1.0, 0.0, 0.0);
    Vector3<double> target2(-1.0, 0.0, 0.0);
    Matrix3<double> rotation2 = Matrix3<double>::identity(); // computeRotation(source2, target2);
    Vector3<double> rotated2 = rotation2 * source2;


    // Verify result
    double error2 = (rotated2 - target2).norm();
    std::cout << "Error norm: " << error2 << (error2 < 1e-6 ? " (PASS)" : " (FAIL)") << "\n";

    return EXIT_SUCCESS;
}
