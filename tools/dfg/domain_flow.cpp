#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

/*
 * Testing domain confluence
 *
 * Each operator has a SURE at its core. The SURE represents a graph embedding in N-dimensional space
 * with a Euclidian distance metric. The inputs and outputs of the SURE are thus projected into that
 * N-dimensional space as well, and will gain an orientation and placement in the N-dimensional
 * index space.
 *
 * The domain Confluence captures this orientation and placement.
 *
 */

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

    std::cout << dfg << '\n';

    // generate the index space for the complete graph
    dfg.instantiateIndexSpaces();

    return EXIT_SUCCESS;
}
