#pragma once

#include <stdexcept>
#include <functional>
#include <concepts>
#include <type_traits>
#include <fstream>

#include <format>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace sw {
    namespace graph {

        static constexpr bool DIRECTED_GRAPH = true;
        static constexpr bool UNDIRECTED_GRAPH = !DIRECTED_GRAPH;

        using nodeId_t = std::size_t;
        using edgeId_t = std::pair<nodeId_t, nodeId_t>;

        template <class T>
        inline void hash_combine(std::size_t& seed, const T& v) {
            std::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        struct edge_id_hash {
            [[nodiscard]] std::size_t operator()(const edgeId_t& key) const {
                size_t seed = 0;
                hash_combine(seed, key.first);
                hash_combine(seed, key.second);

                return seed;
            }
        };

        namespace detail {

            inline std::pair<nodeId_t, nodeId_t> make_sorted_pair(nodeId_t lhs, nodeId_t rhs) {
                if (lhs < rhs) {
                    return std::make_pair(lhs, rhs);
                }
                return std::make_pair(rhs, lhs);
            }

        }  // namespace detail

        /// <summary>
        /// A graph edge with a weight
        /// </summary>
        /// <typeparam name="WeightType"></typeparam>
        template <typename WeightType = int>
        class weighted_edge {
        public:
            using weight_t = WeightType;

            virtual ~weighted_edge() = default;

            virtual WeightType weight() const noexcept = 0;
        };

#ifdef SUPPORTS_CONCEPTS
        template <typename derived>
        concept derived_from_weighted_edge = std::is_base_of_v<weighted_edge<typename derived::weight_t>, derived>;

        template <typename WeightedEdgeType>
            requires derived_from_weighted_edge<WeightedEdgeType>
        inline auto weight(const WeightedEdgeType& edge) { return edge.weight(); }

        template <typename EdgeType>
            requires std::is_arithmetic_v<EdgeType>
        inline EdgeType weight(const EdgeType& edge) { return edge; }

        template <typename EdgeType>
        inline int weight(const EdgeType& /*edge*/) {
            // By default, an edge has unit weight
            return 1;
        }
#else
        // Replace concept with SFINAE
        template <typename Derived, typename WeightType = typename Derived::weight_t>
        constexpr std::enable_if_t<std::is_base_of_v<weighted_edge<WeightType>, Derived>, bool>
            is_derived_from_weighted_edge(const Derived&) {
            return true;
        }

        template <typename Derived>
        constexpr std::enable_if_t<!std::is_base_of_v<weighted_edge<typename Derived::weight_t>, Derived>, bool>
            is_derived_from_weighted_edge(const Derived&) {
            return false;
        }

        template <typename WeightedEdgeType>
        typename WeightedEdgeType::weight_t
            weight(const WeightedEdgeType& edge) {
            static_assert(std::is_base_of_v<weighted_edge<typename WeightedEdgeType::weight_t>, WeightedEdgeType>,
                "WeightedEdgeType must derive from weighted_edge");
            return edge.weight();
        }

        template <typename EdgeType>
        std::enable_if_t<std::is_arithmetic_v<EdgeType>, EdgeType>
            weight(const EdgeType& edge) {
            return edge;
        }

        template <typename EdgeType>
        constexpr bool is_derived_from_weighted_edge_v = std::is_base_of_v<weighted_edge<typename EdgeType::weight_t>, EdgeType>;

        template <typename EdgeType>
        std::enable_if_t<!std::is_arithmetic_v<EdgeType> && !is_derived_from_weighted_edge_v<EdgeType>, int>
            weight(const EdgeType& /*edge*/) {
            // By default, an edge has unit weight
            return 1;
        }

#endif

        /// <summary>
        /// A directed or undirected graph consisting of user-defined Nodes and Weighted Edges
        /// </summary>
        /// <typeparam name="NodeType"></typeparam>
        /// <typeparam name="EdgeType"></typeparam>
        /// <typeparam name="GraphType"></typeparam>
        template <typename NodeType, typename EdgeType, bool GraphType = DIRECTED_GRAPH>
        class graph {
        public:
            static constexpr bool graph_t = GraphType;
            using node_t = NodeType;
            using edge_t = EdgeType;

            using nodeSet_t = std::unordered_set<nodeId_t>;

            using nodeId_to_node_t = std::unordered_map<nodeId_t, NodeType>;
            using edgeId_to_edge_t = std::unordered_map<edgeId_t, edge_t, edge_id_hash>;

            // selectors

            constexpr bool is_directed() const noexcept { return graph_t; }
            std::size_t nrNodes() const noexcept { return m_nodes.size(); }
            std::size_t nrEdges() const noexcept { return m_edges.size(); }

            const nodeId_to_node_t& nodes() const noexcept { return m_nodes; }
            const edgeId_to_edge_t& edges() const noexcept { return m_edges; }

            bool has_node(nodeId_t node_id) const noexcept {
                bool bHas = false;
                if (m_nodes.find(node_id) != m_nodes.end()) {
                    bHas = true;
                }
                return bHas;
                // easier in C++20
                // return m_nodes.contains(node_id);
            }

            bool has_edge(nodeId_t node_id_lhs, nodeId_t node_id_rhs) const noexcept {
                if constexpr (graph_t) {
                    return m_edges.contains({ node_id_lhs, node_id_rhs });
                }
                else {
                    return m_edges.contains(detail::make_sorted_pair(node_id_lhs, node_id_rhs));
                }
            }

            std::size_t in_degree(nodeId_t node_id) const {
                if (!has_node(node_id)) {
                    throw std::invalid_argument{ "Node with ID [" + std::to_string(node_id) + "] not found in graph." };
                }
                // For directed graphs, count how many nodes have the target as their neighbor
                if constexpr (graph_t) {
                    std::size_t count = 0;
                    for (const auto& [source_id, targets] : m_adjacencyList) {
                        if (targets.find(node_id) != targets.end()) {
                            ++count;
                        }
                    }
                    return count;
                }
                else {
                    // For undirected graphs, in-degree equals the number of neighbors
                    return neighbors(node_id).size();
                }
            }
            std::size_t out_degree(nodeId_t node_id) const {
                if (!has_node(node_id)) {
                    throw std::invalid_argument{ "Node with ID [" + std::to_string(node_id) + "] not found in graph." };
                }

                // For directed graphs, out-degree is the number of nodes this node points to
                if constexpr (graph_t) {
                    auto it = m_adjacencyList.find(node_id);
                    if (it == m_adjacencyList.end()) {
                        return 0;
                    }
                    return it->second.size();
                }
                else {
                    // For undirected graphs, out-degree equals in-degree which equals the number of neighbors
                    return neighbors(node_id).size();
                }
            }
            // node selectors
            node_t& node(nodeId_t node_id) {
                return const_cast<node_t&>(const_cast<const graph<node_t, edge_t, graph_t>*>(this)->node(node_id));
            }
            const node_t& node(nodeId_t node_id) const {
                if (!has_node(node_id)) {
                    throw std::invalid_argument{ "Node with ID [" + std::to_string(node_id) + "] not found in graph." };
                }
                return m_nodes.at(node_id);
            }

            // edge selectors
            edge_t& edge(nodeId_t lhs, nodeId_t rhs) {
                return const_cast<graph<node_t, edge_t, graph_t>::edge_t&>(
                    const_cast<const graph<node_t, edge_t, graph_t>*>(this)->edge(lhs, rhs));
            }
            const edge_t& edge(nodeId_t lhs, nodeId_t rhs) const {
                if (!has_edge(lhs, rhs)) {
                    throw std::invalid_argument{ "No edge found between vertices [" +
                                                std::to_string(lhs) + "] -> [" +
                                                std::to_string(rhs) + "]." };
                }

                if constexpr (graph_t) {
                    return m_edges.at({ lhs, rhs });
                }
                else {
                    return m_edges.at(detail::make_sorted_pair(lhs, rhs));
                }
            }
            edge_t& edge(const edgeId_t& edge_id) {
                const auto [lhs, rhs] = edge_id;
                return edge(lhs, rhs);
            }
            const edge_t& edge(const edgeId_t& edge_id) const {
                const auto [lhs, rhs] {edge_id};
                return edge(lhs, rhs);
            }

            // node set selectors
            nodeSet_t neighbors(nodeId_t node_id) const {
#ifdef CPP20
                if (!m_adjacencyList.contains(node_id)) {
                    return {};
                }
                return m_adjacencyList.at(node_id);
#else
                auto it = m_adjacencyList.find(node_id);
                if (it == m_adjacencyList.end()) {
                    return {};
                }
                return it->second;  // Return the nodeSet_t associated with node_id
#endif
            }
            std::unordered_map<nodeId_t, nodeSet_t> adjacencyList() const { return m_adjacencyList; }

            // Modifiers
            void clear() {
                m_runningNodeId = 0;
                m_nodes.clear();
                m_edges.clear();
                m_adjacencyList.clear();
            }
            template<typename AddNodeType>
            nodeId_t add_node(AddNodeType&& node) {
                while (has_node(m_runningNodeId)) {
                    ++m_runningNodeId;
                }
                const auto node_id{ m_runningNodeId };
                m_nodes.emplace(node_id, std::forward<AddNodeType>(node));
                return node_id;
            }
            template<typename AddNodeType>
            nodeId_t add_node(AddNodeType&& node, nodeId_t id) {
                if (has_node(id)) {
                    throw std::invalid_argument{ "Node already exists at ID [" + std::to_string(id) + "]" };
                }

                m_nodes.emplace(id, std::forward<AddNodeType>(node));
                return id;
            }
            void del_node(nodeId_t node_id) {
                if (m_adjacencyList.contains(node_id)) {
                    for (auto& target_node_id : m_adjacencyList.at(node_id)) {
                        m_edges.erase({ node_id, target_node_id });
                    }
                }

                m_adjacencyList.erase(node_id);
                m_nodes.erase(node_id);

                for (auto& [source_node_id, neighbors] : m_adjacencyList) {
                    neighbors.erase(node_id);
                    m_edges.erase({ source_node_id, node_id });
                }
            }
            template<typename AddEdgeType>
            void add_edge(nodeId_t lhs, nodeId_t rhs, AddEdgeType&& edge) {
                if (!has_node(lhs) || !has_node(rhs)) {
                    throw std::invalid_argument{
                        "Nodes with ID [" + std::to_string(lhs) + "] and [" +
                        std::to_string(rhs) + "] not found in graph." };
                }

                if constexpr (graph_t) {
                    m_adjacencyList[lhs].insert(rhs);
                    m_edges.emplace(std::make_pair(lhs, rhs), std::forward<AddEdgeType>(edge));
                    return;
                }
                else {
                    m_adjacencyList[lhs].insert(rhs);
                    m_adjacencyList[rhs].insert(lhs);
                    m_edges.emplace(detail::make_sorted_pair(lhs, rhs), std::forward<AddEdgeType>(edge));
                    return;
                }
            }
            void del_edge(nodeId_t lhs, nodeId_t rhs) {
                if constexpr (graph_t) {
                    m_adjacencyList.at(lhs).erase(rhs);
                    m_edges.erase(std::make_pair(lhs, rhs));
                    return;
                }
                else {
                    m_adjacencyList.at(lhs).erase(rhs);
                    m_adjacencyList.at(rhs).erase(lhs);
                    m_edges.erase(detail::make_sorted_pair(lhs, rhs));
                    return;
                }
            }

            // Save the graph to a text file
            void save(const std::string& filename) const {
                std::ofstream ofs(filename);
                if (!ofs) {
                    throw std::runtime_error("Failed to open file for writing: " + filename);
				}
				// Save the graph to the file
				save(ofs);
				if (!ofs.good()) {
					throw std::runtime_error("Error occurred while writing to file: " + filename);
				}
			}
			// Save the graph to an output stream
			void save(std::ostream& ostr) const {
                if (!ostr) {
                    throw std::runtime_error("invalid ostream");
                }

                // Write header
                ostr << (graph_t ? "DIRECTED" : "UNDIRECTED") << "\n";
                ostr << "RUNNING_NODE_ID " << m_runningNodeId << "\n";

                // Write nodes
                ostr << "NODES " << m_nodes.size() << "\n";
                for (const auto& [id, node] : m_nodes) {
                    ostr << "NODE " << id << " : " << node << "\n";
                }

                // Write edges
                ostr << "EDGES " << m_edges.size() << "\n";
                for (const auto& [edge_id, edge] : m_edges) {
                    ostr << "EDGE " << edge_id.first << " -> " << edge_id.second << " : " << edge << "\n";
                }

                // Write adjacency list
                ostr << "ADJACENCY " << m_adjacencyList.size() << "\n";
                for (const auto& [node_id, neighbors] : m_adjacencyList) {
                    ostr << "ADJ " << node_id << " : ";
                    bool first = true;
                    for (const auto& neighbor : neighbors) {
                        if (!first) ostr << ", ";
                        ostr << neighbor;
                        first = false;
                    }
                    ostr << "\n";
                }

                ostr.flush();
                if (!ostr.good()) {
                    throw std::runtime_error("error occurred while writing ostream");
                }
            }

            // Load the graph from a text file
            void load(const std::string& filename) {
                std::ifstream ifs(filename);
                if (!ifs) {
                    throw std::runtime_error("Failed to open file for reading: " + filename);
                }
                load(ifs);
                if (!ifs.good()) {
                    throw std::runtime_error("Error occurred while reading from file: " + filename);
                }
            }
			// Load the graph from an input stream
			void load(std::istream& ifs) {
                // Clear existing graph
                clear();

                std::string line;
                // Read graph type
                std::getline(ifs, line);
                bool is_directed = (line == "DIRECTED");
                if (is_directed != graph_t) {
                    throw std::runtime_error("graph type mismatch in istream");
                }

                // Read running node ID
                std::getline(ifs, line);
                std::string keyword = line.substr(0, 15);
                if (keyword != "RUNNING_NODE_ID") {
                    throw std::runtime_error("invalid file format: missing RUNNING_NODE_ID");
                }
                std::istringstream(line.substr(16)) >> m_runningNodeId;

                // Read nodes
                std::getline(ifs, line);
                if (line.substr(0, 5) != "NODES") {
                    throw std::runtime_error("invalid file format: missing NODES");
                }
                std::size_t num_nodes;
                std::istringstream(line.substr(6)) >> num_nodes;

                for (std::size_t i = 0; i < num_nodes; ++i) {
                    std::getline(ifs, line);
                    if (line.substr(0, 4) != "NODE") {
                        throw std::runtime_error("invalid file format: missing NODE");
                    }
                    std::istringstream iss(line.substr(5));
                    nodeId_t id;
                    NodeType node;
                    char colon;
                    iss >> id >> colon;
                    if (colon != ':') {
                        throw std::runtime_error("invalid node format");
                    }
                    iss >> node;
                    m_nodes.emplace(id, std::move(node));
                }

                // Read edges
                std::getline(ifs, line);
                if (line.substr(0, 5) != "EDGES") {
                    throw std::runtime_error("invalid file format: missing EDGES");
                }
                std::size_t num_edges;
                std::istringstream(line.substr(6)) >> num_edges;

                for (std::size_t i = 0; i < num_edges; ++i) {
                    std::getline(ifs, line);
                    if (line.substr(0, 4) != "EDGE") {
                        throw std::runtime_error("invalid file format: missing EDGE");
                    }
                    std::istringstream iss(line.substr(5));
                    nodeId_t first, second;
                    EdgeType edge;
                    std::string arrow;
                    char colon;
                    iss >> first >> arrow >> second >> colon;
                    if (arrow != "->" || colon != ':') {
                        throw std::runtime_error("Invalid edge format");
                    }
                    iss >> edge;
                    m_edges.emplace(std::make_pair(first, second), std::move(edge));
                }

                // Read adjacency list
                std::getline(ifs, line);
                if (line.substr(0, 9) != "ADJACENCY") {
                    throw std::runtime_error("invalid file format: missing ADJACENCY");
                }
                std::size_t num_adj;
                std::istringstream(line.substr(10)) >> num_adj;

                for (std::size_t i = 0; i < num_adj; ++i) {
                    std::getline(ifs, line);
                    if (line.substr(0, 3) != "ADJ") {
                        throw std::runtime_error("invalid file format: missing ADJ");
                    }
                    std::istringstream iss(line.substr(4));
                    nodeId_t node_id;
                    char colon;
                    iss >> node_id >> colon;
                    if (colon != ':') {
                        throw std::runtime_error("invalid adjacency format");
                    }

                    nodeSet_t neighbors;
                    std::string neighbors_str;
                    std::getline(iss, neighbors_str);
                    std::istringstream niss(neighbors_str);
                    std::string neighbor_str;
                    while (std::getline(niss, neighbor_str, ',')) {
                        std::istringstream niss2(neighbor_str);
                        nodeId_t neighbor;
                        niss2 >> neighbor;
                        neighbors.insert(neighbor);
                    }
                    m_adjacencyList.emplace(node_id, std::move(neighbors));
                }

                if (!ifs.good() && !ifs.eof()) {
                    throw std::runtime_error("error occurred while reading istream");
                }
            }

        private:
            size_t m_runningNodeId{ 0 };

            nodeId_to_node_t m_nodes{};
            edgeId_to_edge_t m_edges{};

            std::unordered_map<nodeId_t, nodeSet_t> m_adjacencyList{};

            template<typename NNodeType, typename EEdgeType, bool GGraphType>
            friend std::ostream& operator<<(std::ostream& ostr, const graph<NNodeType, EEdgeType, GGraphType>& gr);
        };

        template <typename NodeType, typename EdgeType>
        using directed_graph = graph<NodeType, EdgeType, DIRECTED_GRAPH>;

        template <typename NodeType, typename EdgeType>
        using undirected_graph = graph<NodeType, EdgeType, UNDIRECTED_GRAPH>;

        // ostream operator
        template<typename NNodeType, typename EEdgeType, bool GGraphType>
        std::ostream& operator<<(std::ostream& ostr, const graph<NNodeType, EEdgeType, GGraphType>& gr) {
            // Iterate over the graph nodes
            for (auto const& r : gr.m_nodes) {
                nodeId_t nodeId = r.first;
                const auto& op = r.second; // this is the node object as defined by the graph, i.e. <NNodeType>
                // for each node, print its nodeId in the graph and the NodeType content
                ostr << "nodeId: " << nodeId << ", node: " << op;
                // Print the neighbors of the current node to reflect the edges                   
                auto neighbors = gr.neighbors(nodeId); // Get neighbors safely using the neighbors() method
                bool bNeighbor = false;
                for (auto it = neighbors.begin(); it != neighbors.end(); ++it) {
                    bNeighbor = true;
                    if (it == neighbors.begin()) {
                        ostr << " -> ";  // Add arrow before the first neighbor
                    }
                    ostr << *it;
                    if (std::next(it) != neighbors.end()) {
                        ostr << ", ";  // Add separator between neighbors
                    }
                }
                if (!bNeighbor) ostr << " sink";  // Indicate if there are no neighbors
                ostr << '\n';  // Newline after each node and its neighbors
            }
            return ostr;
        }

        template <typename NodeType, typename EdgeType, bool GraphType>
        std::unordered_map<nodeId_t, std::size_t> calculateNodeDepths(const graph<NodeType, EdgeType, GraphType>& gr) {
			// This function calculates the depth of each node in the graph.
            std::unordered_map<nodeId_t, std::size_t> depths;

            // Handle empty graph
            if (gr.nrNodes() == 0) {
                return depths;
            }

            // For undirected graphs, we can't determine dependency direction
            if constexpr (!GraphType) {
                throw std::runtime_error("Node depth calculation is only meaningful for directed graphs");
            }

            // For lambdas to be recursive, they need to be stored in std::function, which supports self-referencing.
            // Helper function to compute depth recursively
            std::function<std::size_t(nodeId_t, std::unordered_set<nodeId_t>&)> compute_depth =
                [&](nodeId_t node_id, std::unordered_set<nodeId_t>& visited) -> std::size_t {
                    // If already calculated, return cached result
                    if (depths.find(node_id) != depths.end()) {
                        return depths[node_id];
                    }

                    // Check for cycles
                    if (visited.find(node_id) != visited.end()) {
                        throw std::runtime_error("Cycle detected in graph at node " + std::to_string(node_id));
                    }
                    visited.insert(node_id);

                    // Get nodes that point to current node (dependencies)
                    std::unordered_set<nodeId_t> dependencies;
                    for (const auto& [source_id, targets] : gr.adjacencyList()) {
                        if (targets.find(node_id) != targets.end()) {
                            dependencies.insert(source_id);
                        }
                    }

                    // Base case: no incoming edges (leaf node)
                    if (dependencies.empty()) {
                        depths[node_id] = 0;
                        visited.erase(node_id);
                        return 0;
                    }

                    // Recursively compute maximum depth from dependencies
                    std::size_t max_depth = 0;
                    for (const auto& dep_id : dependencies) {
                        std::size_t dep_depth = compute_depth(dep_id, visited);
                        max_depth = std::max(max_depth, dep_depth + 1);
                    }

                    depths[node_id] = max_depth;
                    visited.erase(node_id);
                    return max_depth;
            };

            // Calculate depth for all nodes
            std::unordered_set<nodeId_t> visited;
            for (const auto& [node_id, _] : gr.nodes()) {
                if (depths.find(node_id) == depths.end()) {
                    compute_depth(node_id, visited);
                }
            }

            return depths;
        }

    }
}  // namespace sw::graph

