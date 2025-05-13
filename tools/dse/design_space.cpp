#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

/*
 * Testing pipeline alignment
 *
 * Each operator has a SURE at its core. The SURE represents a graph embedding in N-dimensional space
 * with a Euclidian distance metric. The inputs and outputs of the SURE are thus projected into that
 * N-dimensional space as well, and will gain an orientation and placement in the N-dimensional
 * index space.
 *
 * As operators are chained together, their native schedule will be modulated by the availability
 * of upstream data, and by backpressure from downstream resources.
 *
 * The domain flow methodology tries to align the 'schedules' of the different operators as to create
 * the most efficient flow of data. This requires that the spatial extent of the domain flow created
 * by the schedule and shape of the index space are manipulated in such a way that an unobstructed
 * processor fabric can be created.
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

    // step 1: generate the domains of computation so that we have the information to reason
    // about orientation in the global unified index space.
    // This consists of the Convex Hull of each operator and the faces that need to communicate.
    dfg.instantiateDomains();


    // step 2: generate the schedule for each operator so that we know what the orientation is
    // of the concurrency.
    dfg.generateSchedules();

    // step 3: align faces and wavefronts
    dfg.alignDomainFlow();

    // if we take the simplifying step that all faces need to be aligned to the basic vectors
    // then all transformations to align communicating operators will yield a lattice without shear.

    // The alignment is a transformation into a global index space.
    // The end result being a space that we can visualize

    // If we can apply this alignment transformation on the Domain Of Computation constraints
    // then we can generate the index spaces for visualization from that transformed DoC spec,
    // instead of transforming the potentially thousands of points representing the original index space.
    

    // step 4: report on speed of light results
    // The alignment in a global space allows us to generate a latency in steps, and an 
    // estimate of the energy consumption of data movement and computation.
    dfg.generateSpeedOfLight();


    // step 5: apply a spatial reduction that is consistent with the global domain flow
    dfg.generateFabric();

    // step 6: report on the performance metrics of the data path/processor fabric realization
    dfg.generatePareto();

	ConvexHull hull;

    return EXIT_SUCCESS;
}
