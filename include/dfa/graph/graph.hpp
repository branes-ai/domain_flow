#pragma once

#include <stdexcept>
#include <concepts>
#include <type_traits>

#include <format>
#include <memory>
#include <unordered_map>
#include <unordered_set>


namespace sw {
    namespace dfa {
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

                // Iterate over the unordered_map using an iterator
                for (auto const& r : gr.m_nodes) {
                    nodeId_t nodeId = r.first;
					const auto& op = r.second; // this is the node object as defined by the graph, i.e. <NNodeType>

                    std::cout << "nodeId: " << nodeId << ", operator: " << op << std::endl;
                    // Get neighbors safely using the neighbors() method
                    auto neighbors = gr.neighbors(nodeId);
                    for (auto it = neighbors.begin(); it != neighbors.end(); ++it) {
                        ostr << *it;
                        if (std::next(it) != neighbors.end()) {
                            ostr << ", ";  // Add separator between neighbors
                        }
                    }
                    ostr << "\n";  // Newline after each node and its neighbors
                }
                return ostr;
            }
        }
    }
}  // namespace sw::dfa

