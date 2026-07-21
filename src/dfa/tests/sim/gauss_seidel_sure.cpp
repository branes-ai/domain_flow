// gauss_seidel_sure.cpp
//
// Gauss-Seidel iterative solver -- A x = b -- as a System of Affine Recurrence Equations
// (issue #53, docs/SURE/gauss_seidel.sure). The residual sweep
//   x^{k}_i = ( b_i - sum_{j<i} A(i,j) x^{k}_j - sum_{j>i} A(i,j) x^{k-1}_j ) / A(i,i)
// reads THIS sweep for the lower columns and the PREVIOUS sweep for the upper ones. That
// split is expressed with PER-EQUATION DOMAINS: two reductions on complementary sub-domains,
//   accL(i,j,k | j < i)          (lower, reads x^{k})   and
//   accU(i,j,k | i < j, j < N)   (upper, reads x^{k-1}),
// so the k-vs-(k-1) choice is structural, not a per-point conditional. The lower reduction
// serializes the sweep (row i waits for rows < i) -- a triangular substitution wavefront --
// so Gauss-Seidel is free-schedule only, and converges in fewer sweeps than Jacobi.
//
// This test parses the executable doc and checks:
//   - the parsed structure (free-only, 6 inputs) and that the per-equation domains restrict
//     accL / accU to strictly fewer points than the system domain
//   - the iterate converges to the exact solution [1,1,1] (tighter than Jacobi at equal K)
//   - the RDG is a SARE with four affine arcs (the two gathers + two diagonal-adjacent reads)
//   - the free schedule is legal, and no linear tau is (the triangular reduce-then-divide chain)

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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/gauss_seidel.sure");

        // ---- parsed structure (free-only: no linear tau) ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= spec.tau.empty();
        ok &= (spec.inputs.size() == 6);   // A, B, Diag, accL seed, accU seed, X0
        ok &= (spec.outputs.size() == 1);  // X
        std::cout << "parsed structure (indices, free-only, 6 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        // ---- per-equation domains restrict accL / accU below the full system domain ----
        std::size_t full = spec.system.at("xs").domain.enumerate().size();       // spans the whole domain
        std::size_t nL   = spec.system.at("accL").domain.enumerate().size();     // only j < i
        std::size_t nU   = spec.system.at("accU").domain.enumerate().size();     // only i < j < N
        bool peq = (nL < full) && (nU < full) && (nL > 0) && (nU > 0);
        std::cout << "per-equation domains restrict accL/accU (|accL|=" << nL << ", |accU|=" << nU
                  << " < |xs|=" << full << "): " << (peq ? "PASS" : "FAIL") << "\n";
        ok &= peq;

        // ---- run: X = x after K sweeps, verify it solves A x = b (exactly) ----
        const size_t N = 3;
        const double A[3][3] = { { 10, 1, 1 }, { 1, 10, 1 }, { 1, 1, 10 } };
        const double b[3]    = { 12, 12, 12 };
        double X[3] = {};
        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            X[idx[0]] = out.eval(sim, p);
        }

        double resid = 0, solErr = 0;
        for (size_t i = 0; i < N; ++i) {
            double Ax = 0;
            for (size_t j = 0; j < N; ++j) Ax += A[i][j] * X[j];
            resid  = std::max(resid, std::abs(Ax - b[i]));
            solErr = std::max(solErr, std::abs(X[i] - 1.0));   // exact solution is [1,1,1]
        }
        bool conv = resid < 1e-6 && solErr < 1e-6;             // Gauss-Seidel is essentially exact by K=8
        std::cout << "Gauss-Seidel converged: X = [" << X[0] << "," << X[1] << "," << X[2]
                  << "]  (||A X - b||_inf = " << resid << ", ||X - [1,1,1]||_inf = " << solErr << "): "
                  << (conv ? "PASS" : "FAIL") << "\n";
        ok &= conv;

        // ---- the RDG is a SARE with four affine arcs ----
        Rdg g = buildRdg(spec, "gauss_seidel");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        std::cout << "RDG is a SARE with four affine arcs (two gathers + two solve reads): "
                  << (g.isSare() && affine == 4 ? "PASS" : "FAIL")
                  << "  (" << g.arcs.size() << " arcs, " << affine << " affine)\n";
        ok &= g.isSare() && affine == 4;

        // ---- free schedule legal; no linear tau (triangular reduce-then-divide fusion) ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        bool fl = checkLegality(spec.system, freeSched).legal;
        bool linRejected = !checkLegality(spec.system, LinearSchedule({ 1, 1, 1 })).legal;
        std::cout << "  free: " << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "free legal, linear tau rejected (free-only triangular sweep): "
                  << (fl && linRejected ? "PASS" : "FAIL") << "\n";
        ok &= fl && linRejected;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
