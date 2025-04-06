#pragma once
#include <string>
#include <sstream>

#include <memory>
#include <vector>
#include <map>
#include <stack>
#include <queue>
#include <unordered_set>

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <numbers>

namespace sw {
    namespace dfa {


        // Structure to hold SCC analysis results
        struct SCCProperties {
            size_t size;                   // Number of variables in the SCC
            bool hasSelfLoops;             // Whether any variable depends directly on itself
            bool isElementary;             // Whether all variables have the same dimension
            int maxDimension;              // Maximum dimension of any variable
            std::vector<AffineMap<int>> cycles; // Representative dependency cycles
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
    
            //// Graph analysis methods
            //bool isStronglyConnected();
            //std::vector<std::vector<RecurrenceVariable*>> getStronglyConnectedComponents() ;
            //bool hasUniformDependencies() const;
            //// Analyze properties of a specific SCC
            //SCCProperties analyzeSCC(const std::vector<RecurrenceVariable*>& scc);
            //// Analyze all SCCs in the graph
            //std::vector<SCCProperties> analyzeAllSCCs() ;
            //// Get the condensation graph (DAG of SCCs)
            //std::vector<std::pair<int, int>> getCondensationGraph() ;
            //// Find the execution order of SCCs (topological sort)
            //std::vector<int> getExecutionOrder() ;


            // Builder pattern interface
            class Builder {
            private:
                std::unique_ptr<DependencyGraph> graph;
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
                    const AffineMap<int>& map) const {
                    auto* fromVar = graph->variableMap[from];
                    auto* toVar = graph->variableMap[to];

                    // Check if the affine map's dimensions match the variables
                    // This is a placeholder - implementation TBD
                    return true;  // Replace with actual validation
                }

            public:
                Builder() : graph(std::make_unique<DependencyGraph>()) {}

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
                Builder& edge(const std::string& from, const std::string& to, const AffineMap<int>& map) {
         
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
                Builder& edges(const std::vector<std::tuple<std::string, std::string, AffineMap<int>>>& edges) {
                    for (const auto& [from, to, map] : edges) {
                        edge(from, to, map);
                    }
                    return *this;
                }

                // Build and validate the graph
                std::unique_ptr<DependencyGraph> build() {
                    // Validate the graph structure
                    if (graph->variables.empty()) {
                        throw std::runtime_error("Graph has no variables");
                    }

                    // Optional: validate other graph properties
                    // For example, check if all variables are reachable

                    return std::move(graph);
                }

            };
    
            static Builder create() {
                return Builder();
            }




            // Graph analysis methods
            bool isStronglyConnected() {
                auto sccs = getStronglyConnectedComponents();
                return sccs.size() == 1 && sccs[0].size() == variables.size();
            }

            std::vector<std::vector<RecurrenceVariable*>> getStronglyConnectedComponents() {
                // Initialize algorithm data
                std::vector<std::vector<RecurrenceVariable*>> sccs;
                std::stack<RecurrenceVariable*> stack;
                int index = 0;

                // Reset all nodes
                for (const auto& var : variables) {
                    var->index = -1;
                    var->lowlink = -1;
                    var->onStack = false;
                }

                // Find SCCs
                for (const auto& var : variables) {
                    if (var->index == -1) {
                        // Remove constness to modify algorithm metadata
                        tarjanSCC(const_cast<RecurrenceVariable*>(var.get()), index, stack, sccs);
                    }
                }

                return sccs;
            }

            bool hasUniformDependencies() const {
                for (const auto& var : variables) {
                    if (var->getDependencies().size() != 1) {
                        return false;
                    }
                }
                return true;
            }



