#pragma once

namespace sw {
    namespace dfa {

/*
 The Simplex algorithm iteratively improves a feasible solution by pivoting in the tableau until the objective cannot be increased.
 The tableau includes slack variables to handle LessOrEqual (<=) constraints, and the basis tracks the variables in the solution.

 The implementation assumes (X >= 0) for all variables, which is common in LP problems.
 If negative coordinates are needed, the convex hull should be translated, 
 or the Simplex method can be extended (e.g., using two-phase Simplex).
*/


        // Simplex method solver for linear programming

        /// <summary>
		/// Simplex class for solving linear programming problems.
		/// Assumes that the constraints are in the form Ax <= b, and the objective is to maximize c^T x.
		/// Also assumed that all variables are non-negative (x >= 0) and the origin is feasible.
        /// </summary>
        class Simplex {
        public:
            // Solve LP: maximize c^T x subject to Ax <= b, x >= 0
            // Returns the optimal solution x, or throws if infeasible/unbounded
            std::vector<double> solve(const std::vector<std::vector<double>>& A,
                                      const std::vector<double>& b,
                                      const std::vector<double>& c) {
                int m = b.size(); // Number of constraints
                int n = c.size(); // Number of variables

                // Initialize tableau
                tableau.clear();
                tableau.resize(m + 1, std::vector<double>(n + m + 1));
                basis.resize(m);

                // Setup tableau: constraints
                for (int i = 0; i < m; ++i) {
                    for (int j = 0; j < n; ++j) {
                        tableau[i][j] = A[i][j];
                    }
                    tableau[i][n + i] = 1.0; // Slack variable
                    tableau[i][n + m] = b[i];
                    basis[i] = n + i;
                }

                // Setup objective row
                for (int j = 0; j < n; ++j) {
                    tableau[m][j] = -c[j]; // Negate for maximization
                }
                tableau[m][n + m] = 0.0;

                // Run Simplex algorithm
                while (true) {
                    // Find pivot column (most negative coefficient in objective row)
                    int pivot_col = -1;
                    double min_val = 0.0;
                    for (int j = 0; j < n + m; ++j) {
                        if (tableau[m][j] < min_val) {
                            min_val = tableau[m][j];
                            pivot_col = j;
                        }
                    }
                    if (pivot_col == -1) {
                        break; // Optimal solution found
                    }

                    // Find pivot row (minimum ratio test)
                    int pivot_row = -1;
                    double min_ratio = std::numeric_limits<double>::infinity();
                    for (int i = 0; i < m; ++i) {
                        if (tableau[i][pivot_col] > 1e-6) {
                            double ratio = tableau[i][n + m] / tableau[i][pivot_col];
                            if (ratio < min_ratio) {
                                min_ratio = ratio;
                                pivot_row = i;
                            }
                        }
                    }
                    if (pivot_row == -1) {
                        throw std::runtime_error("LP is unbounded");
                    }

                    // Pivot
                    double pivot = tableau[pivot_row][pivot_col];
                    for (int j = 0; j <= n + m; ++j) {
                        tableau[pivot_row][j] /= pivot;
                    }
                    for (int i = 0; i <= m; ++i) {
                        if (i != pivot_row) {
                            double factor = tableau[i][pivot_col];
                            for (int j = 0; j <= n + m; ++j) {
                                tableau[i][j] -= factor * tableau[pivot_row][j];
                            }
                        }
                    }
                    basis[pivot_row] = pivot_col;
                }

                // Extract solution
                std::vector<double> solution(n, 0.0);
                for (int i = 0; i < m; ++i) {
                    if (basis[i] < n) {
                        solution[basis[i]] = tableau[i][n + m];
                    }
                }
                return solution;
            }

        private:
            std::vector<std::vector<double>> tableau;
            std::vector<int> basis;
        };


