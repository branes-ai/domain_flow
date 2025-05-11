#include <cmath>
#include <vector>
#include <stdexcept>
#include <iostream>

#include <dfa/vector.hpp>
#include <dfa/matrix.hpp>
#include <dfa/transformation.hpp>
#include <dfa/convex_hull.hpp>

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
    ConvexHull hull1(A1, b1);

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
    ConvexHull hull2(A2, b2);

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

	// Anchor the first convex hull with C(0,0) at the origin
	// and Cout normal to the positive y-axis, while keeping all coordinates positive.
	// This is accomplished by a simple coordinate transformation:
    //     [ 0, 1, 0]
    // R = [ 0, 0, 1]
	//     [ 1, 0, 0]
    //Vec3 target_cout_normal(0, 1, 0);
    //Vec3 target_cout_x_axis(1, 0, 0);
    Transformation<float> transform1{};
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
    printMatrix(transformed_hull1.constraints());
    std::cout << "\nPrism 1 Transformed b:\n";
    printVector(transformed_hull1.rightHandSide());
	printVertices(hull1.getOriginalVertices(), "Prism 1 Vertices");
    printVertices(transformed_hull1.getTransformedVertices(T1), "Prism 1 Transformed Vertices");

    std::cout << "\nPrism 2 Transformed A:\n";
    printMatrix(transformed_hull2.constraints());
    std::cout << "\nPrism 2 Transformed b:\n";
    printVector(transformed_hull2.rightHandSide());
    printVertices(hull2.getOriginalVertices(), "Prism 2 Vertices");
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