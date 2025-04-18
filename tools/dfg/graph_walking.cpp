#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

int main(int argc, char** argv) {
    using namespace sw::dfa;

    if (argc != 2) {
	    std::cerr << "Usage: " << argv[0] << " <DFG file>\n";
        return EXIT_SUCCESS; // exit with success for CI purposes
    }

    std::string dataFileName{ argv[1] };
    if (!std::filesystem::exists(dataFileName)) {
        // search for the file in the data directory
        try {
            dataFileName = generateDataFile(argv[1]);
            std::cout << "Data: " << dataFileName << std::endl;
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return EXIT_SUCCESS; // exit with success for CI purposes
        }
    }

    DomainFlowGraph dfg(dataFileName); // Deep Learning graph
    dfg.graph.load(dataFileName);

	// walk the graph
	for (const auto& [nodeId, node] : dfg.graph.nodes()) {
		std::cout << "Node ID: " << nodeId << ", Name: " << node.getName() << " Depth: " << node.getDepth() << std::endl;
		std::cout << "  Operator: " << node.getOperator() << std::endl;
		std::cout << "  Inputs: ";
        for (int i = 0; i < node.getNrInputs(); ++i) {
			std::cout << "Operand " << i << " : " << node.getOperandType(i) << '\n';
		}
		std::cout << "  Outputs: ";
        for (int i = 0; i < node.getNrOutputs(); ++i) {
			std::cout << "Result " << i << " : " << node.getResultType(i) << '\n';
		}

	}

    return EXIT_SUCCESS;
}
