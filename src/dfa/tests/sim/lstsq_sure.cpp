// lstsq_sure.cpp
//
// Least squares via QR -- min ||A x - b||_2 for a tall A -- as a System of Affine
// Recurrence Equations (issue #50, docs/SURE/lstsq.sure). The first multi-operator
// pipeline in the catalog: it AUGMENTS b as an extra column of A and runs the Givens QR
// on [A | b], so the same rotations that triangularize A carry its extra column into
// Q^T b. One pass yields R (the N x N upper-triangular factor) and c = Q^T b -- the
// "QR -> apply Q^T" stages fused, with no explicit Q. The remaining stage is the
// triangular back-substitution R x = c (trsv's upper-triangular mirror), done here.
//
// Verified WITHOUT Q by the sign-robust invariants:
//   - R^T R == A^T A         (R is a valid QR factor of A)
//   - R^T c == A^T b         (c = Q^T b, since R^T c = R^T Q^T b = (QR)^T b = A^T b)
//   - back-substitute R x = c, then A^T A x == A^T b  (the normal equations) -> x is the
//     least-squares solution.
// Plus: the parsed structure and face normals, that the RDG is a SARE (the affine
// rotation broadcast, like qr_givens), and free + linear tau = [1,1,1] both legal.

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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/lstsq.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "p", "q" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 1 });
        ok &= (spec.inputs.size() == 3);   // r seed, A, B
        ok &= (spec.outputs.size() == 2);  // R, C
        std::cout << "parsed structure (indices, tau, 3 inputs, 2 outputs): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        // ---- derived confluence orientation: outward face normals ----
        // r seed on i=-1 (-1,0,0); A and b augmented on p=-1 (0,-1,0); R and c leave on
        // i=M-1 (1,0,0).
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "A" || in.tensor == "B") nok &= (in.normal == std::vector<int>{ 0, -1, 0 });
            else                                      nok &= (in.normal == std::vector<int>{ -1, 0, 0 });   // r seed
        }
        for (const auto& out : spec.outputs) nok &= (out.normal == std::vector<int>{ 1, 0, 0 });   // R, C
        std::cout << "derived face normals (seed i=-1, feed p=-1, result i=M-1): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- the data (must match the spec's data bindings) ----
        const size_t M = 4, N = 3;
        const double A[4][3] = { { 1, 1, 1 }, { 1, 2, 4 }, { 1, 3, 9 }, { 1, 4, 16 } };
        const double b[4]    = { 2, 3, 5, 9 };

        // ---- evaluate R and c ----
        double R[3][3] = {}, c[3] = {};
        SureSimulator<double> sim(spec.system);
        for (const auto& out : spec.outputs) {
            for (const auto& p : out.region.enumerate()) {
                std::vector<long> idx = out.elemIndex(p);
                double v = out.eval(sim, p);
                if (out.tensor == "R") R[idx[0]][idx[1]] = v;
                else                   c[idx[0]] = v;   // C[p]
            }
        }

        // ---- reference normal-equation quantities A^T A and A^T b ----
        double AtA[3][3] = {}, Atb[3] = {};
        for (size_t r = 0; r < N; ++r)
            for (size_t s = 0; s < N; ++s)
                for (size_t i = 0; i < M; ++i) AtA[r][s] += A[i][r] * A[i][s];
        for (size_t r = 0; r < N; ++r)
            for (size_t i = 0; i < M; ++i) Atb[r] += A[i][r] * b[i];

        // ---- invariant 1: R^T R == A^T A ----
        double e1 = 0;
        for (size_t r = 0; r < N; ++r)
            for (size_t s = 0; s < N; ++s) {
                double v = 0;
                for (size_t t = 0; t < N; ++t) v += R[t][r] * R[t][s];
                e1 = std::max(e1, std::abs(v - AtA[r][s]));
            }
        std::cout << "R^T R == A^T A (valid QR factor): " << (e1 < 1e-5 ? "PASS" : "FAIL")
                  << "  (max err " << e1 << ")\n";
        ok &= e1 < 1e-5;

        // ---- invariant 2: R^T c == A^T b  (c = Q^T b, no Q formed) ----
        double e2 = 0;
        for (size_t r = 0; r < N; ++r) {
            double v = 0;
            for (size_t t = 0; t < N; ++t) v += R[t][r] * c[t];
            e2 = std::max(e2, std::abs(v - Atb[r]));
        }
        std::cout << "R^T c == A^T b (c = Q^T b, Q never formed): " << (e2 < 1e-5 ? "PASS" : "FAIL")
                  << "  (max err " << e2 << ")\n";
        ok &= e2 < 1e-5;

        // ---- back-substitute R x = c, then check the normal equations A^T A x == A^T b ----
        double x[3] = {};
        for (long r = static_cast<long>(N) - 1; r >= 0; --r) {
            double s = c[r];
            for (size_t t = r + 1; t < N; ++t) s -= R[r][t] * x[t];
            x[r] = s / R[r][r];
        }
        double e3 = 0;
        for (size_t r = 0; r < N; ++r) {
            double v = 0;
            for (size_t s = 0; s < N; ++s) v += AtA[r][s] * x[s];
            e3 = std::max(e3, std::abs(v - Atb[r]));
        }
        std::cout << "back-substituted x = [" << x[0] << "," << x[1] << "," << x[2]
                  << "] satisfies A^T A x == A^T b: " << (e3 < 1e-6 ? "PASS" : "FAIL")
                  << "  (max err " << e3 << ")\n";
        ok &= e3 < 1e-6;

        // ---- the RDG is a SARE (the affine rotation broadcast, as in qr_givens) ----
        Rdg g = buildRdg(spec, "lstsq");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        std::cout << "RDG is a SARE (affine rotation broadcast): " << (g.isSare() && affine > 0 ? "PASS" : "FAIL")
                  << "  (" << g.arcs.size() << " arcs, " << affine << " affine)\n";
        ok &= g.isSare() && affine > 0;

        // ---- schedule legality: free and the canonical linear tau = [1,1,1] ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);
        bool sok = checkLegality(spec.system, freeSched).legal && checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1,1]: " << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
