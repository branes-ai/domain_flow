// stationary_sure.cpp
//
// Stationary iterative solver -- Jacobi for A x = b -- as a System of Affine Recurrence
// Equations (issue #53, docs/SURE/stationary.sure). K fixed sweeps of the residual form
//   x^{k}_i = x^{k-1}_i + ( b_i - sum_j A(i,j) x^{k-1}_j ) / A(i,i),
// a gemv reduction over j plus an axpy update, iterated over k. Reading the whole
// previous iterate is the matrix-vector gather (affine taps) -- a SARE, but a benign
// forward one (each sweep reads only sweep k-1). Free-schedule only: like the triangular
// solve, each row's reduction and its residual divide fuse at (i,N-1,k).
//
// This test parses the executable doc and checks:
//   - the parsed structure and the derived confluence face normals
//   - the iterate converges: X = x^{K-1} solves A x = b (small residual), and X ~ the
//     exact solution [1,1,1] of the bundled strongly-diagonally-dominant system
//   - the RDG is a SARE (the affine x^{k-1} gather)
//   - the free schedule is legal, and no linear tau is (the reduce-then-divide fusion)

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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/stationary.sure");

        // ---- parsed structure (free-only: no linear tau declared) ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= spec.tau.empty();
        ok &= (spec.inputs.size() == 5);   // A, B, Diag, acc seed, X0
        ok &= (spec.outputs.size() == 1);  // X
        std::cout << "parsed structure (indices, no tau, 5 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        // ---- derived confluence orientation: outward face normals ----
        // A and X0 held from k=-1 (0,0,-1); b/Diag/seed on j=-1 (0,-1,0); X on j=N-1 (0,1,0).
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "A" || in.tensor == "X0") nok &= (in.normal == std::vector<int>{ 0, 0, -1 });
            else                                       nok &= (in.normal == std::vector<int>{ 0, -1, 0 });
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 1, 0 });
        std::cout << "derived face normals (A/x0 on k=-1, result on j=N-1): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- run: X = x after K sweeps, verify it solves A x = b ----
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
        bool conv = resid < 1e-3 && solErr < 1e-3;
        std::cout << "Jacobi converged: X = [" << X[0] << "," << X[1] << "," << X[2]
                  << "]  (||A X - b||_inf = " << resid << ", ||X - [1,1,1]||_inf = " << solErr << "): "
                  << (conv ? "PASS" : "FAIL") << "\n";
        ok &= conv;

        // ---- the RDG is a SARE (the affine x^{k-1} gather) ----
        Rdg g = buildRdg(spec, "stationary");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        std::cout << "RDG is a SARE (affine matrix-vector gather of x^{k-1}): "
                  << (g.isSare() && affine > 0 ? "PASS" : "FAIL")
                  << "  (" << g.arcs.size() << " arcs, " << affine << " affine)\n";
        ok &= g.isSare() && affine > 0;

        // ---- free schedule legal; no linear tau (reduce-then-divide fusion) ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        bool fl = checkLegality(spec.system, freeSched).legal;
        bool linRejected = !checkLegality(spec.system, LinearSchedule({ 1, 1, 1 })).legal;
        std::cout << "  free: " << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "free legal, linear tau rejected (free-only, as for trsv): "
                  << (fl && linRejected ? "PASS" : "FAIL") << "\n";
        ok &= fl && linRejected;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
