#include <iostream>
#include <iomanip>
#include <string>
#include <dfa/dfa.hpp>
#include <dfa/test_graphs/matmul_dfg.hpp>

/*
 * test of a single node DFG modeling a batched matmul
 */

int main() {
    using namespace sw::dfa;
    using IndexPointType = int;
    using ConstraintCoefficientType = int;

    std::string A, B, C, Cin, Cout;

    A = "tensor<6x4x16xf32>";
    B = "tensor<6x16x4xf32>";
    C = "tensor<6x4x4xf32>";
    Cin = Cout = C;

    DomainFlowGraph twoInputMatmul = CreateMatmulDFG("batched-2-input-matmul", A, B, "", Cout);
    std::cout << twoInputMatmul << '\n';

    DomainFlowGraph threeInputMatmul = CreateMatmulDFG("batched-3-input-matmul", A, B, Cin, Cout);
    std::cout << threeInputMatmul << '\n';

    return EXIT_SUCCESS;
}
