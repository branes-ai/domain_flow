// qr_sure.cpp
//
// Functional simulation of a System of (Affine) Recurrence Equations for the
// QR decomposition by Modified Gram-Schmidt (docs/SURE/QR_decomposition.md).
//
// A is m x n with independent columns; we compute A = Q*R, Q orthonormal, R upper
// triangular.  The system mixes dimensionalities and uses affine projection taps,
// reductions, and the sqrt/division nonlinearities -- everything beyond a pure
// uniform recurrence.
//
//   partial vector (column j after removing projections onto q_0..q_{k-1}):
//     v(i,j,k) = v(i,j,k-1) - r(k-1,j)*q(i,k-1)        1 <= k <= j
//     v(i,j,0) = A(i,j)                                (boundary)
//
//   off-diagonal coefficient  r(t,j) = sum_i q(i,t) * v(i,j,t),  t < j:
//     sr(i,t,j) = sr(i-1,t,j) + q(i,t)*v(i,j,t)        boundary i<0 : 0
//     r(t,j)    = sr(m-1,t,j)
//
//   diagonal coefficient (column norm)  rho(j) = || v(.,j,j) ||:
//     sn(i,j) = sn(i-1,j) + v(i,j,j)^2                 boundary i<0 : 0
//     rho(j)  = sqrt( sn(m-1,j) )
//
//   normalized column:  q(i,j) = v(i,j,j) / rho(j)
//
//   R(t,j) = r(t,j) for t<j ;  R(j,j) = rho(j) ;  0 for t>j.

