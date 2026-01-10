#include <vector>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <limits>

// Represents a half-plane constraint in N dimensions: sum(a[i]*x[i]) <= b
struct HalfPlane {
    std::vector<double> a; // Coefficients for each dimension
    double b;             // Right-hand side
};

// Simplex method solver for linear programming
class Simplex {
public:
    // Solve LP: maximize c^T x subject to Ax <= b
    // Returns the optimal solution x, or throws if infeasible/unbounded
    std::vector<double> solve(const std::vector<std::vector<double>>& A,
                             const std::vector<double>& b,
                             const std::vector<double>& c) {
        int m = b.size(); // Number of constraints
        int n = c.size(); // Number of variables

        // Transform constraints to standard form: Ax <= b, b >= 0
        std::vector<std::vector<double>> A_std = A;
        std::vector<double> b_std = b;
        std::vector<bool> needs_artificial(m, false);
        int num_artificial = 0;
        for (int i = 0; i < m; ++i) {
            if (b[i] < 0) {
                needs_artificial[i] = true;
                num_artificial++;
                b_std[i] = -b[i];
                for (int j = 0; j < n; ++j) {
                    A_std[i][j] = -A[i][j];
                }
            }
        }

        // Initialize tableau
        int total_vars = n + m + num_artificial; // Original + slack + artificial
        tableau_.clear();
        tableau_.resize(m + 1, std::vector<double>(total_vars + 1));
        basis_.resize(m);

        // Setup constraints
        int artificial_idx = n + m;
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                tableau_[i][j] = A_std[i][j];
            }
            tableau_[i][n + i] = 1.0; // Slack variable
            tableau_[i][total_vars] = b_std[i];
            basis_[i] = needs_artificial[i] ? artificial_idx++ : n + i;
        }

        // Phase 1: Minimize sum of artificial variables
        if (num_artificial > 0) {
            for (int j = 0; j <= total_vars; ++j) {
                tableau_[m][j] = 0.0;
            }
            for (int i = 0; i < m; ++i) {
                if (needs_artificial[i]) {
                    for (int j = 0; j <= total_vars; ++j) {
                        tableau_[m][j] -= tableau_[i][j];
                    }
                }
            }

            if (!run_simplex(m, total_vars)) {
                throw std::runtime_error("LP is infeasible");
            }

            if (std::abs(tableau_[m][total_vars]) > 1e-10) {
                throw std::runtime_error("LP is infeasible");
            }

            // Clear artificial variables
            tableau_.resize(m + 1, std::vector<double>(n + m + 1));
            total_vars = n + m;
        }

        // Phase 2: Optimize original objective
        for (int j = 0; j < n; ++j) {
            tableau_[m][j] = -c[j]; // Negate for maximization
        }
        tableau_[m][total_vars] = 0.0;

        // Adjust objective for current basis
        for (int i = 0; i < m; ++i) {
            if (basis_[i] < n) {
                double factor = tableau_[m][basis_[i]];
                for (int j = 0; j <= total_vars; ++j) {
                    tableau_[m][j] -= factor * tableau_[i][j];
                }
            }
        }

        if (!run_simplex(m, total_vars)) {
            throw std::runtime_error("LP is unbounded");
        }

        // Extract solution
        std::vector<double> solution(n, 0.0);
        for (int i = 0; i < m; ++i) {
            if (basis_[i] < n) {
                solution[basis_[i]] = tableau_[i][total_vars];
            }
        }
        return solution;
    }

private:
    bool run_simplex(int m, int total_vars) {
        const double EPS = 1e-10;
        while (true) {
            // Find pivot column
            int pivot_col = -1;
            double min_val = -EPS;
            for (int j = 0; j < total_vars; ++j) {
                if (tableau_[m][j] < min_val) {
                    min_val = tableau_[m][j];
                    pivot_col = j;
                }
            }
            if (pivot_col == -1) {
                return true; // Optimal
            }

            // Find pivot row
            int pivot_row = -1;
            double min_ratio = std::numeric_limits<double>::infinity();
            for (int i = 0; i < m; ++i) {
                if (tableau_[i][pivot_col] > EPS) {
                    double ratio = tableau_[i][total_vars] / tableau_[i][pivot_col];
                    if (ratio < min_ratio && ratio >= -EPS) {
                        min_ratio = ratio;
                        pivot_row = i;
                    }
                }
            }
            if (pivot_row == -1) {
                return false; // Unbounded
            }

            // Pivot
            double pivot = tableau_[pivot_row][pivot_col];
            for (int j = 0; j <= total_vars; ++j) {
                tableau_[pivot_row][j] /= pivot;
            }
            for (int i = 0; i <= m; ++i) {
                if (i != pivot_row) {
                    double factor = tableau_[i][pivot_col];
                    for (int j = 0; j <= total_vars; ++j) {
                        tableau_[i][j] -= factor * tableau_[pivot_row][j];
                    }
                }
            }
            basis_[pivot_row] = pivot_col;
        }
    }

    std::vector<std::vector<double>> tableau_;
    std::vector<int> basis_;
};

