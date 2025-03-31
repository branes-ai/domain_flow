#pragma once
#include <optional>
#include <dfa/graph.hpp>
#include <dfa/algorithm/shortest_path/common.hpp>

namespace sw::dfa::algorithm::shortest_path {

/**
 * @brief calculates the shortest path between one start_vertex and one
 * end_vertex using Dijkstra's algorithm. Works on both weighted as well as
 * unweighted graphs. For unweighted graphs, a unit weight is used for each
 * edge.
 *
 * @param graph The graph to extract shortest path from.
 * @param start_vertex Vertex id where the shortest path should start.
 * @param end_vertex Vertex id where the shortest path should end.
 * @return An optional with the shortest path (list of vertices) if found.
 */
template <typename V, typename E, bool Directed,
          typename WeightType = decltype(weight(std::declval<E>()))>
std::optional<graph_path<WeightType>> dijkstra(const graph<V, E, Directed>& graph, nodeId_t start, nodeId_t end)
{
    using weighted_path_item = detail::path_node<WeightType>;
    using dijkstra_queue_t = std::priority_queue<weighted_path_item, std::vector<weighted_path_item>, std::greater<> >;
    dijkstra_queue_t to_explore{};
    std::unordered_map<nodeId_t, weighted_path_item> node_info;

    node_info[start] = {start, 0, start};
    to_explore.push(node_info[start]);

    while (!to_explore.empty()) {
        auto current{to_explore.top()};
        to_explore.pop();

        if (current.id == end) {
            break;
        }

        for (const auto& neighbor : graph.neighbors(current.id)) {
            WeightType edge_weight = weight(graph.edge(current.id, neighbor));

            if (edge_weight < 0) {
                std::ostringstream error_msg;
                error_msg << "Negative edge weight [" << edge_weight
                            << "] between vertices [" << current.id << "] -> ["
                            << neighbor << "].";
                throw std::invalid_argument{error_msg.str()};
            }

            WeightType distance = current.dist_from_start + edge_weight;

            if (!node_info.contains(neighbor) || distance < node_info[neighbor].dist_from_start) {
                node_info[neighbor] = {neighbor, distance, current.id};
                to_explore.push(node_info[neighbor]);
            }
        }
    }

    return reconstruct_path(start, end, node_info);
}

}  // namespace sw::dfa::algorithm::shortest_path