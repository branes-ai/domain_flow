// stationary_sure.cpp
//
// Stationary iterative solver -- Jacobi for A x = b -- as a System of Affine Recurrence
// Equations (issue #53, docs/SURE/stationary.sure). K fixed sweeps of the residual form
//   x^{k}_i = x^{k-1}_i + ( b_i - sum_j A(i,j) x^{k-1}_j ) / A(i,i),
// a gemv reduction over j plus an axpy update, iterated over k. Reading the whole
// previous iterate is the matrix-vector gather (one affine tap) -- a SARE, but a benign
// forward one (each sweep reads only sweep k-1). The residual divide is moved ONE PLANE UP
// (to the j = N update plane), so it reads the completed acc(i,N-1,k) as a translation
// [0,+1,0] instead of a [0,0,0] fusion at (i,N-1,k); with the other operands read one step
// back along their carries, every dependence has slack and the system is LINEARLY SCHEDULABLE
// (declared tau = [-1,1,2N]-family), unlike the fused reduce-then-divide form.
//
// This test parses the executable doc and checks:
//   - the parsed structure (a linear tau is declared) and the derived confluence face normals
//   - the iterate converges: X = x^{K-1} solves A x = b (small residual), and X ~ the
//     exact solution [1,1,1] of the bundled strongly-diagonally-dominant system
//   - the RDG is a SARE with exactly ONE affine arc (the x^{k-1} gather)
//   - BOTH the free schedule and the declared linear tau are legal (the [0,0,0] fusion is broken)

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

        // ---- parsed structure (a linear tau is now declared) ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ -1, 1, 14 });   // linear wavefront (was free-only)
        ok &= (spec.inputs.size() == 6);   // A, xs i=N halo, B, Diag, acc seed, X0
        ok &= (spec.outputs.size() == 1);  // X
        std::cout << "parsed structure (indices, linear tau, 6 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        // ---- derived confluence orientation: outward face normals ----
        // A/X0 held from k=-1 (0,0,-1); b/Diag/acc-seed on j=-1 (0,-1,0); the xs update-plane
        // halo on i=N (1,0,0); the result X on the j=N update plane (0,1,0).
        int nk = 0, njm = 0, ni = 0;
        for (const auto& in : spec.inputs) {
            if      (in.normal == std::vector<int>{ 0,  0, -1 }) ++nk;   // A, X0
            else if (in.normal == std::vector<int>{ 0, -1,  0 }) ++njm;  // B, Diag, acc seed
            else if (in.normal == std::vector<int>{ 1,  0,  0 }) ++ni;   // xs i=N halo
        }
        bool nok = (nk == 2) && (njm == 3) && (ni == 1)
                && (spec.outputs.front().normal == std::vector<int>{ 0, 1, 0 });
        std::cout << "derived face normals (A/x0 on k=-1, seeds on j=-1, halo on i=N, result on j=N): "
                  << (nok ? "PASS" : "FAIL") << "\n";
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

        // ---- the RDG is a SARE with exactly ONE affine arc (the x^{k-1} gather) ----
        Rdg g = buildRdg(spec, "stationary");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        std::cout << "RDG is a SARE with one affine arc (the matrix-vector gather of x^{k-1}): "
                  << (g.isSare() && affine == 1 ? "PASS" : "FAIL")
                  << "  (" << g.arcs.size() << " arcs, " << affine << " affine)\n";
        ok &= g.isSare() && affine == 1;

        // ---- BOTH the free schedule and the declared linear tau are legal ----
        // The plane-shift breaks the [0,0,0] fusion, so a linear tau now orders the system
        // (a naive tau like [1,1,1] is still rejected -- the affine gather needs tau_k >= 2N).
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        bool fl  = checkLegality(spec.system, freeSched).legal;
        bool lin = checkLegality(spec.system, LinearSchedule(spec.tau)).legal;
        bool naiveRejected = !checkLegality(spec.system, LinearSchedule({ 1, 1, 1 })).legal;
        std::cout << "  free:   " << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  linear: " << checkLegality(spec.system, LinearSchedule(spec.tau)) << "\n";
        std::cout << "free legal AND declared linear tau legal (fusion broken; naive tau still rejected): "
                  << (fl && lin && naiveRejected ? "PASS" : "FAIL") << "\n";
        ok &= fl && lin && naiveRejected;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
