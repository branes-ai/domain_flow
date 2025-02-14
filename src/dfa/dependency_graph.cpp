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
#include <numbers>

#include <dfa/affine_map.hpp>
#include <dfa/recurrence_var.hpp>
#include <dfa/dependency_graph.hpp>


// Graph analysis methods
bool DependencyGraph::isStronglyConnected() {
    auto sccs = getStronglyConnectedComponents();
    return sccs.size() == 1 && sccs[0].size() == variables.size();
}

std::vector<std::vector<RecurrenceVariable*>> DependencyGraph::getStronglyConnectedComponents() {
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

bool DependencyGraph::hasUniformDependencies() const {
	for (const auto& var : variables) {
		if (var->getDependencies().size() != 1) {
			return false;
		}
	}
	return true;
}

// Implementation of Tarjan's algorithm for finding SCCs
void DependencyGraph::tarjanSCC(
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

// Analyze properties of a specific SCC
SCCProperties DependencyGraph::analyzeSCC(const std::vector<RecurrenceVariable*>& scc) {
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
std::vector<SCCProperties> DependencyGraph::analyzeAllSCCs() {
    auto sccs = getStronglyConnectedComponents();
    std::vector<SCCProperties> allProps;
    for (const auto& scc : sccs) {
        allProps.push_back(analyzeSCC(scc));
    }
    return allProps;
}

// Get the condensation graph (DAG of SCCs)
std::vector<std::pair<int, int>> DependencyGraph::getCondensationGraph() {
    auto sccs = getStronglyConnectedComponents();
    std::vector<std::pair<int, int>> edges;

    // Map variables to their SCC index
    std::map<RecurrenceVariable*, int> sccIndex;
    for (size_t i = 0; i < sccs.size(); i++) {
        for (auto* var : sccs[i]) {
            sccIndex[var] = i;
        }
    }

    // Find edges between different SCCs
    for (size_t i = 0; i < sccs.size(); i++) {
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
std::vector<int> DependencyGraph::getExecutionOrder() {
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
    for (size_t i = 0; i < sccs.size(); i++) {
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

// Helper method to find cycles in an SCC
std::vector<AffineMap> DependencyGraph::findCycles(const std::vector<RecurrenceVariable*>& scc) const {
    std::vector<AffineMap> cycles;

    // Use DFS to find elementary cycles
    std::function<void(RecurrenceVariable*,
        std::vector<std::pair<RecurrenceVariable*, AffineMap>>&,
        std::unordered_set<RecurrenceVariable*>&)>
        findCyclesDFS = [&](RecurrenceVariable* current,
            std::vector<std::pair<RecurrenceVariable*, AffineMap>>& path,
            std::unordered_set<RecurrenceVariable*>& visited) {
                visited.insert(current);

                for (const auto& [next, map] : current->dependencies) {
                    if (std::find_if(path.begin(), path.end(),
                        [&](const auto& p) { return p.first == next; }) != path.end()) {
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
        std::vector<std::pair<RecurrenceVariable*, AffineMap>> path;
        std::unordered_set<RecurrenceVariable*> visited;
        findCyclesDFS(var, path, visited);
    }

    return cycles;
}

// Generate visualization in specified format
std::string DependencyGraph::generateVisualization(VisualizationFormat format = VisualizationFormat::DOT) {
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
std::string DependencyGraph::generateDOT(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
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
std::string DependencyGraph::generateMermaid(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
    std::stringstream mmd;
    mmd << "graph TD\n";
    
    // Map for tracking SCC clusters
    std::map<RecurrenceVariable*, int> sccMap;
    for (size_t i = 0; i < sccs.size(); i++) {
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
std::string DependencyGraph::generateJSON(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
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
std::string DependencyGraph::generateASCII(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
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
            row.push_back(makeBox(label, label.length() + 4));
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
std::string DependencyGraph::generateHTML(const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
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
    
    for (size_t i = 0; i < variables.size(); i++) {
        double angle = (2 * std::numbers::pi * i) / variables.size();
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
int DependencyGraph::findSCCIndex(RecurrenceVariable* var, 
                    const std::vector<std::vector<RecurrenceVariable*>>& sccs) const {
    for (size_t i = 0; i < sccs.size(); i++) {
        if (std::find(sccs[i].begin(), sccs[i].end(), var) != sccs[i].end()) {
            return i;
        }
    }
    return -1;
}

