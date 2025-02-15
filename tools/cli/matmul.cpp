#include <iostream>
#include <dfa/dfa.hpp>

/*
Let me create a clear visualization of the Reduced Dependency Graph (RDG) 
for matrix multiplication as a uniform recurrence equation system.
Let me first express the matrix multiplication C = A × B as uniform recurrence equations:
For matrices of size N×N, where 0 ≤ i,j,k < N:

C[i,j] = sum(k=0 to N-1) of A[i,k] × B[k,j]

This can be broken down into:
CopyC[i,j] = C[i,j,N]
where:
C[i,j,k] = C[i,j,k-1] + A[i,k] × B[k,j]    for k > 0
C[i,j,0] = 0


The RDG shows the following dependencies for each point (i,j,k) in the iteration space:

Temporal dependency: C[i,j,k] depends on C[i,j,k-1]

This represents the accumulation of partial sums
Distance vector: (0,0,1)


Spatial dependencies:

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

	// create a reduced dependency graph for matrix multiplication
	
	AffineMap<int> iDirection(3,3), jDirection(3,3), kDirection(3,3);
	iDirection.translation({1, 0, 0});
	jDirection.translation({0, 1, 0});
	kDirection.translation({0, 0, 1});
	
	/*
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

	// std::cout << "Affine Map for A: " << matmulRDG.getAffineMap("A") << '\n';
	std::cout << "A propagation : " << jDirection << '\n';

	return EXIT_SUCCESS;
}
