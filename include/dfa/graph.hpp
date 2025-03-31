#pragma once

#include <stdexcept>
#include <format>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace sw::dfa {

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

    template <typename NodeType, typename EdgeType, bool GraphType = DIRECTED_GRAPH>
    class graph {
    public:
        static constexpr bool graph_t = GraphType;
        using node_t = NodeType;
        using edge_t = EdgeType;

        using vertices_t = std::unordered_set<nodeId_t>;

        using nodeId_to_node_t = std::unordered_map<nodeId_t, NodeType>;
        using edgeId_to_edge_t = std::unordered_map<edgeId_t, edge_t, edge_id_hash>;

        // selectors

        [[nodiscard]] constexpr bool is_directed() const noexcept { return graph_t; }
        [[nodiscard]] std::size_t nrNodes() const noexcept { return m_nodes.size(); }
        [[nodiscard]] std::size_t nrEdges() const noexcept { return m_edges.size(); }

        [[nodiscard]] const nodeId_to_node_t& nodes() const noexcept { return m_nodes; }
        [[nodiscard]] const edgeId_to_edge_t& edges() const noexcept { return m_edges; }

        [[nodiscard]] bool has_node(nodeId_t node_id) const noexcept { return m_nodes.contains(node_id); }

        [[nodiscard]] bool has_edge(nodeId_t node_id_lhs, nodeId_t node_id_rhs) const noexcept {
            if constexpr (graph_t) {
                return m_edges.contains({ node_id_lhs, node_id_rhs });
            }
            else {
                return m_edges.contains(detail::make_sorted_pair(node_id_lhs, node_id_rhs));
            }
        }

        [[nodiscard]] node_t& node(nodeId_t node_id) {
            return const_cast<node_t&>(const_cast<const graph<node_t, edge_t, graph_t>*>(this)->node(node_id));
        }

        [[nodiscard]] const node_t& node(nodeId_t node_id) const {
            if (!has_node(node_id)) {
                throw std::invalid_argument{ "Node with ID [" + std::to_string(node_id) + "] not found in graph." };
            }
            return m_nodes.at(node_id);
        }

        [[nodiscard]] edge_t& edge(nodeId_t lhs, nodeId_t rhs) {
            return const_cast<graph<node_t, edge_t, graph_t>::edge_t&>(
                const_cast<const graph<node_t, edge_t, graph_t>*>(this)->edge(lhs, rhs));
        }
        [[nodiscard]] const edge_t& edge(nodeId_t lhs, nodeId_t rhs) const {
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

        [[nodiscard]] edge_t& edge(const edgeId_t& edge_id) {
            const auto [lhs, rhs] = edge_id;
            return edge(lhs, rhs);
        }

        [[nodiscard]] const edge_t& edge(const edgeId_t& edge_id) const {
            const auto [lhs, rhs] {edge_id};
            return edge(lhs, rhs);
        }


        [[nodiscard]] vertices_t neighbors(nodeId_t node_id) const {
            if (!m_adjacencyList.contains(node_id)) {
                return {};
            }
            return m_adjacencyList.at(node_id);
        }

        // Modifiers
        [[nodiscard]] nodeId_t add_node(auto&& vertex) {
            while (has_node(m_runningNodeId)) {
                ++m_runningNodeId;
            }
            const auto node_id{ m_runningNodeId };
            m_nodes.emplace(node_id, std::forward<decltype(vertex)>(vertex));
            return node_id;
        }

        nodeId_t add_node(auto&& vertex, nodeId_t id) {
            if (has_node(id)) {
                throw std::invalid_argument{ "Node already exists at ID [" + std::to_string(id) + "]" };
            }

            m_nodes.emplace(id, std::forward<decltype(vertex)>(vertex));
            return id;
        }
        void remove_vertex(nodeId_t node_id) {
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

        void add_edge(nodeId_t lhs, nodeId_t rhs, auto&& edge) {
            if (!has_node(lhs) || !has_node(rhs)) {
                throw std::invalid_argument{
                    "Nodes with ID [" + std::to_string(lhs) + "] and [" +
                    std::to_string(rhs) + "] not found in graph." };
            }

            if constexpr (graph_t) {
                m_adjacencyList[lhs].insert(rhs);
                m_edges.emplace(std::make_pair(lhs, rhs),
                    std::forward<decltype(edge)>(edge));
                return;
            }
            else {
                m_adjacencyList[lhs].insert(rhs);
                m_adjacencyList[rhs].insert(lhs);
                m_edges.emplace(detail::make_sorted_pair(lhs, rhs),
                    std::forward<decltype(edge)>(edge));
                return;
            }
        }
        void remove_edge(nodeId_t lhs, nodeId_t rhs) {
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

        std::unordered_map<nodeId_t, vertices_t> m_adjacencyList{};

    };

    template <typename NodeType, typename EdgeType>
    using directed_graph = graph<NodeType, EdgeType, DIRECTED_GRAPH>;

    template <typename NodeType, typename EdgeType>
    using undirected_graph = graph<NodeType, EdgeType, UNDIRECTED_GRAPH>;

}  // namespace sw::dfa

