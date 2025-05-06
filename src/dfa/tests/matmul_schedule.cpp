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
