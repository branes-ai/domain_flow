#include <iostream>
#include <iomanip>
#include <string>
#include <dfa/dfa.hpp>
#include <dfa/test_graphs/matmul_dfg.hpp>

/*
 * test of a single node DFG modeling a non-batched matmul
 */


int main() {
    using namespace sw::dfa;
    using IndexPointType = int;
    using ConstraintCoefficientType = int;

    std::string A, B, C, Cin, Cout;

    A = "tensor<4x16xf32>";
    B = "tensor<16x4xf32>";
    Cin = "tensor<4x4xf32>";
    Cout = Cin;

    DomainFlowGraph twoInputMatmul = CreateMatmulDFG("non-batched-2-input-matmul", A, B, "", Cout);
	std::cout << twoInputMatmul << '\n';

    DomainFlowGraph threeInputMatmul = CreateMatmulDFG("non-batched-3-input-matmul", A, B, Cin, Cout);
	std::cout << threeInputMatmul << '\n';

    return EXIT_SUCCESS;
}


/*
 Basic idea is that each operator node in the DFG contains a SURE or SARE and a native reference frame
 which represents the embedding that the kernel developer design by organizing the recurrence equations.
 Remember that the SURE embedding is simply the assignment of each Static Single Assignment variable
 instance of the left hand side of a recurrence equation, and the recurrence index is used as the
 index point in the index space that contains a distance measure. This index is also referred to as the
 signature of the operation on the right hand side.

 For example: a matmul operator with 2 inputs A and B and 1 output C, the recurrence equation is

system ( (i,j,k) | 0 <= i < M, 0 <= j < N, 0 <= k < K ) {
    a(i,j,k) = a(i,j-1,k);
	b(i,j,k) = b(i-1,j,k);
    c(i,j,k) = c(i,j,k-1) + a(i,j-1,k) * b(i-1,j,k);
}

P.S. we are not writing 
	c(i,j,k) = c(i,j,k-1) + a(i,j,k) * b(i,j,k);
as that creates two hops for a and b to reach the ALU for c.

This recurrence index provides a natural embedding of the SSA variables, in this specific case, a 3D index space 
defined by the constraints on the triplet (i, j, k).

We can derive any valid schedule by analyzing the dependencies on the right hand side and finding a
separating hyperplane that can separate the rhs variables from the lhs variable.

For the matmul, such a valid scheduling vector would be the hyperplane with normal [1 1 1] creating
a partial order by using the dot product with the recurrence index. Specifically, the dependencies
are 
    a: (i,j,k) <- a(i,j-1,k), dependence vector is [0 1 0]
	b: (i,j,k) <- b(i-1,j,k), dependence vector is [1 0 0]
	c: (i,j,k) <- c(i,j,k-1), dependence vector is [0 0 1]

This scheduling vector will create the relationships: 
	a: (i + j + k) > (i + j - 1 + k) -> (i + j + k - 1)
	b: (i + j + k) > (i - 1 + j + k) -> (i + j + k - 1)
	c: (i + j + k) > (i + j + k - 1) -> (i + j + k - 1)
Which is a valid schedule across the domain of computation.

*/