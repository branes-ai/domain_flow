// eig_qr_sure.cpp
//
// Symmetric eigensolver, phase 1 -- one Householder tridiagonalization step -- as a
// System of Affine Recurrence Equations (issue #48, docs/SURE/eig_qr.sure). The two-sided
// reflection A' = H A H with H = I - beta v v^T zeros column 0 below the subdiagonal
// (rows >= 2), the executable heart of reducing a symmetric A to tridiagonal T before
// implicit-shift QR iteration. It composes three row-reductions (||A[1:,0]||^2, v^T v,
// v^T A v) and a column-reduction (A v) with the reflector's rank-2 update -- a SARE.
//
// This test parses the executable doc and checks:
//   - the parsed structure and the derived output face normal
//   - column 0 is zeroed below the subdiagonal (A'[2][0] = A'[3][0] = 0) and A' stays symmetric
//   - A' preserves the spectrum: it equals H A H for the H the spec computes (an
//     orthogonal similarity), so trace and Frobenius norm are invariant
//   - the RDG is a SARE (the fixed-index reflector taps and reductions)
//   - the free schedule is legal

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>
#include <dfa/sim/rdg.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/eig_qr.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.outputs.size() == 1);   // R
        ok &= (spec.system.equations().size() == 13);
        std::cout << "parsed structure (indices, 13 variables, 1 output): " << (ok ? "PASS" : "FAIL") << "\n";
        bool nok = (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });   // R drains on k=1
        std::cout << "output face normal (R drains on k=1): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- evaluate A' = R ----
        const size_t N = 4;
        const double A[4][4] = { { 4, 1, 2, 1 }, { 1, 3, 1, 2 }, { 2, 1, 5, 1 }, { 1, 2, 1, 6 } };
        double R[4][4] = {};
        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            R[idx[0]][idx[1]] = out.eval(sim, p);
        }

        // ---- column 0 zeroed below the subdiagonal, A' symmetric ----
        bool zeroed = std::abs(R[2][0]) < 1e-9 && std::abs(R[3][0]) < 1e-9
                   && std::abs(R[0][2]) < 1e-9 && std::abs(R[0][3]) < 1e-9;
        double symErr = 0;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) symErr = std::max(symErr, std::abs(R[i][j] - R[j][i]));
        std::cout << "column 0 tridiagonalized (A'[2][0]=A'[3][0]=0) and A' symmetric: "
                  << (zeroed && symErr < 1e-9 ? "PASS" : "FAIL")
                  << "  (A'[1][0] = " << R[1][0] << ", symErr = " << symErr << ")\n";
        ok &= zeroed && symErr < 1e-9;

        // ---- spectrum preserved: A' == H A H for the spec's reflector (host-recomputed) ----
        double nrm2 = 0;
        for (size_t i = 1; i < N; ++i) nrm2 += A[i][0] * A[i][0];
        double alpha = std::sqrt(nrm2), sgn = (A[1][0] > 0 ? 1.0 : -1.0);
        double v[4] = { 0, A[1][0] + sgn * alpha, A[2][0], A[3][0] };
        double vv = 0; for (size_t i = 0; i < N; ++i) vv += v[i] * v[i];
        double beta = 2.0 / vv;
        double Av[4] = {};
        for (size_t i = 0; i < N; ++i) for (size_t j = 0; j < N; ++j) Av[i] += A[i][j] * v[j];
        double vAv = 0; for (size_t i = 0; i < N; ++i) vAv += v[i] * Av[i];
        double w[4]; for (size_t i = 0; i < N; ++i) w[i] = beta * Av[i] - (beta * beta * vAv / 2) * v[i];
        double HAH[4][4];
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) HAH[i][j] = A[i][j] - v[i] * w[j] - w[i] * v[j];
        double simErr = 0;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) simErr = std::max(simErr, std::abs(R[i][j] - HAH[i][j]));

        double trA = 0, trR = 0, frA = 0, frR = 0;
        for (size_t i = 0; i < N; ++i) { trA += A[i][i]; trR += R[i][i]; }
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) { frA += A[i][j] * A[i][j]; frR += R[i][j] * R[i][j]; }
        bool spectrum = simErr < 1e-9 && std::abs(trA - trR) < 1e-9 && std::abs(frA - frR) < 1e-9;
        std::cout << "A' = H A H (spectrum preserved: trace " << trR << ", Frobenius^2 " << frR << "): "
                  << (spectrum ? "PASS" : "FAIL") << "  (||A' - HAH||_inf = " << simErr << ")\n";
        ok &= spectrum;

        // ---- the RDG is a SARE ----
        Rdg g = buildRdg(spec, "eig_qr");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        std::cout << "RDG is a SARE (fixed-index reflector taps + reductions): "
                  << (g.isSare() && affine > 0 ? "PASS" : "FAIL")
                  << "  (" << g.arcs.size() << " arcs, " << affine << " affine)\n";
        ok &= g.isSare() && affine > 0;

        // ---- the free schedule is legal ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        bool fl = checkLegality(spec.system, freeSched).legal;
        std::cout << "  free: " << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "free schedule legal: " << (fl ? "PASS" : "FAIL") << "\n";
        ok &= fl;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