        class TwoPhaseSimplex {
        public:
            // Solve LP: maximize c^T x subject to Ax <= b
            // Returns the optimal solution x, or throws if infeasible/unbounded
            std::vector<double> solve(const std::vector<std::vector<double>>& A,
                                      const std::vector<double>& b,
                                      const std::vector<double>& c) {
                int m = b.size(); // Number of constraints
                int n = c.size(); // Number of variables

                // Determine which constraints need artificial variables
                std::vector<bool> needs_artificial(m, false);
                int num_artificial = 0;
                for (int i = 0; i < m; ++i) {
                    if (b[i] < 0) {
                        needs_artificial[i] = true;
                        num_artificial++;
                    }
                }

                // Initialize tableau
                int total_vars = n + m + num_artificial; // Original + slack + artificial
                tableau.clear();
                tableau.resize(m + 1, std::vector<double>(total_vars + 1));
                basis.resize(m);

                // Setup constraints
                int artificial_idx = n + m;
                for (int i = 0; i < m; ++i) {
                    double sign = needs_artificial[i] ? -1.0 : 1.0;
                    for (int j = 0; j < n; ++j) {
                        tableau[i][j] = sign * A[i][j];
                    }
                    tableau[i][n + i] = sign; // Slack variable
                    tableau[i][total_vars] = sign * b[i];
                    if (needs_artificial[i]) {
                        tableau[i][artificial_idx] = 1.0; // Artificial variable
                        basis[i] = artificial_idx++;
                    }
                    else {
                        basis[i] = n + i;
                    }
                }

                // Phase 1: Minimize sum of artificial variables
                if (num_artificial > 0) {
                    // Objective: minimize sum of artificial variables
                    std::vector<double> phase1_obj(total_vars, 0.0);
                    for (int j = n + m; j < total_vars; ++j) {
                        phase1_obj[j] = -1.0; // Maximize -sum(artificial)
                    }
                    for (int i = 0; i < m; ++i) {
                        if (needs_artificial[i]) {
                            for (int j = 0; j <= total_vars; ++j) {
                                tableau[m][j] -= tableau[i][j];
                            }
                        }
                    }

                    // Run Simplex for Phase 1
                    if (!run_simplex(m, total_vars)) {
                        throw std::runtime_error("LP is infeasible");
                    }

                    // Check if artificial variables are zero
                    if (std::abs(tableau[m][total_vars]) > 1e-6) {
                        throw std::runtime_error("LP is infeasible");
                    }

                    // Clear artificial variables from tableau
                    for (int j = n + m; j < total_vars; ++j) {
                        for (int i = 0; i <= m; ++i) {
                            tableau[i][j] = 0.0;
                        }
                    }
                    total_vars = n + m;
                    tableau.resize(m + 1, std::vector<double>(total_vars + 1));
                }

                // Phase 2: Optimize original objective
                for (int j = 0; j < n; ++j) {
                    tableau[m][j] = -c[j]; // Negate for maximization
                }
                tableau[m][total_vars] = 0.0;

                // Adjust objective row for non-basic variables
                for (int i = 0; i < m; ++i) {
                    if (basis[i] < n) {
                        double factor = -c[basis[i]];
                        for (int j = 0; j <= total_vars; ++j) {
                            tableau[m][j] += factor * tableau[i][j];
                        }
                    }
                }

                // Run Simplex for Phase 2
                if (!run_simplex(m, total_vars)) {
                    throw std::runtime_error("LP is unbounded");
                }

                // Extract solution
                std::vector<double> solution(n, 0.0);
                for (int i = 0; i < m; ++i) {
                    if (basis[i] < n) {
                        solution[basis[i]] = tableau[i][total_vars];
                    }
                }
                return solution;
            }

        private:
            bool run_simplex(int m, int total_vars) {
                while (true) {
                    // Find pivot column
                    int pivot_col = -1;
                    double min_val = 0.0;
                    for (int j = 0; j < total_vars; ++j) {
                        if (tableau[m][j] < min_val) {
                            min_val = tableau[m][j];
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
                        if (tableau[i][pivot_col] > 1e-6) {
                            double ratio = tableau[i][total_vars] / tableau[i][pivot_col];
                            if (ratio < min_ratio) {
                                min_ratio = ratio;
                                pivot_row = i;
                            }
                        }
                    }
                    if (pivot_row == -1) {
                        return false; // Unbounded
                    }

                    // Pivot
                    double pivot = tableau[pivot_row][pivot_col];
                    for (int j = 0; j <= total_vars; ++j) {
                        tableau[pivot_row][j] /= pivot;
                    }
                    for (int i = 0; i <= m; ++i) {
                        if (i != pivot_row) {
                            double factor = tableau[i][pivot_col];
                            for (int j = 0; j <= total_vars; ++j) {
                                tableau[i][j] -= factor * tableau[pivot_row][j];
                            }
                        }
                    }
                    basis[pivot_row] = pivot_col;
                }
            }

            std::vector<std::vector<double>> tableau;
            std::vector<int> basis;
        };
    }
}

