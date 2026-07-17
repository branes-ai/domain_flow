// cg_sure.cpp
//
// Conjugate Gradient for SPD A x = b as a System of Affine Recurrence Equations
// (issue #52, docs/SURE/cg.sure). The most tightly coupled spec in the catalog: each CG
// step composes a SpMV (reduce over columns j), two dot products (reduce over components
// i), and three axpys, coupled by the scalars alpha^k = (r.r)/(p.Ap) and
// beta^k = (r'.r')/(r.r) over the iteration index k. The two reduction directions and the
// scalar broadcasts back to every component make CG a SARE.
//
// This test parses the executable doc and checks:
//   - the parsed structure and the derived output face normal
//   - the iterate X = x^{K-1} solves A x = b to machine precision (CG converges in <= N
//     steps for an N x N SPD system; K-1 = 4 >= N = 3)
//   - the RDG is a SARE (the affine scalar broadcasts and vector gathers)
//   - the free schedule is legal, and no linear tau is (the scalar-coupling fusion)

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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/cg.sure");

        // ---- parsed structure (free-only: no linear tau) ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= spec.tau.empty();
        ok &= (spec.outputs.size() == 1);   // X
        ok &= (spec.system.equations().size() == 9);   // am, ap, pApd, rrd, alp, bet, xv, rv, pv
        std::cout << "parsed structure (indices, no tau, 9 variables, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";
        ok &= (spec.outputs.front().normal == std::vector<int>{ 0, 1, 0 });   // X leaves on j = N-1
        std::cout << "output face normal (x leaves on j=N-1): "
                  << (spec.outputs.front().normal == std::vector<int>{ 0, 1, 0 } ? "PASS" : "FAIL") << "\n";

        // ---- run: X = x after the CG iterations, verify it solves A x = b ----
        const size_t N = 3;
        const double A[3][3] = { { 4, 1, 0 }, { 1, 4, 1 }, { 0, 1, 4 } };
        const double b[3]    = { 1, 2, 3 };
        double X[3] = {};
        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            X[idx[0]] = out.eval(sim, p);
        }

        double resid = 0;
        for (size_t i = 0; i < N; ++i) {
            double Ax = 0;
            for (size_t j = 0; j < N; ++j) Ax += A[i][j] * X[j];
            resid = std::max(resid, std::abs(Ax - b[i]));
        }
        bool solved = resid < 1e-9;   // CG is exact in <= N steps for SPD A
        std::cout << "CG solved A x = b: X = [" << X[0] << "," << X[1] << "," << X[2]
                  << "]  (||A X - b||_inf = " << resid << "): " << (solved ? "PASS" : "FAIL") << "\n";
        ok &= solved;

        // ---- the RDG is a SARE (scalar broadcasts + vector gathers) ----
        Rdg g = buildRdg(spec, "cg");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        std::cout << "RDG is a SARE (scalar broadcasts + gathers): "
                  << (g.isSare() && affine > 0 ? "PASS" : "FAIL")
                  << "  (" << g.arcs.size() << " arcs, " << affine << " affine)\n";
        ok &= g.isSare() && affine > 0;

        // ---- free schedule legal; no linear tau (the scalar-coupling fusion) ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        bool fl = checkLegality(spec.system, freeSched).legal;
        bool linRejected = !checkLegality(spec.system, LinearSchedule({ 1, 1, 1 })).legal;
        std::cout << "  free: " << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "free legal, linear tau rejected (free-only): "
                  << (fl && linRejected ? "PASS" : "FAIL") << "\n";
        ok &= fl && linRejected;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
