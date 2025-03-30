#pragma once

#include <concepts>
#include <type_traits>

namespace sw::dfa {

    /**
     * @brief Interface for a weighted edge.
     *
     * This is what is stored internally and returned from a weighted graph in
     * order to make sure each edge in a weighted graph has a common interface to
     * extract the weight.
     *
     * @tparam WeightType The type of the weight.
     */
    template <typename WeightType = int>
    class weighted_edge {
     public:
      using weight_t = WeightType;

      virtual ~weighted_edge() = default;

      [[nodiscard]] virtual WeightType weight() const noexcept = 0;
    };

    template <typename derived>
    concept derived_from_weighted_edge =
        std::is_base_of_v<weighted_edge<typename derived::weight_t>, derived>;

    /**
     * Overload set to get the weight from an edge
     */
    template <typename WeightedEdgeType>
      requires derived_from_weighted_edge<WeightedEdgeType>
    [[nodiscard]] inline auto weight(const WeightedEdgeType& edge) { return edge.weight(); }

    template <typename EdgeType>
      requires std::is_arithmetic_v<EdgeType>
    [[nodiscard]] inline EdgeType weight(const EdgeType& edge) { return edge; }

    template <typename EdgeType>
    [[nodiscard]] inline int weight(const EdgeType& /*edge*/) {
        // By default, an edge has unit weight
        return 1;
    }

}  // namespace sw::dfa