// eig_jacobi_sure.cpp
//
// Symmetric eigensolver -- one cyclic-Jacobi rotation -- as a System of Affine Recurrence
// Equations (issue #47, docs/SURE/eig_jacobi.sure). A two-sided Givens rotation
// A' = J^T A J on the fixed pivot (p,q) = (0,1) that zeros the (0,1) off-diagonal, the
// executable heart of the cyclic Jacobi method. Done as two one-sided rotations
// (right-multiply the columns, then left-multiply the rows); the "column/row is p or q?"
// test reads fed indicator vectors, the angle uses sqrt/abs/gt/select, and the pivot
// rows/columns are fixed-index affine taps -- so it is a SARE. (The full sweep is not a
// single SURE: the changing pivot needs data-dependent addressing; see eig_jacobi.md.)
//
// This test parses the executable doc and checks:
//   - the parsed structure and the derived confluence face normals
//   - A'[0][1] and A'[1][0] are driven to zero, and A' stays symmetric
//   - A' preserves the spectrum: it equals J^T A J for the J the spec computes (an
//     orthogonal similarity), so trace and Frobenius norm are invariant
//   - the RDG is a SARE (the fixed-index taps and indicator selects)
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/eig_jacobi.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.inputs.size() == 5);   // A, Pind(col), Qind(col), Pind(row), Qind(row)
        ok &= (spec.outputs.size() == 1);  // R
        std::cout << "parsed structure (indices, 5 inputs, 1 output): " << (ok ? "PASS" : "FAIL") << "\n";
        bool nok = (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });   // R drains on k=1
        for (const auto& in : spec.inputs) nok &= (in.normal == std::vector<int>{ 0, 0, -1 });   // fed on k=-1
        std::cout << "derived face normals (all fed on k=-1, R drains k=1): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- evaluate A' = R ----
        const size_t N = 3;
        const double A[3][3] = { { 2, 1, 1 }, { 1, 3, 1 }, { 1, 1, 4 } };
        double R[3][3] = {};
        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            R[idx[0]][idx[1]] = out.eval(sim, p);
        }

        // ---- the (0,1) pair is zeroed, and A' stays symmetric ----
        bool zeroed = std::abs(R[0][1]) < 1e-9 && std::abs(R[1][0]) < 1e-9;
        double symErr = 0;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) symErr = std::max(symErr, std::abs(R[i][j] - R[j][i]));
        std::cout << "off-diagonal (0,1) zeroed and A' symmetric: "
                  << (zeroed && symErr < 1e-9 ? "PASS" : "FAIL")
                  << "  (|A'[0][1]| = " << std::abs(R[0][1]) << ", symErr = " << symErr << ")\n";
        ok &= zeroed && symErr < 1e-9;

        // ---- spectrum preserved: A' == J^T A J for the spec's rotation (host-recomputed) ----
        double app = A[0][0], aqq = A[1][1], apq = A[0][1];
        double tau = (aqq - app) / (2 * apq);
        double t = (tau > 0 ? 1.0 : -1.0) / (std::abs(tau) + std::sqrt(tau * tau + 1));
        double c = 1.0 / std::sqrt(t * t + 1), s = t * c;
        double J[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
        J[0][0] = c; J[1][1] = c; J[0][1] = s; J[1][0] = -s;   // rotation in the (0,1) plane
        double JtAJ[3][3] = {};
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j)
                for (size_t a = 0; a < N; ++a)
                    for (size_t b = 0; b < N; ++b) JtAJ[i][j] += J[a][i] * A[a][b] * J[b][j];
        double simErr = 0;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) simErr = std::max(simErr, std::abs(R[i][j] - JtAJ[i][j]));

        // trace and Frobenius (spectrum invariants under orthogonal similarity)
        double trA = 0, trR = 0, frA = 0, frR = 0;
        for (size_t i = 0; i < N; ++i) { trA += A[i][i]; trR += R[i][i]; }
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) { frA += A[i][j] * A[i][j]; frR += R[i][j] * R[i][j]; }
        bool spectrum = simErr < 1e-9 && std::abs(trA - trR) < 1e-9 && std::abs(frA - frR) < 1e-9;
        std::cout << "A' = J^T A J (spectrum preserved: trace " << trR << ", Frobenius^2 " << frR << "): "
                  << (spectrum ? "PASS" : "FAIL") << "  (||A' - JtAJ||_inf = " << simErr << ")\n";
        ok &= spectrum;

        // ---- the RDG is a SARE (fixed-index taps + indicator selects) ----
        Rdg g = buildRdg(spec, "eig_jacobi");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        std::cout << "RDG is a SARE (fixed-index rotation taps): "
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
