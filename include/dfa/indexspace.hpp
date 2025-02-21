#pragma once
#include <stdexcept>
#include <dfa/hyperplane.hpp>

// Structure to represent a single static assignment (SSA) variable index point
template<typename IndexPointType = int>
struct IndexPoint {
    std::vector<IndexPointType> indices; // Indices of a SSA instance of a recurrence variable

    IndexPoint(std::initializer_list<IndexPointType> i) : indices(i) {}
	IndexPoint(const std::vector<IndexPointType>& i) : indices(i) {}

    bool operator==(const IndexPoint& other) const {
        return indices == other.indices;
    }

    bool operator<(const IndexPoint& other) const {
        return indices < other.indices;
    }
};

// Structure to represent an fully enumerated index space defined by the HyperPlane constraints
template<typename IndexPointType = int, typename ConstraintCoefficientType = int>
class IndexSpace {
private:
    std::vector<IndexPoint<IndexPointType>> points;
    std::vector<Hyperplane<ConstraintCoefficientType>> constraints;
    std::vector<IndexPointType> lower_bounds;
    std::vector<IndexPointType> upper_bounds;

    // Helper function to find bounds for a single dimension
    std::pair<IndexPointType, IndexPointType> find_dimension_bounds(size_t dim) {
        if (constraints.empty()) {
            return { std::numeric_limits<IndexPointType>::lowest() / 2,
                   std::numeric_limits<IndexPointType>::max() / 2 }; // Avoid overflow
        }

        IndexPointType min_bound = std::numeric_limits<IndexPointType>::lowest() / 2;
        IndexPointType max_bound = std::numeric_limits<IndexPointType>::max() / 2;

        for (const auto& constraint : constraints) {
            if (constraint.normal[dim] == 0) continue;

            // For each constraint ax + by + ... ≤ c
            // Solve for the current dimension when all other dimensions are set to 0
            // This gives a loose bound that we can tighten later
            ConstraintCoefficientType coeff = constraint.normal[dim];
            ConstraintCoefficientType rhs = constraint.rhs;

            // Adjust RHS based on possible contributions from other dimensions
            for (size_t other_dim = 0; other_dim < constraint.normal.size(); ++other_dim) {
                if (other_dim == dim) continue;

                ConstraintCoefficientType other_coeff = constraint.normal[other_dim];
                if (other_coeff > 0) {
                    rhs -= other_coeff * min_bound; // Use minimum bound for positive coefficients
                }
                else if (other_coeff < 0) {
                    rhs -= other_coeff * max_bound; // Use maximum bound for negative coefficients
                }
            }

            // Calculate bound for current dimension
            IndexPointType bound;
            if (coeff > 0) {
                bound = static_cast<IndexPointType>(rhs / coeff);
                max_bound = std::min(max_bound, bound);
            }
            else if (coeff < 0) {
                bound = static_cast<IndexPointType>(rhs / coeff);
                min_bound = std::max(min_bound, bound);
            }
        }

        // Add some padding to ensure we don't miss any valid points
        min_bound -= 1;
        max_bound += 1;

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

    const std::vector<IndexPoint<IndexPointType>>& get_ssa_points() const {
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
                points.push_back(IndexPoint<IndexPointType>(current_point));
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