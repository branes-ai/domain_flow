#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <stack>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <queue>
#include <numeric>


// Structure to hold SCC analysis results
struct SCCProperties {
    int size;                       // Number of variables in the SCC
    bool hasSelfLoops;             // Whether any variable depends directly on itself
    bool isElementary;             // Whether all variables have the same dimension
    int maxDimension;              // Maximum dimension of any variable
    std::vector<AffineMap> cycles; // Representative dependency cycles
    double averageDependencyDegree;// Average number of dependencies per variable

    // Constructor with initialization
    SCCProperties() : size(0), hasSelfLoops(false), isElementary(true),
        maxDimension(0), averageDependencyDegree(0.0) {
    }
};

// Enumeration for supported visualization formats
enum class VisualizationFormat {
    DOT,        // GraphViz DOT format
    MERMAID,    // Mermaid diagram format
    JSON,       // JSON format for custom renderers
    ASCII,      // ASCII art visualization
    HTML        // HTML/SVG visualization
};

// Main class representing the reduced dependency graph
class DependencyGraph {
public:
    // DSL interface for building the graph
    // Creates a new variable and adds it to the graph
    RecurrenceVariable& createVariable(const std::string& name) {
        // Validate variable name
        if (!isValidVariableName(name)) {
            throw std::invalid_argument("Invalid variable name: " + name);
        }

        // Check for duplicate variable names
        if (variableMap.find(name) != variableMap.end()) {
            throw std::invalid_argument("Variable already exists: " + name);
        }

        // Create new variable with default dimension of 1
        // Dimension can be changed later using withDimension()
        auto var = std::make_unique<RecurrenceVariable>(name, 1);
        RecurrenceVariable* varPtr = var.get();

        // Add to both containers
        variableMap[name] = varPtr;
        variables.push_back(std::move(var));

        // Return reference for method chaining
        return *varPtr;
    }

    // Overloaded version that takes initial dimension
    RecurrenceVariable& createVariable(const std::string& name, int dimension) {
        // Create variable using base implementation
        RecurrenceVariable& var = createVariable(name);

        // Set dimension if different from default
        if (dimension != 1) {
            var.withDimension(dimension);
        }

        return var;
    }

    // Find variable by name
    RecurrenceVariable* findVariable(const std::string& name) const {
        auto it = variableMap.find(name);
        return (it != variableMap.end()) ? it->second : nullptr;
    }

    // Remove variable from graph
    bool removeVariable(const std::string& name) {
        auto it = variableMap.find(name);
        if (it == variableMap.end()) {
            return false;
        }

        RecurrenceVariable* varPtr = it->second;

        // Remove all dependencies to this variable
        for (const auto& var : variables) {
            var->removeDependency(varPtr);
        }

        // Remove from variables vector
        variables.erase(
            std::remove_if(variables.begin(), variables.end(),
                [varPtr](const auto& var) { return var.get() == varPtr; }),
            variables.end()
        );

        // Remove from map
        variableMap.erase(it);

        return true;
    }

    // Get all variable names
    std::vector<std::string> getVariableNames() const {
        std::vector<std::string> names;
        names.reserve(variables.size());
        for (const auto& var : variables) {
            names.push_back(var->getName());
        }
        return names;
    }
    
    // Graph analysis methods
    bool isStronglyConnected();
    std::vector<std::vector<RecurrenceVariable*>> getStronglyConnectedComponents() ;
    bool hasUniformDependencies() const;
    // Analyze properties of a specific SCC
    SCCProperties analyzeSCC(const std::vector<RecurrenceVariable*>& scc);
    // Analyze all SCCs in the graph
    std::vector<SCCProperties> analyzeAllSCCs() ;
    // Get the condensation graph (DAG of SCCs)
    std::vector<std::pair<int, int>> getCondensationGraph() ;
    // Find the execution order of SCCs (topological sort)
    std::vector<int> getExecutionOrder() ;


    // Builder pattern interface
    class Builder {
    private:
        DependencyGraph* graph;
        std::unordered_set<std::string> definedVariables;

        // Validate variable name
        bool isValidVariableName(const std::string& name) const {
            if (name.empty()) return false;

            // First character must be a letter or underscore
            if (!std::isalpha(name[0]) && name[0] != '_') return false;

            // Rest can be letters, numbers, or underscores
            return std::all_of(name.begin() + 1, name.end(),
                [](char c) { return std::isalnum(c) || c == '_'; });
        }

        // Validate dimension
        bool isValidDimension(int dimension) const {
            return dimension > 0;
        }

