// svd_sure.cpp
//
// Singular Value Decomposition, one-sided Jacobi -- one column-orthogonalizing rotation --
// as a System of Affine Recurrence Equations (issue #46, docs/SURE/svd.sure). One-sided
// Jacobi sweeps column pairs (p,q), rotating columns p,q of A so they become orthogonal;
// at convergence the column norms are the singular values. This spec is one such rotation
// on the fixed pair (0,1): the angle comes from the 2x2 Gram matrix (three dot reductions
// over rows), and the update right-multiplies by J (only columns 0,1 change) -- a SARE.
//
// This test parses the executable doc and checks:
//   - the parsed structure and the derived output face normal
//   - columns 0 and 1 of A' become orthogonal, and column 2 is unchanged
//   - A' = A J for the J the spec computes, so the singular values are preserved
//     (right-multiplying by an orthogonal J leaves them invariant -> Frobenius invariant)
//   - the RDG is a SARE (the Gram reductions + fixed-column taps)
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/svd.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.outputs.size() == 1);   // R
        std::cout << "parsed structure (indices, 1 output): " << (ok ? "PASS" : "FAIL") << "\n";
        bool nok = (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });   // R drains on k=1
        std::cout << "output face normal (R drains on k=1): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- evaluate A' = R ----
        const size_t M = 4, N = 3;
        const double A[4][3] = { { 1, 2, 0 }, { 2, 1, 1 }, { 0, 1, 3 }, { 1, 0, 2 } };
        double R[4][3] = {};
        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            R[idx[0]][idx[1]] = out.eval(sim, p);
        }

        // ---- columns 0 and 1 orthogonalized; column 2 unchanged ----
        double dot01 = 0, col2Err = 0;
        for (size_t i = 0; i < M; ++i) { dot01 += R[i][0] * R[i][1]; col2Err = std::max(col2Err, std::abs(R[i][2] - A[i][2])); }
        bool orth = std::abs(dot01) < 1e-9 && col2Err < 1e-9;
        std::cout << "columns 0,1 orthogonal (a0'.a1' = " << dot01 << "), column 2 unchanged: "
                  << (orth ? "PASS" : "FAIL") << "\n";
        ok &= orth;

        // ---- A' == A J for the spec's rotation (host-recomputed), singular values preserved ----
        double alpha = 0, beta = 0, gamma = 0;
        for (size_t i = 0; i < M; ++i) { alpha += A[i][0] * A[i][0]; beta += A[i][1] * A[i][1]; gamma += A[i][0] * A[i][1]; }
        double zeta = (beta - alpha) / (2 * gamma);
        double t = (zeta > 0 ? 1.0 : -1.0) / (std::abs(zeta) + std::sqrt(zeta * zeta + 1));
        double c = 1.0 / std::sqrt(t * t + 1), s = t * c;
        double AJ[4][3];
        for (size_t i = 0; i < M; ++i) {
            AJ[i][0] = c * A[i][0] - s * A[i][1];
            AJ[i][1] = s * A[i][0] + c * A[i][1];
            AJ[i][2] = A[i][2];
        }
        double simErr = 0, frA = 0, frR = 0;
        for (size_t i = 0; i < M; ++i)
            for (size_t j = 0; j < N; ++j) {
                simErr = std::max(simErr, std::abs(R[i][j] - AJ[i][j]));
                frA += A[i][j] * A[i][j]; frR += R[i][j] * R[i][j];
            }
        bool preserved = simErr < 1e-9 && std::abs(frA - frR) < 1e-9;
        std::cout << "A' = A J (singular values preserved: Frobenius^2 " << frR << " == " << frA << "): "
                  << (preserved ? "PASS" : "FAIL") << "  (||A' - AJ||_inf = " << simErr << ")\n";
        ok &= preserved;

        // ---- the RDG is a SARE ----
        Rdg g = buildRdg(spec, "svd");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        std::cout << "RDG is a SARE (Gram reductions + fixed-column taps): "
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
