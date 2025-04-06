#pragma once
#include <optional>
#include <functional>
#include <queue>
#include <graph/base/graph.hpp>
#include <graph/algorithm/shortest_path/common.hpp>

namespace sw {
    namespace graph {
        namespace algorithm {
            namespace shortest_path {

                /// <summary>
                /// Dijkstra algorithm to calculate the shortest path between two nodes in a graph.
                /// The shortest path is the path with the smallest weight. 
                /// </summary>
                /// <typeparam name="V"></typeparam>
                /// <typeparam name="E"></typeparam>
                /// <typeparam name="Directed"></typeparam>
                /// <typeparam name="WeightType"></typeparam>
                /// <param name="graph"></param>
                /// <param name="start"></param>
                /// <param name="end"></param>
                /// <returns></returns>
                template <typename V, typename E, bool Directed,
                    typename WeightType = decltype(weight(std::declval<E>()))>
                std::optional<graph_path<WeightType>> dijkstra(const graph<V, E, Directed>& graph, nodeId_t start, nodeId_t end)
                {
                    using weighted_path_item = detail::path_node<WeightType>;

                    std::priority_queue<weighted_path_item, std::vector<weighted_path_item>, std::greater<> > nodes_to_explore{};
                    std::unordered_map<nodeId_t, weighted_path_item> node_info;

                    node_info[start] = { start, 0, start };
                    nodes_to_explore.push(node_info[start]);

                    while (!nodes_to_explore.empty()) {
                        auto current{ nodes_to_explore.top() };
                        nodes_to_explore.pop();

                        if (current.id == end) break;

                        for (const auto& neighbor : graph.neighbors(current.id)) {
                            WeightType edge_weight = weight(graph.edge(current.id, neighbor));

                            if (edge_weight < 0) {
                                std::ostringstream error_msg;
                                error_msg << "Negative edge weight [" << edge_weight
                                    << "] between vertices [" << current.id << "] -> ["
                                    << neighbor << "].";
                                throw std::invalid_argument{ error_msg.str() };
                            }

                            WeightType distance = current.dist_from_start + edge_weight;

                            if (!node_info.contains(neighbor) || distance < node_info[neighbor].dist_from_start) {
                                node_info[neighbor] = { neighbor, distance, current.id };
                                nodes_to_explore.push(node_info[neighbor]);
                            }
                        }
                    }

                    return reconstruct_path(start, end, node_info);
                }

			} // namespace shortest_path
		} // namespace algorithm
	} // namespace graph
}  // namespace sw