            // Analyze properties of a specific SCC
            SCCProperties analyzeSCC(const std::vector<RecurrenceVariable*>& scc) {
                SCCProperties props;
                props.size = scc.size();

                // Calculate various properties
                int totalDeps = 0;
                std::unordered_set<int> dimensions;

                for (auto* var : scc) {
                    // Update maximum dimension
                    props.maxDimension = std::max(props.maxDimension, var->getDimension());
                    dimensions.insert(var->getDimension());

                    // Count dependencies within the SCC
                    for (const auto& [dep, _] : var->dependencies) {
                        if (std::find(scc.begin(), scc.end(), dep) != scc.end()) {
                            totalDeps++;
                            if (dep == var) {
                                props.hasSelfLoops = true;
                            }
                        }
                    }
                }

                props.isElementary = dimensions.size() == 1;
                props.averageDependencyDegree = static_cast<double>(totalDeps) / scc.size();
                props.cycles = findCycles(scc);

                return props;
            }

            // Analyze all SCCs in the graph
            std::vector<SCCProperties> analyzeAllSCCs() {
                auto sccs = getStronglyConnectedComponents();
                std::vector<SCCProperties> allProps;
                for (const auto& scc : sccs) {
                    allProps.push_back(analyzeSCC(scc));
                }
                return allProps;
            }

            // Get the condensation graph (DAG of SCCs)
            std::vector<std::pair<int, int>> getCondensationGraph() {
                auto sccs = getStronglyConnectedComponents();
                std::vector<std::pair<int, int>> edges;

                // Map variables to their SCC index
                std::map<RecurrenceVariable*, int> sccIndex;
                for (int i = 0; i < sccs.size(); i++) {
                    for (auto* var : sccs[i]) {
                        sccIndex[var] = i;
                    }
                }

                // Find edges between different SCCs
                for (int i = 0; i < sccs.size(); i++) {
                    std::unordered_set<int> connectedComponents;
                    for (auto* var : sccs[i]) {
                        for (const auto& [dep, _] : var->dependencies) {
                            int targetSCC = sccIndex[dep];
                            if (targetSCC != i) {
                                connectedComponents.insert(targetSCC);
                            }
                        }
                    }
                    for (int target : connectedComponents) {
                        edges.emplace_back(i, target);
                    }
                }

                return edges;
            }


            // Find the execution order of SCCs (topological sort)
            std::vector<int> getExecutionOrder() {
                auto condensation = getCondensationGraph();
                auto sccs = getStronglyConnectedComponents();
                std::vector<int> order;

                // Build adjacency list and in-degree count
                std::vector<std::vector<int>> adj(sccs.size());
                std::vector<int> inDegree(sccs.size(), 0);

                for (const auto& [from, to] : condensation) {
                    adj[from].push_back(to);
                    inDegree[to]++;
                }

                // Perform topological sort using Kahn's algorithm
                std::queue<int> q;
                for (int i = 0; i < sccs.size(); i++) {
                    if (inDegree[i] == 0) {
                        q.push(i);
                    }
                }

                while (!q.empty()) {
                    int curr = q.front();
                    q.pop();
                    order.push_back(curr);

                    for (int next : adj[curr]) {
                        inDegree[next]--;
                        if (inDegree[next] == 0) {
                            q.push(next);
                        }
                    }
                }

                return order;
            }





            ////////////////////////////////////////////////////////////////////////////////////////////////////
            /// visualization methods

              // Generate visualization in specified format
            std::string generateVisualization(VisualizationFormat format = VisualizationFormat::DOT) {
                auto sccs = getStronglyConnectedComponents();

                switch (format) {
                case VisualizationFormat::DOT:
                    return generateDOT(sccs);
                case VisualizationFormat::MERMAID:
                    return generateMermaid(sccs);
                case VisualizationFormat::JSON:
                    return generateJSON(sccs);
                case VisualizationFormat::ASCII:
                    return generateASCII(sccs);
                case VisualizationFormat::HTML:
                    return generateHTML(sccs);
                default:
                    return generateDOT(sccs);
                }
            }

