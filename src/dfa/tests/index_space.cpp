#include <iostream>
#include <iomanip>
#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;
    using IndexPointType = int;
    using ConstraintCoefficientType = int;

    ConstraintSet<ConstraintCoefficientType> constraints = {
     {{1, 0, 0}, 0, ConstraintType::GreaterOrEqual}  // x >= 0
    ,{{0, 1, 0}, 0, ConstraintType::GreaterOrEqual}  // y >= 0
    ,{{0, 0, 1}, -1, ConstraintType::GreaterThan}    // z >  -1
    ,{{1, 0, 0}, 5, ConstraintType::LessOrEqual}     // x <= 5
    ,{{0, 1, 0}, 5, ConstraintType::LessOrEqual}     // y <= 5
    ,{{0, 0, 1}, 6, ConstraintType::LessThan}        // z < 6
  //,{{1, 1, 1}, 5, ConstraintType::LessOrEqual}     // x + y + z <= 5
    };
 //   std::vector<IndexPointType> lower_bounds = {0, 0};
 //   std::vector<IndexPointType> upper_bounds = {5, 5};

    IndexSpace<ConstraintCoefficientType> index_space(constraints);

    const std::vector<IndexPoint>& points = index_space.get_ssa_points();

    for (const auto& point : points) {
        std::cout << point;
    }
    std::cout << std::endl;

    return 0;
}