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

    // walk the graph, and report on the 3D points that make up the convex hull of the domain of computation
    for (const auto& [nodeId, node] : dfg.graph.nodes()) {
        std::cout << "Node ID: " << nodeId << ", Name: " << node.getName() << " Depth: " << node.getDepth() << std::endl;
        std::cout << "  Operator: " << node.getOperator() << std::endl;

        std::cout << "Convex Hull\n";
        auto pointCloud = node.convexHullPointSet();
        for (const auto& p : pointCloud.pointSet) {
            std::cout << "Point: " << p << '\n';
        }

        std::cout << "Index Space\n";
        //auto indexSpace = node.elaborateIndexSpace();
        //for (const auto& p : indexSpace.get_ssa_points()) {
        //    std::cout << "Point: " << p << '\n';
        //}

    }

    return EXIT_SUCCESS;
}
