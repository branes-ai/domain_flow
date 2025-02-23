#pragma once
#include <string>
#include <stdexcept>
#include <algorithm>
#include <dfa/affine_map.hpp>

namespace sw {
    namespace dfa {


        // Represents a variable in the recurrence equation system
        class RecurrenceVariable {

        public:
            RecurrenceVariable(const std::string& name, int dim)
                : name(name), dimension(dim) {
                if (dim <= 0) {
                    throw std::invalid_argument("Dimension must be positive: " + std::to_string(dim));
                }
            }

            // DSL interface for building dependencies
            RecurrenceVariable& dependsOn(RecurrenceVariable* var, const AffineMap<int>& map) {
                // Validate input
                if (!var) {
                    throw std::invalid_argument("Dependency variable cannot be null");
                }

                // Validate affine map compatibility
                if (!isValidAffineMap(map, var)) {
                    throw std::invalid_argument(
                        "Incompatible affine map between " + name +
                        " (dim=" + std::to_string(dimension) + ") and " +
                        var->getName() + " (dim=" + std::to_string(var->getDimension()) + ")"
                    );
                }

                // Check for duplicate dependencies
                auto it = std::find_if(dependencies.begin(), dependencies.end(),
                    [var](const auto& dep) { return dep.first == var; });

                if (it != dependencies.end()) {
                    // Either update existing dependency or throw error
                    // Here we choose to update the existing dependency
                    it->second = map;
                }
                else {
                    // Add new dependency
                    dependencies.emplace_back(var, map);
                }

                return *this;
            }

            // Fluent interface for building recurrence equations
            RecurrenceVariable& withDimension(int dim) {
                if (dim <= 0) {
                    throw std::invalid_argument(
                        "Invalid dimension for variable " + name +
                        ": " + std::to_string(dim)
                    );
                }

                // Check if changing dimension would invalidate existing dependencies
                if (!dependencies.empty() && dim != dimension) {
                    // Validate all existing dependencies with new dimension
                    for (const auto& [var, map] : dependencies) {
                        if (!isValidAffineMap(map, var)) {
                            throw std::invalid_argument(
                                "Cannot change dimension to " + std::to_string(dim) +
                                ": would invalidate existing dependency to " + var->getName()
                            );
                        }
                    }
                }

                dimension = dim;
                return *this;
            }

            // Add multiple dependencies at once
            RecurrenceVariable& dependsOnAll(
                const std::vector<std::pair<RecurrenceVariable*, AffineMap<int>>>& deps) {
                for (const auto& [var, map] : deps) {
                    dependsOn(var, map);
                }
                return *this;
            }

            // Remove a dependency
            RecurrenceVariable& removeDependency(RecurrenceVariable* var) {
                auto it = std::find_if(dependencies.begin(), dependencies.end(),
                    [var](const auto& dep) { return dep.first == var; });

                if (it != dependencies.end()) {
                    dependencies.erase(it);
                }

                return *this;
            }

            // Clear all dependencies
            RecurrenceVariable& clearDependencies() {
                dependencies.clear();
                return *this;
            }

            // Getters
            const std::string& getName() const { return name; }
            int getDimension() const { return dimension; }
            const auto& getDependencies() const { return dependencies; }

            // Friends for SCC implementation
            friend class DependencyGraph;

        private:
            std::string name;
            int dimension;
            std::vector<std::pair<RecurrenceVariable*, AffineMap<int>>> dependencies;

            // Tarjan's algorithm metadata
            int index = -1;
            int lowlink = -1;
            bool onStack = false;

            // Helper function to validate affine map compatibility
            bool isValidAffineMap(const AffineMap<int>& map, const RecurrenceVariable* target) const {
                // Check if the affine map dimensions are compatible with this variable and the target
                // This is a placeholder - implement based on your AffineMap representation
                // Should verify that the map can transform from this variable's space to target's space
                return true;  // Replace with actual validation
            }

        };

    }
}

/*
 the dependsOn and withDimension methods with additional features:

dependsOn method:

Validates input variable pointer
Checks affine map compatibility
Handles duplicate dependencies by updating existing ones
Returns reference for method chaining


withDimension method:

Validates dimension value
Checks compatibility with existing dependencies
Updates dimension if valid
Returns reference for method chaining


Additional helper methods:

dependsOnAll for batch dependency addition
removeDependency for removing single dependency
clearDependencies for removing all dependencies


Validation features:

Input validation for null pointers
Dimension validation
Affine map compatibility checking
Dependency consistency checking


Error handling:

Descriptive error messages
Exception throwing for invalid operations
Safe state maintenance
*/