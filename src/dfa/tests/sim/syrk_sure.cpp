// syrk_sure.cpp
//
// BLAS Level-3 symmetric rank-k update as a System of Uniform Recurrence Equations
// (issue #38, docs/SURE/syrk.sure):
//
//   C(i,j) := beta*C(i,j) + alpha*sum_k A(i,k) A(j,k)   on the triangular output j<=i.
//
// syrk is gemm with B = A^T (a single operand A feeding both taps) and a triangular
// output. The A(j,k) tap propagates +i; on the triangular domain it enters on the
// super-diagonal halo i-j=-1 (its +i pipeline would otherwise read outside the
// triangle). alpha is folded into the reduction; beta*C_in joins the completed sum in
// a terminal-face epilogue on k=K-1. It is the SPD-forming kernel behind blocked
// Cholesky's trailing update.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - the numeric result beta*Cin + alpha*A*A^T (lower) against a reference
//   - schedule legality (free and the canonical linear tau = [2,1,1])
//   - flux revalidation rejects the box-default tau = [1,1,1]
//   - memory analysis agrees with the eviction run

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/syrk.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 2, 1, 1 });
        ok &= (spec.inputs.size() == 6);   // A->a, A->b, Alpha, Beta, Cin, seed
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, tau, 6 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("syrk: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // A->a on j=-1; A->b on the super-diagonal i-j=-1; alpha/beta/Cin/seed on
        // k=-1; the result exits on k=K-1.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "A" && in.normal == std::vector<int>{ 0, -1, 0 }) continue;      // a feed
            if (in.tensor == "A" && in.normal == std::vector<int>{ -1, 1, 0 }) continue;      // b feed (super-diagonal)
            if (in.tensor == "A") { nok = false; continue; }
            nok &= (in.normal == std::vector<int>{ 0, 0, -1 });                               // Alpha, Beta, Cin, seed
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });
        std::cout << "derived face normals (incl. super-diagonal b feed): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: beta*Cin + alpha*(A A^T) on j<=i (mirrors the spec) ----
        const size_t N = 3, K = 2;
        const double A[3][2] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
        const double Cin[3][3] = { { 1, 0, 0 }, { 2, 3, 0 }, { 4, 5, 6 } };
        const double alpha = 2, beta = 10;
        double ref[3][3] = {};
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j <= i; ++j) {
                double s = 0;
                for (size_t k = 0; k < K; ++k) s += A[i][k] * A[j][k];
                ref[i][j] = beta * Cin[i][j] + alpha * s;
            }

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        size_t checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // C[i][j], j <= i
            double got = out.eval(sim, p);
            double want = ref[idx[0]][idx[1]];
            bool m = std::abs(got - want) < 1e-9;
            std::cout << "  C[" << idx[0] << "][" << idx[1] << "] = " << got
                      << "  (ref " << want << ")" << (m ? "" : "  MISMATCH") << "\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == N * (N + 1) / 2);   // lower triangle size
        std::cout << "parsed syrk == beta*Cin + alpha*A*A^T (lower): " << (numok ? "PASS" : "FAIL") << "\n";
        ok &= numok;

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [2,1,1]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[2,1,1]: " << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis on parsed system: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;

        // ---- the box-default tau=[1,1,1] must be rejected: the super-diagonal b
        //      feed needs tau_i > tau_j, so tau_i == tau_j gives non-negative flux ----
        bool fok = false;
        try { validateSureFlux(spec, { 1, 1, 1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation rejects tau=[1,1,1] (super-diagonal): " << (fok ? "PASS" : "FAIL") << "\n";
        ok &= fok;

        // ---- memory analysis agrees with the eviction run ----
        LivenessReport lr = sim.analyzeMemory(good);
        long peak = 0;
        sim.run(good, &peak);
        bool mok = (peak == lr.peakLiveValues);
        std::cout << "memory analysis vs eviction run: " << (mok ? "PASS" : "FAIL")
                  << "  (peakLiveValues=" << lr.peakLiveValues << ")\n";
        ok &= mok;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