#include <iostream>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_simulator.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    const int m = 3, n = 3;
    std::vector<std::vector<double>> Amat = {
        { 12, -51,   4 },
        {  6, 167, -68 },
        { -4,  24, -41 }
    };

    RecurrenceSystem<double> sys;

    // A: input operand (i,j) -> A[i][j]
    sys.add(Equation<double>{
        "A", Domain{}, {}, nullptr,
        [&](const IndexPoint& p) { return Amat[p[0]][p[1]]; }
    });

    // v(i,j,k): domain  0<=i<m, 0<=j<n, 1<=k<=j
    {
        Domain dom(3);
        dom.axis(0, 0, m).axis(1, 0, n).axis(2, 0, n);
        dom.add({ std::vector<int>{0, 0, 1},  1, HalfSpace::GE });   // k >= 1
        dom.add({ std::vector<int>{0, -1, 1}, 0, HalfSpace::LE });   // k - j <= 0
        sys.add(Equation<double>{
            "v", dom,
            { { "v", AffineDependency::shift({ 0, 0, -1 }) },                      // v(i,j,k-1)
              { "r", AffineDependency::map({{0,0,1},{0,1,0}}, { -1, 0 }) },        // r(k-1,j)
              { "q", AffineDependency::map({{1,0,0},{0,0,1}}, {  0,-1 }) } },      // q(i,k-1)
            [](const std::vector<double>& t, const IndexPoint&) { return t[0] - t[1] * t[2]; },
            [&](const IndexPoint& p) { return Amat[p[0]][p[1]]; }                 // v(i,j,0) = A(i,j)
        });
    }

    // sr(i,t,j): reduction for r,  domain 0<=i<m, 0<=t<n, t<j<n
    {
        Domain dom(3);
        dom.axis(0, 0, m).axis(1, 0, n).axis(2, 0, n);
        dom.add({ std::vector<int>{0, -1, 1}, 1, HalfSpace::GE });   // j - t >= 1
        sys.add(Equation<double>{
            "sr", dom,
            { { "sr", AffineDependency::shift({ -1, 0, 0 }) },                         // sr(i-1,t,j)
              { "q",  AffineDependency::map({{1,0,0},{0,1,0}}, { 0, 0 }) },            // q(i,t)
              { "v",  AffineDependency::map({{1,0,0},{0,0,1},{0,1,0}}, { 0,0,0 }) } }, // v(i,j,t)
            [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[2]; },
            [](const IndexPoint&) { return 0.0; }                                     // i<0 : 0
        });
    }

    // r(t,j) = sr(m-1,t,j),  domain 0<=t<n, t<j<n
    {
        Domain dom(2);
        dom.axis(0, 0, n).axis(1, 0, n);
        dom.add({ std::vector<int>{-1, 1}, 1, HalfSpace::GE });      // j - t >= 1
        sys.add(Equation<double>{
            "r", dom,
            { { "sr", AffineDependency::map({{0,0},{1,0},{0,1}}, { m - 1, 0, 0 }) } },
            [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
            [](const IndexPoint&) { return 0.0; }
        });
    }

    // sn(i,j): reduction for the column norm,  domain 0<=i<m, 0<=j<n
    sys.add(Equation<double>{
        "sn", Domain(2).axis(0, 0, m).axis(1, 0, n),
        { { "sn", AffineDependency::shift({ -1, 0 }) },                          // sn(i-1,j)
          { "v",  AffineDependency::map({{1,0},{0,1},{0,1}}, { 0, 0, 0 }) } },   // v(i,j,j)
        [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[1]; },
        [](const IndexPoint&) { return 0.0; }
    });

    // rho(j) = sqrt(sn(m-1,j)),  domain 0<=j<n
    sys.add(Equation<double>{
        "rho", Domain(1).axis(0, 0, n),
        { { "sn", AffineDependency::map({{0},{1}}, { m - 1, 0 }) } },
        [](const std::vector<double>& t, const IndexPoint&) { return std::sqrt(t[0]); },
        [](const IndexPoint&) { return 0.0; }
    });

    // q(i,j) = v(i,j,j) / rho(j),  domain 0<=i<m, 0<=j<n
    sys.add(Equation<double>{
        "q", Domain(2).axis(0, 0, m).axis(1, 0, n),
        { { "v",   AffineDependency::map({{1,0},{0,1},{0,1}}, { 0, 0, 0 }) },    // v(i,j,j)
          { "rho", AffineDependency::map({{0,1}}, { 0 }) } },                    // rho(j)
        [](const std::vector<double>& t, const IndexPoint&) { return t[0] / t[1]; },
        [](const IndexPoint&) { return 0.0; }
    });

    // ---- evaluate Q and R ----
    SureSimulator<double> sim(sys);
    std::vector<std::vector<double>> Q(m, std::vector<double>(n, 0));
    std::vector<std::vector<double>> R(n, std::vector<double>(n, 0));
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j)
            Q[i][j] = sim.eval("q", IndexPoint({ i, j }));
    for (int j = 0; j < n; ++j) {
        R[j][j] = sim.eval("rho", IndexPoint({ j }));
        for (int t = 0; t < j; ++t)
            R[t][j] = sim.eval("r", IndexPoint({ t, j }));
    }

    auto printMat = [](const char* tag, const std::vector<std::vector<double>>& Mx) {
        std::cout << tag << ":\n";
        for (const auto& row : Mx) {
            std::cout << "  ";
            for (double x : row) std::cout << (std::abs(x) < 1e-12 ? 0.0 : x) << "\t";
            std::cout << "\n";
        }
    };
    printMat("Q", Q);
    printMat("R", R);

    // ---- verification ----
    bool ok = true;
    double maxQR = 0, maxOrtho = 0;
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j) {
            double qr = 0;
            for (int t = 0; t < n; ++t) qr += Q[i][t] * R[t][j];     // (Q R)[i][j]
            maxQR = std::max(maxQR, std::abs(qr - Amat[i][j]));
        }
    for (int a = 0; a < n; ++a)
        for (int b = 0; b < n; ++b) {
            double d = 0;
            for (int i = 0; i < m; ++i) d += Q[i][a] * Q[i][b];      // (Q^T Q)[a][b]
            maxOrtho = std::max(maxOrtho, std::abs(d - (a == b ? 1.0 : 0.0)));
        }
    std::cout << "\nmax|Q*R - A|   = " << maxQR << "\n";
    std::cout << "max|Q^T*Q - I| = " << maxOrtho << "\n";
    ok &= (maxQR < 1e-9) && (maxOrtho < 1e-9);

    // ---- memory cardinality under the free schedule ----
    // A single global linear tau is ill-defined across the heterogeneous index
    // spaces here (v is 3D, q/r/sn 2D, rho 1D), so we use the free schedule, which
    // is well-defined for any recurrence system.
    ExplicitSchedule freeSched = sim.computeFreeSchedule();
    LivenessReport fr = sim.analyzeMemory(freeSched);
    long freePeak = 0;
    sim.run(freeSched, &freePeak);
    std::cout << "\nfree schedule: peakLiveValues=" << fr.peakLiveValues
              << "  latency=" << fr.latency
              << "  work=" << fr.totalValues
              << "  (run footprint=" << freePeak << ")\n";
    std::cout << "  per-variable peak live: ";
    for (const auto& [var, pk] : fr.peakPerVar) std::cout << var << "=" << pk << " ";
    std::cout << "\n";
    ok &= (freePeak == fr.peakLiveValues);

    // ---- schedule legality: the free schedule is legal by construction ----
    std::cout << "\nLegality:\n  free schedule : " << checkLegality(sys, freeSched) << "\n";
    ok &= checkLegality(sys, freeSched).legal;

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
