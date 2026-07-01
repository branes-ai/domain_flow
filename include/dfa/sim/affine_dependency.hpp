#pragma once
#include <iostream>
#include <functional>
#include <vector>
#include <stdexcept>
#include <dfa/index_point.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // AffineDependency: a constant affine map  f(p) = A*p + b  that takes a
            // consumer index point to the producer index point it reads.
            //
            // For a *uniform* recurrence the map is a pure translation (A = I, b = shift),
            // e.g. c(i,j,k) reads c(i,j,k-1) via shift {0,0,-1}.  For an *affine* recurrence
            // (SARE) the matrix may be rectangular, expressing a projection or permutation,
            // e.g. q(i,k) read from index (i,j,k) via the 2x3 map [[1,0,0],[0,0,1]].
            //
            // This mirrors sw::dfa::AffineMap<int>::apply() but is kept dependency-free so the
            // simulator headers compile standalone; it can be swapped for AffineMap<int>.
            class AffineDependency {
                std::vector<std::vector<int>> A_;   // outDim x inDim
                std::vector<int> b_;                // outDim

            public:
                AffineDependency(std::vector<std::vector<int>> A, std::vector<int> b)
                    : A_(std::move(A)), b_(std::move(b)) {
                    if (A_.size() != b_.size()) {
                        throw std::invalid_argument("AffineDependency: row count of A must match size of b");
                    }
                }

                IndexPoint apply(const IndexPoint& p) const {
                    std::vector<int> out(A_.size());
                    for (std::size_t r = 0; r < A_.size(); ++r) {
                        const auto& row = A_[r];
                        if (row.size() != p.size()) {
                            throw std::invalid_argument("AffineDependency: input dimension mismatch");
                        }
                        long s = b_[r];
                        for (std::size_t c = 0; c < row.size(); ++c) {
                            s += static_cast<long>(row[c]) * p[c];
                        }
                        out[r] = static_cast<int>(s);
                    }
                    return IndexPoint(out);
                }

                std::size_t inDim()  const noexcept { return A_.empty() ? 0 : A_[0].size(); }
                std::size_t outDim() const noexcept { return A_.size(); }

                // Uniform dependency: producer = p + shift  (A = I, b = shift)
                static AffineDependency shift(const std::vector<int>& s) {
                    std::size_t n = s.size();
                    std::vector<std::vector<int>> A(n, std::vector<int>(n, 0));
                    for (std::size_t i = 0; i < n; ++i) A[i][i] = 1;
                    return AffineDependency(std::move(A), s);
                }

                // General affine/projection map: producer = A*p + b
                static AffineDependency map(std::vector<std::vector<int>> A, std::vector<int> b) {
                    return AffineDependency(std::move(A), std::move(b));
                }
            };

        }
    }
}
