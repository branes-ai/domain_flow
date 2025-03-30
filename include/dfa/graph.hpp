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

    using vertex_id_t = std::size_t;
    using edge_id_t = std::pair<vertex_id_t, vertex_id_t>;

    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    struct edge_id_hash {
        [[nodiscard]] std::size_t operator()(const edge_id_t& key) const {
            size_t seed = 0;
            hash_combine(seed, key.first);
            hash_combine(seed, key.second);

            return seed;
        }
    };

    namespace detail {

        inline std::pair<vertex_id_t, vertex_id_t> make_sorted_pair(vertex_id_t vertex_id_lhs, vertex_id_t vertex_id_rhs) {
            if (vertex_id_lhs < vertex_id_rhs) {
                return std::make_pair(vertex_id_lhs, vertex_id_rhs);
            }
            return std::make_pair(vertex_id_rhs, vertex_id_lhs);
        }

    }  // namespace detail

    template <typename VertexType, typename EdgeType, bool GraphType = DIRECTED_GRAPH>
    class graph {
    public:
        static constexpr bool graph_t = GraphType;
        using vertex_t = VertexType;
        using edge_t = EdgeType;

        using vertices_t = std::unordered_set<vertex_id_t>;

        using vertex_id_to_vertex_t = std::unordered_map<vertex_id_t, VertexType>;
        using edge_id_to_edge_t = std::unordered_map<edge_id_t, edge_t, edge_id_hash>;

        // selectors

        [[nodiscard]] constexpr bool is_directed() const noexcept { return graph_t; }
        [[nodiscard]] std::size_t nrVertices() const noexcept { return m_vertices.size(); }
        [[nodiscard]] std::size_t nrEdges() const noexcept { return m_edges.size(); }

        [[nodiscard]] const vertex_id_to_vertex_t& vertices() const noexcept { return m_vertices; }
        [[nodiscard]] const edge_id_to_edge_t& edges() const noexcept { return m_edges; }

        [[nodiscard]] bool has_vertex(vertex_id_t vertex_id) const noexcept { return m_vertices.contains(vertex_id); }

        [[nodiscard]] bool has_edge(vertex_id_t vertex_id_lhs, vertex_id_t vertex_id_rhs) const noexcept {
            if constexpr (graph_t) {
                return m_edges.contains({ vertex_id_lhs, vertex_id_rhs });
            }
            else {
                return m_edges.contains(detail::make_sorted_pair(vertex_id_lhs, vertex_id_rhs));
            }
        }

        [[nodiscard]] vertex_t& vertex(vertex_id_t vertex_id) {
            return const_cast<vertex_t&>(const_cast<const graph<vertex_t, edge_t, graph_t>*>(this)->vertex(vertex_id));
        }

        [[nodiscard]] const vertex_t& vertex(vertex_id_t vertex_id) const {
            if (!has_vertex(vertex_id)) {
                throw std::invalid_argument{ "Vertex with ID [" + std::to_string(vertex_id) + "] not found in graph." };
            }
            return m_vertices.at(vertex_id);
        }

        [[nodiscard]] edge_t& edge(vertex_id_t lhs, vertex_id_t rhs) {
            return const_cast<graph<vertex_t, edge_t, graph_t>::edge_t&>(
                const_cast<const graph<vertex_t, edge_t, graph_t>*>(this)->edge(lhs, rhs));
        }
        [[nodiscard]] const edge_t& edge(vertex_id_t lhs, vertex_id_t rhs) const {
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

        [[nodiscard]] edge_t& edge(const edge_id_t& edge_id) {
            const auto [lhs, rhs] = edge_id;
            return edge(lhs, rhs);
        }

        [[nodiscard]] const edge_t& edge(const edge_id_t& edge_id) const {
            const auto [lhs, rhs] {edge_id};
            return edge(lhs, rhs);
        }


        [[nodiscard]] vertices_t neighbors(vertex_id_t vertex_id) const {
            if (!m_adjacencyList.contains(vertex_id)) {
                return {};
            }
            return m_adjacencyList.at(vertex_id);
        }

        // Modifiers
        [[nodiscard]] vertex_id_t add_vertex(auto&& vertex) {
            while (has_vertex(m_runningVertexId)) {
                ++m_runningVertexId;
            }
            const auto vertex_id{ m_runningVertexId };
            m_vertices.emplace(vertex_id, std::forward<decltype(vertex)>(vertex));
            return vertex_id;
        }

        vertex_id_t add_vertex(auto&& vertex, vertex_id_t id) {
            if (has_vertex(id)) {
                throw std::invalid_argument{ "Vertex already exists at ID [" + std::to_string(id) + "]" };
            }

            m_vertices.emplace(id, std::forward<decltype(vertex)>(vertex));
            return id;
        }
        void remove_vertex(vertex_id_t vertex_id) {
            if (m_adjacencyList.contains(vertex_id)) {
                for (auto& target_vertex_id : m_adjacencyList.at(vertex_id)) {
                    m_edges.erase({ vertex_id, target_vertex_id });
                }
            }

            m_adjacencyList.erase(vertex_id);
            m_vertices.erase(vertex_id);

            for (auto& [source_vertex_id, neighbors] : m_adjacencyList) {
                neighbors.erase(vertex_id);
                m_edges.erase({ source_vertex_id, vertex_id });
            }
        }

        void add_edge(vertex_id_t lhs, vertex_id_t rhs, auto&& edge) {
            if (!has_vertex(lhs) || !has_vertex(rhs)) {
                throw std::invalid_argument{
                    "Vertices with ID [" + std::to_string(lhs) + "] and [" +
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
        void remove_edge(vertex_id_t lhs, vertex_id_t rhs) {
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
        size_t m_runningVertexId{ 0 };

        vertex_id_to_vertex_t m_vertices{};
        edge_id_to_edge_t m_edges{};

        std::unordered_map<vertex_id_t, vertices_t> m_adjacencyList{};

    };

    template <typename VertexType, typename EdgeType>
    using directed_graph = graph<VertexType, EdgeType, DIRECTED_GRAPH>;

    template <typename VertexType, typename EdgeType>
    using undirected_graph = graph<VertexType, EdgeType, UNDIRECTED_GRAPH>;

}  // namespace sw::dfa

