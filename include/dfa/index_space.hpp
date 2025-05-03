#pragma once
#include <stdexcept>
#include <dfa/hyperplane.hpp>
#include <dfa/simplex_method.hpp>

namespace sw {
    namespace dfa {

        // Structure to represent an fully enumerated index space defined by the HyperPlane constraints
        template<typename ConstraintCoefficientType = int>
        class IndexSpace {
            using IndexPointType = int;
        public:
			// default constructor is empty
            IndexSpace() : dimension{ 0 }, constraints {}, lower_bounds{}, upper_bounds{}, points{} {}
			// given a set of constraints, create the index space
            IndexSpace(const ConstraintSet<ConstraintCoefficientType>& c) 
                : constraints(c) {
				if (c.empty()) {
					std::cerr << "IndexSpace constructor: at least one constraint is required\n";
				}
				dimension = constraints[0].normal.size();
				compute_bounding_box();
                generate();
            }

            // modifiers
			void clear() {
				dimension = 0;
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
				if (dimension == 0) {
					dimension = constraints[0].normal.size();
				}

			}

            // Enumerate all integer lattice points inside the convex hull
            std::vector<std::vector<int>> enumerate_points() {
                std::vector<std::vector<int>> points;

                // Initialize current point to the minimum integer coordinates
                std::vector<int> current(dimension);
                for (int d = 0; d < dimension; ++d) {
                    current[d] = static_cast<int>(std::floor(lower_bounds[d]));
                }

                // Iterate over all integer points in the bounding box
                while (current[0] <= static_cast<int>(std::ceil(upper_bounds[0]))) {
                    if (is_inside_hull(current)) {
                        points.push_back(current);
                    }

                    // Advance to the next point (odometer-like increment)
                    int d = dimension - 1;
                    while (d >= 0) {
                        current[d]++;
                        if (current[d] <= static_cast<int>(std::ceil(upper_bounds[d]))) {
                            break;
                        }
                        current[d] = static_cast<int>(std::floor(lower_bounds[d]));
                        d--;
                    }
                    if (d < 0) {
                        break; // Exhausted all points
                    }
                    for (int i = d + 1; i < dimension; ++i) {
                        current[i] = static_cast<int>(std::floor(lower_bounds[i]));
                    }
                }

                return points;
            }

            // selectors
            const std::vector<IndexPoint>& get_points() const {
                return points;
            }

        private:
            int dimension;
            ConstraintSet<ConstraintCoefficientType> constraints;
            std::vector<IndexPointType> lower_bounds;
            std::vector<IndexPointType> upper_bounds;
            std::vector<IndexPoint> points;

            // Check if point is inside the hull
			bool isInsideHull(const IndexPoint& point) const {
				for (const auto& constraint : constraints.get_constraints()) {
					if (!constraint.is_satisfied(point)) {
						return false;
					}
				}
				return true;
			}

            // Compute an approximate bounding box for the convex hull
			// precondition: constraints must be set
            void compute_bounding_box_grid_sampler () {
				if (constraints.empty()) {
					throw std::runtime_error("No constraints set for bounding box computation");
				}
                lower_bounds.resize(dimension, 1e10);
                upper_bounds.resize(dimension, -1e10);
                // Sample points in a large N-dimensional cube
                const double R = 1000.0; // Search radius
                const int steps = 20;    // Grid points per dimension
                IndexPoint point(dimension, 0);
                std::vector<int> indices(dimension, -steps);

                while (indices[0] <= steps) {
                    // Convert indices to coordinates
                    for (int d = 0; d < dimension; ++d) {
                        point[d] = indices[d] * R / steps;
                    }

					// Check if point is inside the convex hull
                    bool inside = isInsideHull(point);

                    // Update bounds if point is inside
                    if (inside) {
                        for (int d = 0; d < dimension; ++d) {
                            lower_bounds[d] = std::min(lower_bounds[d], point[d]);
                            upper_bounds[d] = std::max(upper_bounds[d], point[d]);
                        }
                    }

                    // Advance to next point in grid
                    int d = dimension - 1;
                    while (d >= 0) {
                        indices[d]++;
                        if (indices[d] <= steps) {
                            break;
                        }
                        indices[d] = -steps;
                        d--;
                    }
                    if (d < 0) {
                        break;
                    }
                    for (int i = d + 1; i < dimension; ++i) {
                        indices[i] = -steps;
                    }
                }

                // Check if valid points were found
                for (int d = 0; d < dimension; ++d) {
                    if (lower_bounds[d] > upper_bounds[d]) {
                        throw std::runtime_error("No valid points found in convex hull");
                    }
                }
            }

            // Compute exact bounding box using Simplex method
            void compute_bounding_box() {
                lower_bounds.resize(dimension, std::numeric_limits<double>::infinity());
                upper_bounds.resize(dimension, -std::numeric_limits<double>::infinity());

                Simplex simplex;
                std::vector<std::vector<double>> A(constraints.size(), std::vector<double>(dimension));
                std::vector<double> b(constraints.size());

                // Setup constraint matrix A and vector b
                for (size_t i = 0; i < constraints.size(); ++i) {
                    for (int j = 0; j < dimension; ++j) {
                        A[i][j] = constraints[i].normal[j];
                    }
                    if (constraints[i].constraint == ConstraintType::GreaterOrEqual ||
                        constraints[i].constraint == ConstraintType::GreaterThan) {
                        b[i] = -constraints[i].rhs;
                    }
					else {
						b[i] = constraints[i].rhs;
					}
                }

                // For each dimension, compute min and max
                for (int d = 0; d < dimension; ++d) {
                    // Objective function: maximize x_d (or minimize -x_d)
                    std::vector<double> c(dimension, 0.0);

                    // Maximize x_d
                    c[d] = 1.0;
                    try {
                        auto solution = simplex.solve(A, b, c);
                        upper_bounds[d] = solution[d];
                    }
                    catch (const std::exception& e) {
                        // Handle unbounded or infeasible cases
                        upper_bounds[d] = 1e10; // Large fallback value
                    }

                    // Minimize x_d (maximize -x_d)
                    c[d] = -1.0;
                    try {
                        auto solution = simplex.solve(A, b, c);
                        lower_bounds[d] = solution[d];
                    }
                    catch (const std::exception& e) {
                        lower_bounds[d] = -1e10; // Large negative fallback value
                    }
                }

                // Validate bounds
                for (int d = 0; d < dimension; ++d) {
                    if (lower_bounds[d] > upper_bounds[d]) {
                        throw std::runtime_error("Invalid bounding box: no feasible points");
                    }
                }
            }

			// Generate all points in the index space
			// precondition: bounding box must be set
            void generate() {
                std::vector<IndexPointType> current_point(dimension);
                for (size_t i = 0; i < dimension; ++i) {
                    current_point[i] = lower_bounds[i];
                }

                while (true) {
                    // Check if a point satisfies all half-plane constraints
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

                    int current_dimension = dimension - 1;
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
