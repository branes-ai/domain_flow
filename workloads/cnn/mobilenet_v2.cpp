#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main() {
    using namespace sw::dfa;

    std::string graphName = "mobilenet_v2";
    DomainFlowGraph mobilenet(graphName);

    // load the graph from a file
    std::string dfgFilename = graphName + ".dfg";
    dfgFilename = generateDataOutputFile(std::string("dl/convolution/") + dfgFilename);  // stick it in the data directory
    try {
        mobilenet.load(dfgFilename);
    }
	catch (const std::exception& e) {
		std::cerr << "Error loading graph: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

    // report on the operator statistics
    reportOperatorStats(mobilenet);
    reportArithmeticComplexity(mobilenet);
    reportNumericalComplexity(mobilenet);

	// std::cout << mobilenet << std::endl;

    return EXIT_SUCCESS;
}
