#include <iostream>
#include <iomanip>
#include <dfa/dfa.hpp>

int main() {
    using IndexPointType = int;
    using ConstraintCoefficientType = int;

    std::vector<Hyperplane<ConstraintCoefficientType>> constraints = {
        {{1, 0}, 0, ConstraintType::GreaterOrEqual}   // x >= 0
        ,{{0, 1}, 0, ConstraintType::GreaterOrEqual}  // y >= 0
        ,{{1, 0}, 5, ConstraintType::LessOrEqual}     // x <= 5
        ,{{0, 1}, 5, ConstraintType::LessOrEqual}     // y <= 5
 //       ,{{1, 1}, 5, ConstraintType::LessOrEqual}      // x + y <= 5
    };
    std::vector<IndexPointType> lower_bounds = {0, 0};
    std::vector<IndexPointType> upper_bounds = {5, 5};

    IndexSpace<IndexPointType, ConstraintCoefficientType> index_space(constraints);

    const std::vector<IndexPoint<IndexPointType>>& points = index_space.get_ssa_points();

    for (const auto& point : points) {
        std::cout << "(";
        for (size_t i = 0; i < point.indices.size(); ++i) {
            std::cout << point.indices[i];
            if (i < point.indices.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << ") ";
    }
    std::cout << std::endl;

    return 0;
}