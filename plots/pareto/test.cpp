#include <iostream>
#include <filesystem>
#include <random>
#include <matplot/matplot.h>
#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

/*
* Scenario: take a domain flow graph, generate the speed of light and minimum energy
* and plot that in the graph with a very specific symbol.
* Then generate a set of data points representing CPU configurations,
* then GPU configurations,
* then TPU configurations,
* then FPGA configurations,
* then KPU configurations
* 
* Take a video of this process and share.
 */
int main(int argc, char* argv[]) {
	using namespace matplot;
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
    // dfg.instantiateDomains();
    dfg.instantiateIndexSpaces();  // this sets up the DoCs as well

    // step 2: generate the schedule for each operator so that we know what the orientation is
    // of the concurrency.
    ScheduleVector<int> tau;
    dfg.generateSchedule(tau);
    dfg.applyLinearSchedule(tau);

    // step 3: align faces and wavefronts
    dfg.alignDomainFlow();

    // step 4: report on speed of light results
    // The alignment in a global space allows us to generate a latency in steps, and an 
    // estimate of the energy consumption of data movement and computation.
    dfg.generateSpeedOfLight();

	// find the operator node(s) in the graph
    DataPoint speedOfLight;
    for (const auto& [nodeId, node] : dfg.nodes()) {
        if (node.isOperator()) {
			DataPoint speedOfLight = node.getSpeedOfLight();
            std::cout << "Operator node: " << nodeId << std::endl;
            std::cout << "  Type: " << node.getOperatorType() << std::endl;
            std::cout << "  Latency: " << speedOfLight.first << std::endl;
            std::cout << "  Energy: " << speedOfLight.second << std::endl;
		}
    }

	// Generate the speed of light and minimum energy
	auto minLatency = speedOfLight.first;
	auto minEnergy = speedOfLight.second;

	// generate a set of CPU configurations
//	auto cpuConfigs = dfg.getCpuConfigurations();

	// generate a set of GPU configurations
//	auto gpuConfigs = dfg.getGpuConfigurations();

	//auto x = linspace(0, 3 * pi, 200);
	//auto y = transform(x, [&](double x) {return cos(x) + rand(0, 1);});

	//scatter(x, y);

	//show();


	return EXIT_SUCCESS;
}
