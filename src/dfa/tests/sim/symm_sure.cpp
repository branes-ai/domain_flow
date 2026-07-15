// symm_sure.cpp
//
// BLAS Level-3 symmetric matrix-matrix product as a System of Uniform Recurrence
// Equations (issue #41, docs/SURE/symm.sure):
//
//   C(i,j) := beta*C(i,j) + alpha*sum_k A(i,k) B(k,j),   A = A^T, one stored triangle.
//
// symm is gemm for a symmetric A stored as a single (lower) triangle. The symmetry
// lives in the confluence: a two-face feed for the a-tap reads each stored element
// two ways -- the lower cells read A[i][k], the upper cells read the reflected
// A[k][i] -- so cell (i,j,k) always sees A_sym(i,k) and the reduction is the ordinary
// gemm sweep (the symv confluence pattern on gemm's 3-D cube).
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals (two A feeds)
//   - the numeric result against a dense symmetric reference (reflected from lower)
//   - schedule legality (free and the canonical linear tau = [1,1,1])
//   - flux revalidation rejects a backward-flux tau
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/symm.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 1 });
        ok &= (spec.inputs.size() == 7);   // two A feeds, B, Alpha, Beta, Cin, seed
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, tau, 7 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("symm: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // Both A feeds on j=-1; B and beta on i=-1; alpha/Cin/seed on k=-1; the result
        // exits on the terminal k=K-1.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "A")                             nok &= (in.normal == std::vector<int>{ 0, -1, 0 });
            else if (in.tensor == "B" || in.tensor == "Beta") nok &= (in.normal == std::vector<int>{ -1, 0, 0 });
            else                                              nok &= (in.normal == std::vector<int>{ 0, 0, -1 }); // Alpha, Cin, seed
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });
        std::cout << "derived face normals (two symmetric A feeds): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: beta*Cin + alpha*A_sym*B (A_sym reflected from lower) ----
        const size_t M = 3, N = 2, K = 3;
        const double Alo[3][3] = { { 2, 0, 0 }, { 3, 4, 0 }, { 5, 6, 7 } };  // stored lower triangle
        double Asym[3][3];
        for (size_t i = 0; i < M; ++i)
            for (size_t k = 0; k < K; ++k)
                Asym[i][k] = (k <= i) ? Alo[i][k] : Alo[k][i];   // reflect
        const double B[3][2] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
        const double Cin[3][2] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
        const double alpha = 2, beta = 10;
        double ref[3][2];
        for (size_t i = 0; i < M; ++i)
            for (size_t j = 0; j < N; ++j) {
                double s = 0;
                for (size_t k = 0; k < K; ++k) s += Asym[i][k] * B[k][j];
                ref[i][j] = beta * Cin[i][j] + alpha * s;
            }

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        size_t checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // C[i][j]
            double got = out.eval(sim, p);
            double want = ref[idx[0]][idx[1]];
            bool m = std::abs(got - want) < 1e-9;
            std::cout << "  C[" << idx[0] << "][" << idx[1] << "] = " << got
                      << "  (ref " << want << ")" << (m ? "" : "  MISMATCH") << "\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == M * N);
        std::cout << "parsed symm == beta*Cin + alpha*A_sym*B reference: " << (numok ? "PASS" : "FAIL") << "\n";
        ok &= numok;

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,1,1]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1,1]: " << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis on parsed system: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;

        // ---- a backward-flux override must be rejected by flux revalidation ----
        bool fok = false;
        try { validateSureFlux(spec, { -1, -1, -1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation for overridden tau=[-1,-1,-1]: " << (fok ? "PASS" : "FAIL") << "\n";
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
