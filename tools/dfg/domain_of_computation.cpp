#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <util/data_file.hpp>

namespace sw::dfa {
    void reportConvexHulls(const DomainFlowGraph& dfg) {
        // for all the operators in the subgraph, print the convex hull of the domain of computation
        for (const auto& [nodeId, node] : dfg.graph.nodes()) {
            if (node.isOperator()) {
                std::cout << "Node ID: " << nodeId << ", Name: " << node.getName() << " Depth: " << node.getDepth() << std::endl;
                std::cout << "  Operator: " << node.getOperator() << std::endl;
                // generate the domain of computation information for each node
                std::cout << "Convex Hull\n";
                std::cout << node.convexHull() << '\n';
            }
        }
    }
}

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

    // the Domain Flow Graph is the raw representation of the graph nodes and their dependencies
    // To work with a DFG, we need to associate parallel algorithms to each node
    // and analyze the resulting dynamics. However, representations such as the index space
    // for an operator could potentially be gigabytes of information, so we don't want to
    // expand the graph as a whole. Instead, it is better to work with subgraphs of
	// the graph so that we focus on the information that is relevant to the analysis.

    // Conceptually, is it productive to think of a subgraph having inputs and outputs that originate and terminate in a memory abstraction?
    // What would we lose if we used this abstraction?

    // Say we have an API that allows us to copy a subgraph out of the original
    // we could then work with this graph in all its detail.
    // Finding subgraphs will require that we can identify inputs and outputs programmatically.
    // We have a boolean in the edge data structure to indicate if the edge carries a
    // flow that is coming from memory. And we can mark that dynamically as we are 
    // introducing subgraph cuts.

    // A natural interaction with the full DFG is to ask for regions defined by depth.
    // This collects all the inputs required for executing the graph, and defines
	// the outputs that need to be written back to memory.
	dfg.assignNodeDepths(); // assign depth values to nodes based on their maximum distance from inputs
    dfg.distributeConstants(); // push the constants to a depth that is commensurate to their first use
	// pull a subgraph by defining its starting depth and ending depth
	int startDepth = 0;
	int endDepth = 1;
	DomainFlowGraph subgraph = dfg.subgraph(startDepth, endDepth); // extract a subgraph of the DFG
    // print the subgraph (how are we doing the edges?)
	std::cout << "Subgraph: " << subgraph.getName() << '\n';
	std::cout << "Nodes: " << subgraph.getNrNodes() << '\n';
	std::cout << "Edges: " << subgraph.getNrEdges() << '\n';
	// print the graph
	std::cout << "+-------\n" << subgraph << std::endl;
    
	// now we can expand the subgraph to include the index space for each operator
    // First action: create all the domains of computation for the operators in the subgraph
	std::cout << "\n\nFirst Step: Instantiate the Domains of Computation for the subgraph:\n";
	subgraph.instantiateDomains();  

	// print the convex hull of the domain of computation for a specific node
    std::cout << "\nConvex Hull for node 8:\n" << subgraph.convexHull(8) << '\n';
    std::cout << "\nConvex Hull Point Set for node 8:\n" << subgraph.convexHullPointSet(8) << '\n';
	std::cout << "Tensor Confluences for node 8:\n" << subgraph.confluences(8) << '\n';

    // Second action: create the index space for each operator in the subgraph
    std::cout << "\n\nSecond Step: Instantiate the Index Space for each DoC:\n";
	subgraph.instantiateIndexSpaces(); 
    std::cout << "Constraint Set for node 8:\n" << subgraph.constraints(8) << '\n';

    return EXIT_SUCCESS;
}