            // Generate DOT format (previous implementation remains...)
            std::string generateDOT(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
                std::stringstream dot;
                dot << "digraph DependencyGraph {\n";

                // Assign colors to different SCCs
                std::map<RecurrenceVariable*, std::string> colorMap;
                const std::vector<std::string> colors = {
                    "lightblue", "lightgreen", "lightpink", "lightyellow",
                    "lightgrey", "lightcoral", "lightsalmon", "lightseagreen"
                };

                for (size_t i = 0; i < sccs.size(); i++) {
                    std::string color = colors[i % colors.size()];
                    for (auto* var : sccs[i]) {
                        colorMap[var] = color;
                    }
                }

                // Add nodes
                for (const auto& var : variables) {
                    dot << "  \"" << var->getName() << "\" [shape=box, style=filled, "
                        << "fillcolor=\"" << colorMap[var.get()] << "\", "
                        << "label=\"" << var->getName() << "\\n(dim=" << var->getDimension() << ")\"];\n";
                }

                // Add edges
                for (const auto& var : variables) {
                    for (const auto& [dep, map] : var->getDependencies()) {
                        dot << "  \"" << var->getName() << "\" -> \"" << dep->getName() << "\" "
                            << "[label=\"" << formatAffineMap(map) << "\"];\n";
                    }
                }

                dot << "}\n";
                return dot.str();
            }

            // Generate Mermaid format
            std::string generateMermaid(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
                std::stringstream mmd;
                mmd << "graph TD\n";

                // Map for tracking SCC clusters
                std::map<RecurrenceVariable*, int> sccMap;
                for (int i = 0; i < sccs.size(); i++) {
                    for (auto* var : sccs[i]) {
                        sccMap[var] = i;
                    }
                }

                // Define subgraphs for each SCC
                for (size_t i = 0; i < sccs.size(); i++) {
                    mmd << "  subgraph SCC" << i << "\n";
                    for (auto* var : sccs[i]) {
                        mmd << "    " << var->getName()
                            << "[" << var->getName() << "<br/>dim="
                            << var->getDimension() << "]\n";
                    }
                    mmd << "  end\n";
                }

                // Add edges
                for (const auto& var : variables) {
                    for (const auto& [dep, map] : var->getDependencies()) {
                        mmd << "  " << var->getName() << " --> |\""
                            << formatAffineMap(map) << "\"| "
                            << dep->getName() << "\n";
                    }
                }

                return mmd.str();
            }

            // Generate JSON format
            std::string generateJSON(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
                std::stringstream json;
                json << "{\n  \"nodes\": [\n";

                // Generate nodes
                bool firstNode = true;
                for (const auto& var : variables) {
                    if (!firstNode) json << ",\n";
                    json << "    {\n"
                        << "      \"id\": \"" << var->getName() << "\",\n"
                        << "      \"dimension\": " << var->getDimension() << ",\n"
                        << "      \"scc\": " << findSCCIndex(var.get(), sccs) << "\n"
                        << "    }";
                    firstNode = false;
                }

                json << "\n  ],\n  \"edges\": [\n";

                // Generate edges
                bool firstEdge = true;
                for (const auto& var : variables) {
                    for (const auto& [dep, map] : var->getDependencies()) {
                        if (!firstEdge) json << ",\n";
                        json << "    {\n"
                            << "      \"source\": \"" << var->getName() << "\",\n"
                            << "      \"target\": \"" << dep->getName() << "\",\n"
                            << "      \"map\": \"" << formatAffineMap(map) << "\"\n"
                            << "    }";
                        firstEdge = false;
                    }
                }

                json << "\n  ]\n}";
                return json.str();
            }

