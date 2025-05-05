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

    DomainFlowGraph dfg(dataFileName); // Domain Flow Graph
    dfg.load(dataFileName);

	// the Domain Flow Graph is the raw representation of the graph nodes and their dependencies
	// To work with a DFG, we ask it to generate specific pieces of information.
	// For example, we can ask it to generate the convex hull of the domain of computation
	// for each operator in the graph. The convex hull is a set of constraints that define the
	// domain of computation for the operator.
	// Or we can ask for the index space of the operator, which is a set of points that
	// make up the domain of computation for the operator.

    // Here we are going to generate the index spaces the (sub)graph and report on them.
    dfg.instantiateIndexSpaces();

    // walk the graph, and report on the 3D points that make up the convex hull of the domain of computation
    for (const auto& [nodeId, node] : dfg.graph.nodes()) {
        std::cout << "Node ID: " << nodeId << ", Name: " << node.getName() << " Depth: " << node.getDepth() << std::endl;
        std::cout << "  Operator: " << node.getOperator() << std::endl;

		// for each operator node, report the index space

		auto indexSpace = node.indexSpace();
        if (indexSpace.empty()) continue;

        std::cout << "Index Space\n";
        for (const auto& p : indexSpace.get_points()) {
            std::cout << "Point: " << p << '\n';
        }

    }

    return EXIT_SUCCESS;
}