// Class to enumerate integer lattice points in an N-dimensional convex hull
class ConvexHullLatticeEnumerator {
public:
    // Constructor takes dimension and list of half-plane constraints
    ConvexHullLatticeEnumerator(int dimension, const std::vector<HalfPlane>& planes)
        : dimension_(dimension), planes_(planes) {
        if (dimension < 1) {
            throw std::invalid_argument("Dimension must be positive");
        }
        if (planes.empty()) {
            throw std::invalid_argument("At least one half-plane constraint is required");
        }
        for (const auto& plane : planes) {
            if (plane.a.size() != static_cast<size_t>(dimension)) {
                throw std::invalid_argument("Constraint coefficients must match dimension");
            }
        }
        compute_bounding_box();
    }

    // Enumerate all integer lattice points inside the convex hull
    std::vector<std::vector<int>> enumerate_points() {
        std::vector<std::vector<int>> points;

        // Initialize current point to the minimum integer coordinates
        std::vector<int> current(dimension_);
        for (int d = 0; d < dimension_; ++d) {
            current[d] = static_cast<int>(std::floor(bounds_min_[d]));
        }

        // Iterate over all integer points in the bounding box
        while (current[0] <= static_cast<int>(std::ceil(bounds_max_[0]))) {
            if (is_inside_hull(current)) {
                points.push_back(current);
            }

            // Advance to the next point (odometer-like increment)
            int d = dimension_ - 1;
            while (d >= 0) {
                current[d]++;
                if (current[d] <= static_cast<int>(std::ceil(bounds_max_[d]))) {
                    break;
                }
                current[d] = static_cast<int>(std::floor(bounds_min_[d]));
                d--;
            }
            if (d < 0) {
                break; // Exhausted all points
            }
            for (int i = d + 1; i < dimension_; ++i) {
                current[i] = static_cast<int>(std::floor(bounds_min_[i]));
            }
        }

        return points;
    }

private:
    // Check if a point satisfies all half-plane constraints
    bool is_inside_hull(const std::vector<int>& point) const {
        for (const auto& plane : planes_) {
            double value = 0.0;
            for (int i = 0; i < dimension_; ++i) {
                value += plane.a[i] * point[i];
            }
            if (value > plane.b + 1e-10) { // Small numerical tolerance
                return false;
            }
        }
        return true;
    }

    // Compute exact bounding box using Simplex method
    void compute_bounding_box() {
        bounds_min_.resize(dimension_, std::numeric_limits<double>::infinity());
        bounds_max_.resize(dimension_, -std::numeric_limits<double>::infinity());

        Simplex simplex;
        std::vector<std::vector<double>> A(planes_.size(), std::vector<double>(dimension_));
        std::vector<double> b(planes_.size());

        // Setup constraint matrix A and vector b
        for (size_t i = 0; i < planes_.size(); ++i) {
            for (int j = 0; j < dimension_; ++j) {
                A[i][j] = planes_[i].a[j];
            }
            b[i] = planes_[i].b;
        }

        // For each dimension, compute min and max
        for (int d = 0; d < dimension_; ++d) {
            // Objective function: maximize x_d (or minimize -x_d)
            std::vector<double> c(dimension_, 0.0);
            
            // Maximize x_d
            c[d] = 1.0;
            try {
                auto solution = simplex.solve(A, b, c);
                bounds_max_[d] = solution[d];
            } catch (const std::exception& e) {
                bounds_max_[d] = 1e10; // Fallback for unbounded
            }

            // Minimize x_d (maximize -x_d)
            c[d] = -1.0;
            try {
                auto solution = simplex.solve(A, b, c);
                bounds_min_[d] = solution[d];
            } catch (const std::exception& e) {
                bounds_min_[d] = -1e10; // Fallback for unbounded
            }

            // Debug output for bounds
            std::cout << "Dimension " << d << ": min = " << bounds_min_[d]
                      << ", max = " << bounds_max_[d] << "\n";
        }

        // Validate bounds
        for (int d = 0; d < dimension_; ++d) {
            if (bounds_min_[d] > bounds_max_[d] + 1e-10) {
                throw std::runtime_error("Invalid bounding box: min > max");
            }
        }
    }

    int dimension_;
    std::vector<HalfPlane> planes_;
    std::vector<double> bounds_min_;
    std::vector<double> bounds_max_;
};

// Example usage
int main() {
    try {
        // Example: 1D constraint -x <= -1 (i.e., x >= 1) and x <= 2
        int dimension = 1;
        std::vector<HalfPlane> planes = {
            {{-1}, -1},  // -x <= -1 (x >= 1)
            {{1}, 2}     // x <= 2
        };

        ConvexHullLatticeEnumerator enumerator(dimension, planes);
        auto points = enumerator.enumerate_points();

        std::cout << "Integer lattice points in " << dimension << "D convex hull:\n";
        for (const auto& point : points) {
            std::cout << "(";
            for (int i = 0; i < dimension; ++i) {
                std::cout << point[i];
                if (i < dimension - 1) std::cout << ", ";
            }
            std::cout << ")\n";
        }
        std::cout << "Total points: " << points.size() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}