#include <iostream>
#include <dfa/dfa.hpp>

/*
Visualizing the Reduced Dependency Graph (RDG) 

for matrix multiplication as a uniform recurrence equation system.
Express the matrix multiplication C = A × B as set of dot products
between rows of A and columns of B:

For matrices of size N×N, where 0 ≤ i,j,k < N:
    C[i,j] = sum(k=0 to N-1) of A[i,k] × B[k,j]

This can be transformed into a system of affine recurrence equations:

C[i,j,0] = 0
C[i,j,k] = C[i,j,k-1] + A[i,k] × B[k,j]    for k > 0

The RDG shows the following dependencies for each point (i,j,k) in the iteration space:

C[i,j,k] depends on C[i,j,k-1]:         dependence vector: (0,0,1)

This represents the accumulation of partial sums

Spatial dependencies are affine:

C[i,j,k] depends on A[i,k]
C[i,j,k] depends on B[k,j]
These represent the input matrix elements needed for each multiplication

Key characteristics of this RDG:
- All dependencies are uniform (constant distance vectors)
- The temporal dependency forms a recurrence along the k dimension
- The spatial dependencies show how elements from matrices A and B are accessed
- The graph clearly shows the local nature of the computation at each point

This RDG can be used to:
- Analyze parallelization opportunities
- Determine optimal scheduling
- Identify data dependencies that might affect performance
- Guide loop transformation strategies
*/


int main() {
	using namespace sw::dfa;

	// create a reduced dependency graph for matrix multiplication
	MatrixX<int> Eye = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
	VectorX<int> i = { 1, 0, 0 };
	VectorX<int> j = { 0, 1, 0 };
	VectorX<int> k = { 0, 0, 1 };
	AffineMap<int> iDirection(Eye, i), jDirection(Eye, j), kDirection(Eye, k);
	
	/*
		C = A * B

		iteration spaces i, j, k are aligned to the canonical basis (x, y, z)
		We pick the k direction as the recurrence, or iteration, direction.
		The operator A * B is a parallel matrix of dot products:
			 each element of a column of B needs to propagate to every element in a row of the A matrix
		With the k direction being the iteration direction, this implies
		that we 'place' the C matrix in the [i, j, 1] plane, and the dot products 'iterate' in the k+1 direction.
		We need to place the A matrix in the [i, 1, k] plane, 
			aligning the rows of A in the k direction, 
			and the columns of A in the i direction.
		and we need to place the B matrix in the [1, j, k] plane,
			aligning the colums of B in the k direction,
			and the rows of B in the j direction.
		With this organization, the dot product calculating c[1,1] = A[1,k]*B[k,1] 
		as the first row of A, and the first column of B are already present in the [1, 1, k] vector
		allowing the dot product to be a recurrence: c[i,i,k] = c[i,j,k-1] + a[1,k]*b[k,1]

		system(i,j,k | 0 <= i,j,k < N) {
			a(i,j,k) = a(i, j-1, k);
			b(i,j,k) = b(i-1, j, k);
			c(i,j,k) = a(i, j-1, k) * b(i-1, j, k) + c(i, j, k-1);
		}
	*/
	auto matmulRDG = DependencyGraph::create()
		.variable("A", 3)
		.variable("B", 3)
		.variable("C", 3)
		.edge("A", "A", jDirection)
		.edge("B", "B", iDirection)
		.edge("C", "C", kDirection)
		.edge("C", "A", jDirection)
		.edge("C", "B", iDirection)
		.build();

	std::cout << matmulRDG << '\n';

	std::cout << "A propagation : " << jDirection << '\n';

	return EXIT_SUCCESS;
}
