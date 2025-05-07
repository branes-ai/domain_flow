#include <iostream>
#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;

	Vector<int> v{ 1, 2, 3 };
	Matrix<int> m{ 2, 3 };


	ConvexHull<int> mm1DoC = make3DBox(8, 32, 4);  // tensor<8x32xf32> * tensor<32x4xf32> -> tensor<8x4xf32>
	ConvexHull<int> mm2DoC = make3DBox(8, 12, 4);  // tensor<8x22xf32> * tensor<22x4xf32> -> tensor<8x4xf32>
	ConvexHull<int> mm3DoC = make3DBox(8, 16, 4);  // tensor<8x16xf32> * tensor<16x4xf32> -> tensor<8x4xf32>


	// chained matmul mm1 + mm2 = E => A*B + C*D = E
	// the Cout face (top face of the box) of mm1 is the same as the Cin face (bottom face of the box) of mm2

	// we want to visualize this as a stream moving from left to right, so we need to reorient the hulls

	DomainFlow df;
	df.addConvexHull(mm1DoC);
	df.addConvexHull(mm2DoC);
	df.addConvexHull(mm3DoC);

    std::cout << df << '\n';

    return EXIT_SUCCESS;
}
