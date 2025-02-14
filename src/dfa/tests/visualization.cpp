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

	// Generate visualizations in different formats
	std::string dotViz = graph->generateVisualization(VisualizationFormat::DOT);
	std::string mermaidViz = graph->generateVisualization(VisualizationFormat::MERMAID);
	std::string jsonViz = graph->generateVisualization(VisualizationFormat::JSON);
	std::string asciiViz = graph->generateVisualization(VisualizationFormat::ASCII);
	std::string htmlViz = graph->generateVisualization(VisualizationFormat::HTML);

	return EXIT_SUCCESS;
}
