#include <iostream>
#include <iomanip>
#include <string>
#include <dfa/dfa.hpp>
#include <dfa/test_graphs/matmul_dfg.hpp>
#include <util/data_file.hpp>

/*
 * test of a single node DFG modeling a non-batched matmul
 */


int main() {
    using namespace sw::dfa;
    using IndexPointType = int;
    using ConstraintCoefficientType = int;

    std::string graphName, dfgFilename, A, B, C, Cin, Cout;

    A = "tensor<4x16xf32>";
    B = "tensor<16x4xf32>";
    Cin = "tensor<4x4xf32>";
    Cout = Cin;

	graphName = "non-batched-2-input-matmul";
    DomainFlowGraph twoInputMatmul = CreateMatmulDFG(graphName, A, B, "", Cout);
	std::cout << twoInputMatmul << '\n';
    // Save the graph to a file
    dfgFilename = graphName + ".dfg";
    dfgFilename = generateDataOutputFile(std::string("workloads/dfa/") + dfgFilename);  // stick it in the data directory
    twoInputMatmul.save(dfgFilename);

    std::cout << "Saved graph to: " << dfgFilename << std::endl;

	graphName = "non-batched-3-input-matmul";
    DomainFlowGraph threeInputMatmul = CreateMatmulDFG(graphName, A, B, Cin, Cout);
	std::cout << threeInputMatmul << '\n';
    // Save the graph to a file
    dfgFilename = graphName + ".dfg";
    dfgFilename = generateDataOutputFile(std::string("workloads/dfa/") + dfgFilename);  // stick it in the data directory
    threeInputMatmul.save(dfgFilename);

    std::cout << "Saved graph to: " << dfgFilename << std::endl;

    return EXIT_SUCCESS;
}
