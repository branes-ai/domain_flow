#include <dfa/dfa.hpp>

void example() {
    auto graph = DependencyGraph::create()
        .variable("X", 2)
        .variable("Y", 2)
        .edge("X", "Y", AffineMap({1, 0, 0, 1}, {1, 0}))
        .build();

    // Or using the more explicit interface:
    auto& x = graph->createVariable("X").withDimension(2);
    auto& y = graph->createVariable("Y").withDimension(2);
    x.dependsOn(&y, AffineMap({1, 0, 0, 1}, {1, 0}));
}

// The builder will validate:
// - Variable name format
// - Dimension validity
// - No duplicate variables
// - Existence of variables when creating edges
// - Affine map compatibility
// - Overall graph structure	
int main() {
    auto graph = DependencyGraph::create()
        // Add individual variables
        .variable("X", 2)
        .variable("Y", 2)
        .variable("Z", 2)

        // Or add multiple variables at once
        .variables({
            {"A", 3},
            {"B", 3},
            {"C", 3}
            })

        // Add individual edges
        .edge("X", "Y", AffineMap({ 1, 0, 0, 1 }, { 1, 0 }))
        .edge("Y", "Z", AffineMap({ 1, 0, 0, 1 }, { 0, 1 }))

        // Or add multiple edges at once
        .edges({
            {"A", "B", AffineMap({1, 0, 0, 1}, {1, 0})},
            {"B", "C", AffineMap({1, 0, 0, 1}, {0, 1})},
            {"C", "A", AffineMap({1, 0, 0, 1}, {1, 1})}
            })

        .build();



	return EXIT_SUCCESS;
}

/*
Variable Creation (variable method):

Validates variable names (must start with letter/underscore, contain only alphanumeric/underscore)
Ensures positive dimensions
Prevents duplicate variables
Creates and stores the variable in the graph


Edge Creation (edge method):

Validates existence of source and target variables
Checks affine map compatibility with variable dimensions
Adds the dependency to the graph structure


Batch Operations:

variables method for adding multiple variables at once
edges method for adding multiple edges at once


Validation Helpers:

isValidVariableName for name format checking
isValidDimension for dimension validation
isValidAffineMap for checking map compatibility


Build Validation:

Ensures graph is non-empty
Could be extended to check for other properties (e.g., reachability)



The Builder provides:

Fluent interface for easy graph construction
Strong validation at each step
Clear error messages for invalid operations
Memory safety with smart pointers
Support for both individual and batch operations
*/