            // Generate ASCII art visualization
            std::string generateASCII(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
                std::stringstream ascii;
                const int maxWidth = 80;

                // Helper function to create box around text
                auto makeBox = [](const std::string& text, int width) {
                    std::string result;
                    result += "+" + std::string(width - 2, '-') + "+\n";
                    result += "|" + text + std::string(width - text.length() - 2, ' ') + "|\n";
                    result += "+" + std::string(width - 2, '-') + "+";
                    return result;
                    };

                // Create layout matrix
                std::vector<std::vector<std::string>> matrix;
                int currentRow = 0;

                // Place SCCs in matrix
                for (const auto& scc : sccs) {
                    std::vector<std::string> row;
                    for (auto* var : scc) {
                        std::string label = var->getName() + "(" +
                            std::to_string(var->getDimension()) + ")";
                        row.push_back(makeBox(label, static_cast<int>(label.length()) + 4));
                    }
                    matrix.push_back(row);
                    currentRow++;
                }

                // Draw the matrix
                for (size_t i = 0; i < matrix.size(); i++) {
                    // Split boxes into lines
                    std::vector<std::vector<std::string>> rowLines;
                    for (const auto& box : matrix[i]) {
                        std::stringstream ss(box);
                        std::string line;
                        std::vector<std::string> lines;
                        while (std::getline(ss, line)) {
                            lines.push_back(line);
                        }
                        rowLines.push_back(lines);
                    }

                    // Print lines of all boxes in the row
                    for (size_t lineIdx = 0; lineIdx < rowLines[0].size(); lineIdx++) {
                        for (const auto& boxLines : rowLines) {
                            ascii << boxLines[lineIdx] << "  ";
                        }
                        ascii << "\n";
                    }
                    ascii << "\n";
                }

                // Draw edges
                for (const auto& var : variables) {
                    for (const auto& [dep, map] : var->getDependencies()) {
                        ascii << var->getName() << " --("
                            << formatAffineMap(map) << ")--> "
                            << dep->getName() << "\n";
                    }
                }

                return ascii.str();
            }

            // Generate HTML/SVG visualization
            std::string generateHTML(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
                using std::cos, std::sin;

                std::stringstream html;
                html << R"(
        <svg width="800" height="600" xmlns="http://www.w3.org/2000/svg">
        <defs>
            <marker id="arrowhead" markerWidth="10" markerHeight="7" refX="9" refY="3.5" orient="auto">
                <polygon points="0 0, 10 3.5, 0 7" fill="#000"/>
            </marker>
        </defs>
        )";

                // Calculate node positions (simple force-directed layout)
                std::map<RecurrenceVariable*, std::pair<double, double>> positions;
                const double radius = 200;
                const double centerX = 400;
                const double centerY = 300;
#if CPP20
				const double PI = std::numbers::pi;
#else
				const double PI = 3.14159265358979323846;
#endif
                for (size_t i = 0; i < variables.size(); i++) {
                    double angle = (2 * PI * i) / variables.size();
                    positions[variables[i].get()] = {
                        centerX + radius * cos(angle),
                        centerY + radius * sin(angle)
                    };
                }

                // Draw edges
                for (const auto& var : variables) {
                    for (const auto& [dep, map] : var->getDependencies()) {
                        auto [x1, y1] = positions[var.get()];
                        auto [x2, y2] = positions[dep];

                        html << "    <line x1=\"" << x1 << "\" y1=\"" << y1
                            << "\" x2=\"" << x2 << "\" y2=\"" << y2
                            << "\" stroke=\"black\" stroke-width=\"1\" "
                            << "marker-end=\"url(#arrowhead)\"/>\n";

                        // Edge label
                        double labelX = (x1 + x2) / 2;
                        double labelY = (y1 + y2) / 2;
                        html << "    <text x=\"" << labelX << "\" y=\"" << labelY
                            << "\" text-anchor=\"middle\" font-size=\"10\">"
                            << formatAffineMap(map) << "</text>\n";
                    }
                }

                // Draw nodes
                for (const auto& var : variables) {
                    auto [x, y] = positions[var.get()];
                    std::string color = "lightblue";

                    // Find SCC index for color
                    int sccIdx = findSCCIndex(var.get(), sccs);
                    const std::vector<std::string> colors = {
                        "#ADD8E6", "#90EE90", "#FFB6C1", "#F0E68C"
                    };
                    color = colors[sccIdx % colors.size()];

                    html << "    <circle cx=\"" << x << "\" cy=\"" << y
                        << "\" r=\"30\" fill=\"" << color << "\" stroke=\"black\"/>\n"
                        << "    <text x=\"" << x << "\" y=\"" << y
                        << "\" text-anchor=\"middle\" dy=\".3em\">"
                        << var->getName() << "</text>\n"
                        << "    <text x=\"" << x << "\" y=\"" << (y + 15)
                        << "\" text-anchor=\"middle\" font-size=\"10\">"
                        << "dim=" << var->getDimension() << "</text>\n";
                }