        // Validate affine map compatibility
        bool isValidAffineMap(const std::string& from, const std::string& to,
            const AffineMap& map) const {
            auto* fromVar = graph->variableMap[from];
            auto* toVar = graph->variableMap[to];

            // Check if the affine map's dimensions match the variables
            // This is a placeholder - implement based on your AffineMap representation
            return true;  // Replace with actual validation
        }

    public:
        Builder() : graph(new DependencyGraph()) {}

        // Add a variable to the graph
        Builder& variable(const std::string& name, int dimension) {
            // Validate input
            if (!isValidVariableName(name)) {
                throw std::invalid_argument("Invalid variable name: " + name);
            }
            if (!isValidDimension(dimension)) {
                throw std::invalid_argument("Invalid dimension for variable " +
                    name + ": " + std::to_string(dimension));
            }
            if (definedVariables.find(name) != definedVariables.end()) {
                throw std::invalid_argument("Variable already defined: " + name);
            }

            // Create and store the variable
            auto var = std::make_unique<RecurrenceVariable>(name, dimension);
            graph->variableMap[name] = var.get();
            graph->variables.push_back(std::move(var));
            definedVariables.insert(name);

            return *this;
        }

        // Add an edge (dependency) between variables
        Builder& edge(const std::string& from, const std::string& to,
            const AffineMap& map) {
            // Validate variables exist
            if (definedVariables.find(from) == definedVariables.end()) {
                throw std::invalid_argument("Source variable not defined: " + from);
            }
            if (definedVariables.find(to) == definedVariables.end()) {
                throw std::invalid_argument("Target variable not defined: " + to);
            }

            // Validate affine map compatibility
            if (!isValidAffineMap(from, to, map)) {
                throw std::invalid_argument(
                    "Incompatible affine map between " + from + " and " + to);
            }

            // Add the dependency
            auto* fromVar = graph->variableMap[from];
            auto* toVar = graph->variableMap[to];
            fromVar->dependencies.push_back({ toVar, map });

            return *this;
        }

        // Add multiple variables at once
        Builder& variables(const std::vector<std::pair<std::string, int>>& vars) {
            for (const auto& [name, dim] : vars) {
                variable(name, dim);
            }
            return *this;
        }

        // Add multiple edges at once
        Builder& edges(const std::vector<std::tuple<std::string, std::string, AffineMap>>& edges) {
            for (const auto& [from, to, map] : edges) {
                edge(from, to, map);
            }
            return *this;
        }

        // Build and validate the graph
        DependencyGraph* build() {
            // Validate the graph structure
            if (graph->variables.empty()) {
                throw std::runtime_error("Graph has no variables");
            }

            // Optional: validate other graph properties
            // For example, check if all variables are reachable

            return graph;
        }

        ~Builder() {
            if (graph) delete graph;
        }
    };
    
    static Builder create() {
        return Builder();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
	/// visualization methods

    // Generate visualization in specified format
    std::string generateVisualization(VisualizationFormat format) ;

    // Generate DOT format (previous implementation remains...)
    std::string generateDOT(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const;

    // Generate Mermaid format
    std::string generateMermaid(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const;

    // Generate JSON format
    std::string generateJSON(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const;

    // Generate ASCII art visualization
    std::string generateASCII(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const;

    // Generate HTML/SVG visualization
    std::string generateHTML(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const;

    // Helper function to find SCC index for a variable
    int findSCCIndex(RecurrenceVariable* var,
        const std::vector<std::vector<RecurrenceVariable*>>& sccs) const;



private:
    std::vector<std::unique_ptr<RecurrenceVariable>> variables;
    std::map<std::string, RecurrenceVariable*> variableMap;

    // Helper method to validate variable name
    bool isValidVariableName(const std::string& name) const {
        if (name.empty()) return false;

        // First character must be a letter or underscore
        if (!std::isalpha(name[0]) && name[0] != '_') return false;

        // Rest can be letters, numbers, or underscores
        return std::all_of(name.begin() + 1, name.end(),
            [](char c) { return std::isalnum(c) || c == '_'; });
    }

    // Helper method to find cycles in an SCC
    std::vector<AffineMap> findCycles(const std::vector<RecurrenceVariable*>& scc) const;

    // Implementation of Tarjan's algorithm for finding SCCs
    void tarjanSCC(
        RecurrenceVariable* v,
        int& index,
        std::stack<RecurrenceVariable*>& stack,
        std::vector< std::vector<RecurrenceVariable*> >& sccs);
};
