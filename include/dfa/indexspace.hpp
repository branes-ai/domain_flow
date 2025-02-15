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

template<typename IndexPointType = int, typename ConstraintCoefficientType = int>
class IndexSpace {
private:
    std::vector<IndexPoint<IndexPointType>> points;
    std::vector<Hyperplane<ConstraintCoefficientType>> constraints;
    std::vector<IndexPointType> lower_bounds;
    std::vector<IndexPointType> upper_bounds;

public:
    IndexSpace(std::initializer_list<Hyperplane<ConstraintCoefficientType>> c, std::initializer_list<IndexPointType> lb, std::initializer_list<IndexPointType> ub) : constraints(c), lower_bounds(lb), upper_bounds(ub) {
        if (lower_bounds.size() != upper_bounds.size()) {
            throw std::invalid_argument("Lower and upper bound dimensions must match.");
        }

        if(constraints.size() > 0 && constraints[0].normal.size() != lower_bounds.size()){
          throw std::invalid_argument("Constraints and bounds dimensions do not match");
        }
        generate();
    }
    IndexSpace(std::vector<Hyperplane<ConstraintCoefficientType>> c, std::vector<IndexPointType> lb, std::vector<IndexPointType> ub) : constraints(c), lower_bounds(lb), upper_bounds(ub) {
        if (lower_bounds.size() != upper_bounds.size()) {
            throw std::invalid_argument("Lower and upper bound dimensions must match.");
        }

        if (constraints.size() > 0 && constraints[0].normal.size() != lower_bounds.size()) {
            throw std::invalid_argument("Constraints and bounds dimensions do not match");
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