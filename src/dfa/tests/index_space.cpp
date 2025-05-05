#include <iostream>
#include <iomanip>
#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;
    using IndexPointType = int;
    using ConstraintCoefficientType = int;

    ConstraintSet<ConstraintCoefficientType> constraints = {
     { {1, 0, 0}, 0, ConstraintType::GreaterOrEqual}  // x >= 0
    // { { -1, 0, 0 }, -1, ConstraintType::LessOrEqual }  // -x <= -1
    ,{ {0, 1, 0}, 0, ConstraintType::GreaterOrEqual}  // y >= 0
    ,{ {0, 0, 1}, 0, ConstraintType::GreaterOrEqual}  // z >= 0
		//     ,{{0, 0, 1}, -1, ConstraintType::GreaterThan}  // z > -1      Simplex method does not support strict inequalities
    ,{ {1, 0, 0}, 5, ConstraintType::LessOrEqual}     // x <= 5
    ,{ {0, 1, 0}, 5, ConstraintType::LessOrEqual}     // y <= 5
    ,{ {0, 0, 1}, 5, ConstraintType::LessOrEqual}     // z <= 5
  //  ,{{0, 0, 1}, 6, ConstraintType::LessThan}        // z < 6
    ,{{1, 1, 1}, 6, ConstraintType::LessOrEqual}     // x + y + z <= 6
    };

	// Create an index space from the constraints
    IndexSpace<ConstraintCoefficientType> index_space(constraints);

	// check the bounding box
	std::vector<IndexPointType> lower_bounds;
	std::vector<IndexPointType> upper_bounds;
	index_space.getBounds(lower_bounds, upper_bounds);
	std::cout << "Bounding box:\n";
	for (size_t i = 0; i < lower_bounds.size(); ++i) {
		std::cout << "Dim " << i << " Lower bound: " << lower_bounds[i] << ", Upper bound: " << upper_bounds[i] << std::endl;
	}

	// Check the enumerated points
    const std::vector<IndexPoint>& points = index_space.getPoints();
	std::cout << "Enumerated points in the index space:" << std::endl;
	if (points.empty()) {
		std::cout << "No points found in the index space." << std::endl;
		return EXIT_FAILURE;
	}
    int count{ 0 };
    for (const auto& point : points) {
        std::cout << point;
		if (++count % 6 == 0) {
			std::cout << '\n';
		}
    }

    std::cout << std::endl;

    return EXIT_SUCCESS;
}