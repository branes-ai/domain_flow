#pragma once
#include <list>
#include <optional>
#include <graph/graph.hpp>

namespace sw {
    namespace graph {
        namespace algorithm {
            namespace shortest_path {

                // Forward declaration
                template <typename WEIGHT_T>
                struct graph_path;

                namespace detail {

                    template <typename WeightType>
                    struct path_node {
                        nodeId_t id;
                        WeightType dist_from_start;
                        nodeId_t prev_id;

                        [[nodiscard]] bool operator>(const path_node<WeightType>& other) {
                            return dist_from_start > other.dist_from_start;
                        }
                    };

                    template <typename WeightType>
                    std::optional<graph_path<WeightType>> reconstruct_path(
                        nodeId_t start, nodeId_t end,
                        std::unordered_map<nodeId_t, path_node<WeightType>>& node_info)
                    {
                        if (!node_info.contains(end)) {
                            return std::nullopt;
                        }

                        graph_path<WeightType> path;
                        auto current = end;

                        while (current != start) {
                            path.nodes.push_front(current);
                            current = node_info[current].prev_id;
                        }

                        path.nodes.push_front(start);
                        path.total_weight = node_info[end].dist_from_start;
                        return path;
                    }

                }  // namespace detail

                template <typename WeightType>
                struct graph_path {
                    std::list<nodeId_t> nodes;
                    WeightType total_weight;

                    bool operator==(const graph_path& other) const {
                        return nodes == other.nodes && total_weight == other.total_weight;
                    }
                };

			} // namespace shortest_path
		} // namespace algorithm
	} // namespace graph
}  // namespace sw
