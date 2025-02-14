#include <iostream>
#include <iomanip>
#include <dfa/dfa.hpp>

int main() {
	auto graph = DependencyGraph::create()
        .variable("X", 2)
        .variable("Y", 2)
        .variable("Z", 2)
        .edge("X", "Y", AffineMap({1, 0, 0, 1}, {1, 0}))
        .edge("Y", "Z", AffineMap({1, 0, 0, 1}, {0, 1}))
        .edge("Z", "X", AffineMap({1, 0, 0, 1}, {1, 1}))
        .build();

    // Analyze SCCs
    auto sccs = graph->getStronglyConnectedComponents();
    auto properties = graph->analyzeAllSCCs();
    for (const auto& prop : properties) {
        std::cout << "SCC Size: " << prop.size << "\n"
                  << "Elementary: " << prop.isElementary << "\n"
                  << "Has Self-loops: " << prop.hasSelfLoops << "\n"
                  << "Max Dimension: " << prop.maxDimension << "\n"
                  << "Avg Dependencies: " << prop.averageDependencyDegree << "\n";
    }

    // Get execution order
    auto order = graph->getExecutionOrder();

    // Generate visualization
    auto vizFormat = VisualizationFormat::DOT;
    std::string dot = graph->generateVisualization(vizFormat);
    // Save dot to file or use with GraphViz
    std::cout << dot << std::endl;

    return EXIT_SUCCESS;
}
