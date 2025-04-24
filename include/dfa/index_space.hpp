#pragma once
#include <stdexcept>
#include <dfa/hyperplane.hpp>

namespace sw {
    namespace dfa {

        // Structure to represent an fully enumerated index space defined by the HyperPlane constraints
        template<typename ConstraintCoefficientType = int>
        class IndexSpace {
            using IndexPointType = int;
        public:
            // default constructor defines the origin in 3D
			IndexSpace() : constraints{}, lower_bounds{}, upper_bounds{}, points{} {
				lower_bounds.resize(3, 0);
				upper_bounds.resize(3, 0);
				points.push_back(IndexPoint({ 0, 0, 0 }));
			}
            IndexSpace(const ConstraintSet<ConstraintCoefficientType>& c) {
				setConstraints(c);
			}

            // modifiers
			void clear() {
				constraints.clear();
				lower_bounds.clear();
				upper_bounds.clear();
				points.clear();
			}
			void setConstraints(const ConstraintSet<ConstraintCoefficientType>& c) {
				constraints.clear();
				if (c.empty()) {
					std::cerr << "IndexSpace setConstraints: at least one constraint is required\n";
				}
                constraints = c;
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
            // selectors
            const std::vector<IndexPoint>& get_ssa_points() const {
                return points;
            }

        private:
            ConstraintSet<ConstraintCoefficientType> constraints;
            std::vector<IndexPointType> lower_bounds;
            std::vector<IndexPointType> upper_bounds;
            std::vector<IndexPoint> points;

            void generate() {
                int dimensions = static_cast<int>(lower_bounds.size());
                std::vector<IndexPointType> current_point(dimensions);
                for (size_t i = 0; i < dimensions; ++i) {
                    current_point[i] = lower_bounds[i];
                }

                while (true) {
                    bool satisfies_all = true;
                    for (const auto& constraint : constraints.get_constraints()) {
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

            // Helper function to find bounds for a single dimension
            std::pair<IndexPointType, IndexPointType> find_dimension_bounds(size_t dim) {
                if (constraints.empty()) {
                    return { std::numeric_limits<IndexPointType>::lowest() / 2,
                        std::numeric_limits<IndexPointType>::max() / 2 }; // Avoid overflow
                }

                // we can just guard rail this with relatively wide bounds
                // as the enumeration is fast enough for visual domains
                IndexPointType min_bound = -1;
                IndexPointType max_bound = 256;

                return { min_bound, max_bound };
            }
        };

    }
}