                html << "</svg>";
                return html.str();
            }

            // Helper function to find SCC index for a variable
            int findSCCIndex(RecurrenceVariable* var,
                const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
                for (int i = 0; i < sccs.size(); i++) {
                    if (std::find(sccs[i].begin(), sccs[i].end(), var) != sccs[i].end()) {
                        return i;
                    }
                }
                return -1;
            }

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
            std::vector<AffineMap<int>> findCycles(const std::vector<RecurrenceVariable*>& scc) const {
                std::vector<AffineMap<int>> cycles;

                // Use DFS to find elementary cycles
                std::function<void(RecurrenceVariable*,
                    std::vector<std::pair<RecurrenceVariable*, AffineMap<int>>>&,
                    std::unordered_set<RecurrenceVariable*>&)>
                    findCyclesDFS = [&](RecurrenceVariable* current,
                        std::vector<std::pair<RecurrenceVariable*, AffineMap<int>>>& path,
                        std::unordered_set<RecurrenceVariable*>& visited) {
                            visited.insert(current);

                            for (const auto& [next, map] : current->dependencies) {
                                if (std::find_if(path.begin(), path.end(), [&](const auto& p) { return p.first == next; }) != path.end()) {
                                    // Found a cycle, compose the affine maps
                                    AffineMap composedMap = map;
                                    for (auto it = path.rbegin(); it != path.rend(); ++it) {
                                        composedMap = composedMap * it->second;
                                    }
                                    cycles.push_back(composedMap);
                                }
                                else if (visited.find(next) == visited.end()) {
                                    path.push_back({ next, map });
                                    findCyclesDFS(next, path, visited);
                                    path.pop_back();
                                }
                            }
                    };

                for (auto* var : scc) {
                    std::vector<std::pair<RecurrenceVariable*, AffineMap<int>>> path;
                    std::unordered_set<RecurrenceVariable*> visited;
                    findCyclesDFS(var, path, visited);
                }

                return cycles;
            }

            // Implementation of Tarjan's algorithm for finding SCCs
            void tarjanSCC(
                RecurrenceVariable* v,
                int& index,
                std::stack<RecurrenceVariable*>& stack,
                std::vector<std::vector<RecurrenceVariable*>>& sccs
            ) {
                // Set the depth index for v to the smallest unused index
                v->index = index;
                v->lowlink = index;
                index++;
                stack.push(v);
                v->onStack = true;

                // Consider successors of v
                for (const auto& [w, _] : v->dependencies) {
                    if (w->index == -1) {
                        // Successor w has not yet been visited; recurse on it
                        tarjanSCC(w, index, stack, sccs);
                        v->lowlink = std::min(v->lowlink, w->lowlink);
                    }
                    else if (w->onStack) {
                        // Successor w is in stack and hence in the current SCC
                        v->lowlink = std::min(v->lowlink, w->index);
                    }
                }

                // If v is a root node, pop the stack and generate an SCC
                if (v->lowlink == v->index) {
                    std::vector<RecurrenceVariable*> scc;
                    RecurrenceVariable* w;
                    do {
                        w = stack.top();
                        stack.pop();
                        w->onStack = false;
                        scc.push_back(w);
                    } while (w != v);
                    sccs.push_back(scc);
                }
            }

	        friend std::ostream& operator<<(std::ostream& os, const DependencyGraph* graph);
        };

        std::ostream& operator<<(std::ostream& os, const DependencyGraph* graph) {
            os << "Dependency Graph:\n";
            for (const auto& var : graph->variables) {
                os << "  " << var->getName() << " (dim=" << var->getDimension() << "): ";
                for (const auto& [dep, map] : var->getDependencies()) {
                    os << dep->getName() << " ";
                }
                os << '\n';
            }
            return os;
        }

   }
}
