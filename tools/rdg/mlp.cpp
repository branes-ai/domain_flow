#include <iostream>
#include <dfa/dfa.hpp>

/*
Multilayer Perceptron (MLP)

An MLP consists of N inputs, M neurons, a bias for each neuron, and an activation function sigma, 
yielding M outputs.

Mathematically

   z = sigma(W^T * x  + b)
*/


int main() {
	using namespace sw::dfa;

	
	// create a reduced dependency graph for matrix multiplication
	MatrixX<int> Eye = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
	VectorX<int> i = { 1, 0, 0 };
	VectorX<int> j = { 0, 1, 0 };
	VectorX<int> k = { 0, 0, 1 };
	AffineMap<int> iDirection(Eye, i), jDirection(Eye, j), kDirection(Eye, k);
	
	auto mlpRDG = DependencyGraph::create()
		.variable("W", 3)
		.variable("x", 1)
		.variable("b", 1)
		.variable("y", 1)
		.variable("z", 1)
		.edge("W", "W", jDirection)
		.edge("x", "x", iDirection)
		.edge("b", "b", kDirection)
		.edge("y", "y", jDirection)
		.edge("z", "z", iDirection)
		.build();

	std::cout << mlpRDG << '\n';

	// std::cout << "Affine Map for A: " << matmulRDG.getAffineMap("A") << '\n';
	// std::cout << "A propagation : " << jDirection << '\n';

	using IndexPointType = int;
	using ConstraintCoefficientType = int;

	/*
	    y = W^T * x + b
		
		iteration spaces i, j, k are aligned to the canonical basis (x, y, z)
		We pick the k direction as the recurrence, or iteration, direction.
		The operator W^T * x is a parallel vector of dot products: 
		     each element of x needs to propagate to every element in a row of the W^T matrix
		With the k direction being the iteration direction, this implies
		that we 'place' the W^T matrix in the [i, 1, k] plane, and the x vector on the [i, 1, 1] line
		system(i,j,k | 0 <= i,k < N, j = 1) {
			WT(i,j,k) = WT(i, j-1, k);    <--- input phase
			x(i,j,k) = x(i-1, j, k);
			(i,j,k) = WT(i, j-1, k) * WT(i-1, j, k) + c(i, j, k-1);
		}
	*/
	std::vector<Hyperplane<ConstraintCoefficientType>> constraints = {
	 {{1, 0, 0}, 0, ConstraintType::GreaterOrEqual}  // x >= 0
	,{{0, 1, 0}, 0, ConstraintType::GreaterOrEqual}  // y >= 0
	,{{0, 0, 1}, 0, ConstraintType::GreaterOrEqual}  // z >= 0
	,{{1, 0, 0}, 5, ConstraintType::LessOrEqual}     // x <= 5
	,{{0, 1, 0}, 5, ConstraintType::LessOrEqual}     // y <= 5
	,{{0, 0, 1}, 5, ConstraintType::LessOrEqual}     // z <= 5
		//,{{1, 1, 1}, 5, ConstraintType::LessOrEqual}     // x + y + z <= 5
	};

	return EXIT_SUCCESS;
}
