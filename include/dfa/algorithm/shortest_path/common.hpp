#pragma once
#include <list>
#include <optional>
#include <dfa/graph.hpp>

namespace sw::dfa::algorithm::shortest_path {

    // Forward declaration
    template <typename WEIGHT_T>
    struct graph_path;

    namespace detail {

        template <typename WeightType>
        struct path_vertex {
            vertex_id_t id;
            WeightType dist_from_start;
            vertex_id_t prev_id;

            [[nodiscard]] bool operator>(const path_vertex<WeightType>& other) {
                return dist_from_start > other.dist_from_start;
            }
        };

        template <typename WeightType>
        std::optional<graph_path<WeightType>> reconstruct_path(
            vertex_id_t start, vertex_id_t end,
            std::unordered_map<vertex_id_t, path_vertex<WeightType>>& vertex_info) 
        {
            if (!vertex_info.contains(end)) {
                return std::nullopt;
            }

            graph_path<WeightType> path;
            auto current = end;

            while (current != start) {
                path.vertices.push_front(current);
                current = vertex_info[current].prev_id;
            }

            path.vertices.push_front(start);
            path.total_weight = vertex_info[end].dist_from_start;
            return path;
        }

    }  // namespace detail

    template <typename WeightType>
    struct graph_path {
        std::list<vertex_id_t> vertices;
        WeightType total_weight;

        bool operator==(const graph_path& other) const {
            return vertices == other.vertices && total_weight == other.total_weight;
        }
    };

}  // namespace sw::dfa::algorithm::shortest_path
