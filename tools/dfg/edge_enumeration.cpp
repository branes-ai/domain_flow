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
    dfg.load(dataFileName);

    // std::cout << dfg << '\n';

    // walk the nodes
    for (const auto& [nodeId, node] : dfg.graph.nodes()) {
        
        std::cout << "nodeID: " << nodeId << ", Name " << node.getName() << '\n';

    }

	// walk the edges, and connect the outputs of one node to the inputs of another
	for (const auto& [edgeId, edge] : dfg.graph.edges()) {

        std::cout << "src Node: " << edgeId.first << " output Slot: " << edge.srcSlot << " dstNode: " << edgeId.second << " input Slot: " << edge.dstSlot << '\n';

    }

    return EXIT_SUCCESS;
}
