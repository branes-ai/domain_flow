#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

/*
 * Generating wavefronts
 *
 * A Domain Flow Graph contains one operator per node, and operators are connected to form a pipeline.
 * Each operator is associated with a System of Uniform Recurrence Equations, which can be specified
 * by a Reduced Dependency Graph. The RDG contains all the dependencies among the recurrence variables
 * and by hierarchically identifying the strongly connected components of the graph, we can deduce a
 * valid schedule. A valid schedule is a linear, or piecewise linear, function that partially orders
 * the index points in the index space that represents the domain of computation.
 *
 * When you chain operators, the schedule will get modulated by the arrival time of the input domains.
 * How do you know if and how they might effect each other? 
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

    // generate the index space for the complete graph
	//dfg.generateIndexSpace();

	// analyze the index space and gather valid schedules

    // each operator has a 'native' schedule defined by their internal data dependencies
    // These dependencies are part of the design of the system of recurrence equations
	// and thus are not unique. When we walk the Domain Flow Graph, we key off the operator
	// and select a system of recurrence equations that is a functional implementation
	// of the operator. 
    //
    // Each operator thus has a set of schedules that are governed by these recurrence
    // equations and can be seen as 'inherent' to the algorithm. But as we are data driven
    // the schedule of each internal operation is also impacted by the arrival of
    // input data.

    // 
    Schedule<int> schedule;
    dfg.generateSchedule(schedule);

    // schedule the index space union
    //dfg.schedule(schedule);

    return EXIT_SUCCESS;
}
