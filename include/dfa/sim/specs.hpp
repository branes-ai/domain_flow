#pragma once
// Built-in System-of-Recurrence-Equation specifications for the SURE simulator.
// Each builder returns a self-contained Spec: the recurrence system (with its
// input data captured by value), a canonical legal linear schedule (tau/beta, or
// empty tau when no single global linear schedule applies), and a closure that
// evaluates and prints/verifies the operator's outputs.
#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <dfa/sim/sure_simulator.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            struct Spec {
                std::string name;
                std::string description;
                RecurrenceSystem<double> system;
                std::vector<int> tau;                 // canonical linear schedule (empty => free-only)
                std::map<std::string, long> beta;     // per-variable stage offsets
                std::function<bool(SureSimulator<double>&, std::ostream&)> printOutputs;  // returns ok
            };

            // C = C0 + A*B  (three-input matmul; C0 enters as the k<0 boundary of c)
            inline Spec buildMatmul3() {
                const int M = 2, N = 2, K = 2;
                std::vector<std::vector<double>> A  = {{1, 2}, {3, 4}};
                std::vector<std::vector<double>> B  = {{5, 6}, {7, 8}};
                std::vector<std::vector<double>> C0 = {{10, 20}, {30, 40}};

                RecurrenceSystem<double> sys;
                sys.add(Equation<double>{
                    "a", Domain(3).axis(0, 0, M).axis(1, 0, N).axis(2, 0, K),
                    { { "a", AffineDependency::shift({0, -1, 0}) } },
                    [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
                    [A](const IndexPoint& p) { return A[p[0]][p[2]]; } });
                sys.add(Equation<double>{
                    "b", Domain(3).axis(0, 0, M).axis(1, 0, N).axis(2, 0, K),
                    { { "b", AffineDependency::shift({-1, 0, 0}) } },
                    [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
                    [B](const IndexPoint& p) { return B[p[2]][p[1]]; } });
                sys.add(Equation<double>{
                    "c", Domain(3).axis(0, 0, M).axis(1, 0, N).axis(2, 0, K),
                    { { "c", AffineDependency::shift({0,  0, -1}) },
                      { "a", AffineDependency::shift({0, -1,  0}) },
                      { "b", AffineDependency::shift({-1, 0,  0}) } },
                    [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[2]; },
                    [C0](const IndexPoint& p) { return C0[p[0]][p[1]]; } });

                Spec s;
                s.name = "matmul";
                s.description = "C = C0 + A*B  (2x2, uniform recurrence)";
                s.system = std::move(sys);
                s.tau = {1, 1, 1};
                s.printOutputs = [M, N, K, A, B, C0](SureSimulator<double>& sim, std::ostream& os) {
                    bool ok = true;
                    os << "C = C0 + A*B:\n";
                    for (int i = 0; i < M; ++i)
                        for (int j = 0; j < N; ++j) {
                            double ref = C0[i][j];
                            for (int k = 0; k < K; ++k) ref += A[i][k] * B[k][j];
                            double got = sim.eval("c", IndexPoint({ i, j, K - 1 }));
                            bool m = std::abs(got - ref) < 1e-9;
                            os << "  C[" << i << "][" << j << "] = " << got
                               << "  (ref " << ref << ")" << (m ? "" : "  MISMATCH") << "\n";
                            ok &= m;
                        }
                    return ok;
                };
                return s;
            }

            // y = A*x  (matrix-vector product; xs broadcasts x down the rows)
            inline Spec buildMatvec() {
                const int M = 2, N = 3;
                std::vector<std::vector<double>> A = {{1, 2, 3}, {4, 5, 6}};
                std::vector<double> x = {1, 0, -1};

                RecurrenceSystem<double> sys;
                sys.add(Equation<double>{
                    "A", Domain{}, {}, nullptr,
                    [A](const IndexPoint& p) { return A[p[0]][p[1]]; } });
                sys.add(Equation<double>{
                    "xs", Domain(2).axis(0, 0, M).axis(1, 0, N),
                    { { "xs", AffineDependency::shift({-1, 0}) } },
                    [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
                    [x](const IndexPoint& p) { return x[p[1]]; } });
                sys.add(Equation<double>{
                    "y", Domain(2).axis(0, 0, M).axis(1, 0, N),
                    { { "y",  AffineDependency::shift({0, -1}) },
                      { "A",  AffineDependency::shift({0,  0}) },
                      { "xs", AffineDependency::shift({0,  0}) } },
                    [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[2]; },
                    [](const IndexPoint&) { return 0.0; } });

                Spec s;
                s.name = "matvec";
                s.description = "y = A*x  (2x3, instantaneous y<-xs dependency)";
                s.system = std::move(sys);
                s.tau = {1, 1};
                s.beta = { { "y", 1 } };   // y reads xs at the same point => stage offset
                s.printOutputs = [M, N, A, x](SureSimulator<double>& sim, std::ostream& os) {
                    bool ok = true;
                    os << "y = A*x:\n";
                    for (int i = 0; i < M; ++i) {
                        double ref = 0;
                        for (int k = 0; k < N; ++k) ref += A[i][k] * x[k];
                        double got = sim.eval("y", IndexPoint({ i, N - 1 }));
                        bool m = std::abs(got - ref) < 1e-9;
                        os << "  y[" << i << "] = " << got << "  (ref " << ref << ")"
                           << (m ? "" : "  MISMATCH") << "\n";
                        ok &= m;
                    }
                    return ok;
                };
                return s;
            }

            // A = Q*R  (Modified Gram-Schmidt QR as a SARE; sqrt/division nonlinearities)
            inline Spec buildQR() {
                const int m = 3, n = 3;
                std::vector<std::vector<double>> Amat = {
                    { 12, -51,   4 }, {  6, 167, -68 }, { -4,  24, -41 } };

                RecurrenceSystem<double> sys;
                sys.add(Equation<double>{
                    "A", Domain{}, {}, nullptr,
                    [Amat](const IndexPoint& p) { return Amat[p[0]][p[1]]; } });
                {
                    Domain dom(3);
                    dom.axis(0, 0, m).axis(1, 0, n).axis(2, 0, n);
                    dom.add({ std::vector<int>{0, 0, 1},  1, HalfSpace::GE });
                    dom.add({ std::vector<int>{0, -1, 1}, 0, HalfSpace::LE });
                    sys.add(Equation<double>{
                        "v", dom,
                        { { "v", AffineDependency::shift({ 0, 0, -1 }) },
                          { "r", AffineDependency::map({{0,0,1},{0,1,0}}, { -1, 0 }) },
                          { "q", AffineDependency::map({{1,0,0},{0,0,1}}, {  0,-1 }) } },
                        [](const std::vector<double>& t, const IndexPoint&) { return t[0] - t[1] * t[2]; },
                        [Amat](const IndexPoint& p) { return Amat[p[0]][p[1]]; } });
                }
                {
                    Domain dom(3);
                    dom.axis(0, 0, m).axis(1, 0, n).axis(2, 0, n);
                    dom.add({ std::vector<int>{0, -1, 1}, 1, HalfSpace::GE });
                    sys.add(Equation<double>{
                        "sr", dom,
                        { { "sr", AffineDependency::shift({ -1, 0, 0 }) },
                          { "q",  AffineDependency::map({{1,0,0},{0,1,0}}, { 0, 0 }) },
                          { "v",  AffineDependency::map({{1,0,0},{0,0,1},{0,1,0}}, { 0,0,0 }) } },
                        [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[2]; },
                        [](const IndexPoint&) { return 0.0; } });
                }
                {
                    Domain dom(2);
                    dom.axis(0, 0, n).axis(1, 0, n);
                    dom.add({ std::vector<int>{-1, 1}, 1, HalfSpace::GE });
                    sys.add(Equation<double>{
                        "r", dom,
                        { { "sr", AffineDependency::map({{0,0},{1,0},{0,1}}, { m - 1, 0, 0 }) } },
                        [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
                        [](const IndexPoint&) { return 0.0; } });
                }
                sys.add(Equation<double>{
                    "sn", Domain(2).axis(0, 0, m).axis(1, 0, n),
                    { { "sn", AffineDependency::shift({ -1, 0 }) },
                      { "v",  AffineDependency::map({{1,0},{0,1},{0,1}}, { 0, 0, 0 }) } },
                    [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[1]; },
                    [](const IndexPoint&) { return 0.0; } });
                sys.add(Equation<double>{
                    "rho", Domain(1).axis(0, 0, n),
                    { { "sn", AffineDependency::map({{0},{1}}, { m - 1, 0 }) } },
                    [](const std::vector<double>& t, const IndexPoint&) { return std::sqrt(t[0]); },
                    [](const IndexPoint&) { return 0.0; } });
                sys.add(Equation<double>{
                    "q", Domain(2).axis(0, 0, m).axis(1, 0, n),
                    { { "v",   AffineDependency::map({{1,0},{0,1},{0,1}}, { 0, 0, 0 }) },
                      { "rho", AffineDependency::map({{0,1}}, { 0 }) } },
                    [](const std::vector<double>& t, const IndexPoint&) { return t[0] / t[1]; },
                    [](const IndexPoint&) { return 0.0; } });

                Spec s;
                s.name = "qr";
                s.description = "A = Q*R  (3x3 Modified Gram-Schmidt, SARE)";
                s.system = std::move(sys);
                // heterogeneous index-space dims => no single global tau; free-only.
                s.printOutputs = [m, n, Amat](SureSimulator<double>& sim, std::ostream& os) {
                    std::vector<std::vector<double>> Q(m, std::vector<double>(n, 0));
                    std::vector<std::vector<double>> R(n, std::vector<double>(n, 0));
                    for (int i = 0; i < m; ++i)
                        for (int j = 0; j < n; ++j) Q[i][j] = sim.eval("q", IndexPoint({ i, j }));
                    for (int j = 0; j < n; ++j) {
                        R[j][j] = sim.eval("rho", IndexPoint({ j }));
                        for (int t = 0; t < j; ++t) R[t][j] = sim.eval("r", IndexPoint({ t, j }));
                    }
                    double maxQR = 0;
                    for (int i = 0; i < m; ++i)
                        for (int j = 0; j < n; ++j) {
                            double qr = 0;
                            for (int t = 0; t < n; ++t) qr += Q[i][t] * R[t][j];
                            maxQR = std::max(maxQR, std::abs(qr - Amat[i][j]));
                        }
                    os << "Q:\n";
                    for (int i = 0; i < m; ++i) {
                        os << "  ";
                        for (int j = 0; j < n; ++j) os << Q[i][j] << "\t";
                        os << "\n";
                    }
                    os << "R:\n";
                    for (int i = 0; i < n; ++i) {
                        os << "  ";
                        for (int j = 0; j < n; ++j) os << (std::abs(R[i][j]) < 1e-12 ? 0.0 : R[i][j]) << "\t";
                        os << "\n";
                    }
                    os << "max|Q*R - A| = " << maxQR << "\n";
                    return maxQR < 1e-9;
                };
                return s;
            }

            inline std::vector<std::string> specNames() { return { "matmul", "matvec", "qr" }; }

            inline Spec buildSpec(const std::string& name) {
                if (name == "matmul") return buildMatmul3();
                if (name == "matvec") return buildMatvec();
                if (name == "qr")     return buildQR();
                throw std::invalid_argument("unknown spec '" + name + "' (try: matmul, matvec, qr)");
            }

        }
    }
}
