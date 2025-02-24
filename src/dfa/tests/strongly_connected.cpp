#include <iostream>
#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;
    auto graph = DependencyGraph::create()
        .variable("X", 2)
        .variable("Y", 2)
        .variable("Z", 2)
        //.edge("X", "Y", AffineMap({1, 0, 0, 1}, {1, 0}))
        //.edge("Y", "Z", AffineMap({1, 0, 0, 1}, {0, 1}))
        //.edge("Z", "X", AffineMap({1, 0, 0, 1}, {1, 1}))
        .build();

    auto sccs = graph->getStronglyConnectedComponents();
    bool isStronglyConnected = graph->isStronglyConnected();
	return EXIT_SUCCESS;
}
