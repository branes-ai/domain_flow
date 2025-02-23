#pragma once
#include <stdexcept>
#include <dfa/hyperplane.hpp>

namespace sw {
    namespace dfa {

        // Structure to represent an fully enumerated index space defined by the HyperPlane constraints
        template<typename ConstraintCoefficientType = int>
        class IndexSpace {
            using IndexPointType = int;
        private:
            std::vector<IndexPoint> points;
            std::vector<Hyperplane<ConstraintCoefficientType>> constraints;
            std::vector<IndexPointType> lower_bounds;
            std::vector<IndexPointType> upper_bounds;

            // Helper function to find bounds for a single dimension
            std::pair<IndexPointType, IndexPointType> find_dimension_bounds(size_t dim) {
                if (constraints.empty()) {
                    return { std::numeric_limits<IndexPointType>::lowest() / 2,
                        std::numeric_limits<IndexPointType>::max() / 2 }; // Avoid overflow
                }

                // we can just guard rail this with relatively wide bounds
                // as the enumeration is fast enough for visual domains
                IndexPointType min_bound = -100;
                IndexPointType max_bound = 100;

                return { min_bound, max_bound };
            }

        public:
            IndexSpace(std::vector<Hyperplane<ConstraintCoefficientType>> c)
                : constraints(c) {
                if (constraints.empty()) {
                    throw std::invalid_argument("At least one constraint is required.");
                }
                size_t dimensions = constraints[0].normal.size();
                lower_bounds.resize(dimensions);
                upper_bounds.resize(dimensions);

                // Find bounds for each dimension
                for (size_t dim = 0; dim < dimensions; ++dim) {
                    auto [lb, ub] = find_dimension_bounds(dim);
                    lower_bounds[dim] = lb;
                    upper_bounds[dim] = ub;
                }

                generate();
            }

            const std::vector<IndexPoint>& get_ssa_points() const {
                return points;
            }

        private:
            void generate() {
                int dimensions = static_cast<int>(lower_bounds.size());
                std::vector<IndexPointType> current_point(dimensions);
                for (size_t i = 0; i < dimensions; ++i) {
                    current_point[i] = lower_bounds[i];
                }

                while (true) {
                    bool satisfies_all = true;
                    for (const auto& constraint : constraints) {
                        if (!constraint.is_satisfied(current_point)) {
                            satisfies_all = false;
                            break;
                        }
                    }

                    if (satisfies_all) {
                        points.push_back(IndexPoint(current_point));
                    }

                    int current_dimension = dimensions - 1;
                    while (current_dimension >= 0) {
                        current_point[current_dimension]++;
                        if (current_point[current_dimension] <= upper_bounds[current_dimension]) {
                            break;
                        } else {
                            current_point[current_dimension] = lower_bounds[current_dimension];
                            current_dimension--;
                        }
                    }

                    if (current_dimension < 0) {
                        break; // All points have been generated
                    }
                }
                std::sort(points.begin(), points.end()); // Optional: Sort for consistent ordering
            }
        };

    }
}