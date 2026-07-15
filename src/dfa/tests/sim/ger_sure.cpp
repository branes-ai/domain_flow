// ger_sure.cpp
//
// BLAS Level-2 rank-1 update as a System of Uniform Recurrence Equations (issue
// #32, docs/SURE/ger.sure):
//
//   A(i,j) := A(i,j) + alpha * x(i) * y(j),   0 <= i < M, 0 <= j < N
//
// ger is the outer-product counterpart to gemv: a 2-D fully-parallel update with
// no reduction. x broadcasts along +j, y along +i, and the matrix A — indexed by
// both domain axes and consumed once — enters and (updated) exits on the (i,j)
// face of a depth-2 feed/drain axis k: A seeds the accumulator on the k=-1 halo,
// the rank-1 term is added once (alpha is injected only on that halo), and the
// result drains out on the k=1 face.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - the numeric result A + alpha*x*y^T against a direct reference
//   - schedule legality (free and the canonical linear tau = [1,1,1])
//   - flux revalidation rejects a backward-flux tau
//   - memory analysis agrees with the eviction run

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/ger.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 1 });
        ok &= (spec.inputs.size() == 4);   // A, Alpha, X, Y
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, tau, 4 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        // ---- derived confluence orientation: outward face normals ----
        // A (the seed) and alpha enter on the (i,j) feed face k=-1; x on the j=-1
        // face, y on the i=-1 face; the updated matrix drains on the k=1 face.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "A" || in.tensor == "Alpha") nok &= (in.normal == std::vector<int>{ 0, 0, -1 });
            else if (in.tensor == "X")                    nok &= (in.normal == std::vector<int>{ 0, -1, 0 });
            else                                          nok &= (in.normal == std::vector<int>{ -1, 0, 0 }); // Y
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });
        std::cout << "derived face normals: " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: A(i,j) + alpha*x(i)*y(j) (mirrors the spec's data) ----
        const size_t M = 2, N = 3;
        const double A[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
        const double X[2] = { 10, 20 };
        const double Y[3] = { 1, 2, 3 };
        const double alpha = 2;
        double ref[2][3];
        for (size_t i = 0; i < M; ++i)
            for (size_t j = 0; j < N; ++j)
                ref[i][j] = A[i][j] + alpha * X[i] * Y[j];

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        size_t checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // Aout[i][j]
            double got = out.eval(sim, p);
            double want = ref[idx[0]][idx[1]];
            bool m = std::abs(got - want) < 1e-9;
            std::cout << "  Aout[" << idx[0] << "][" << idx[1] << "] = " << got
                      << "  (ref " << want << ")" << (m ? "" : "  MISMATCH") << "\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == M * N);   // a full matrix leaves the drain face
        std::cout << "parsed ger == A + alpha*x*y^T reference: " << (numok ? "PASS" : "FAIL") << "\n";
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
