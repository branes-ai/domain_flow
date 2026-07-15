// gemv_sure.cpp
//
// BLAS Level-2 general matrix-vector product as a System of Uniform Recurrence
// Equations (issue #31, docs/SURE/gemv.sure):
//
//   y(i) = beta * y(i) + alpha * sum_j A(i,j) * x(j),   0 <= i < M, 0 <= j < N
//
// gemv is matmul with the second matrix collapsed to a vector x: a reduction over
// the column j with x reused across rows (pipelined along +i) and A consumed once
// (entering on the (i,j) face of a depth-1 feed axis k). The scalars alpha and beta
// are projected onto faces and consumed as products of recurrence variables
// (project-and-pipeline); alpha folds into the reduction and beta*y(i) combines with
// the completed sum in a terminal-face epilogue on the output face j = N-1.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - the numeric result y = beta*y + alpha*A*x against a direct reference
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/gemv.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 1 });
        ok &= (spec.inputs.size() == 6);   // A, X, Alpha, Beta, Yin, and the accumulator seed
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, tau, 6 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        // ---- derived confluence orientation: outward face normals ----
        // A and alpha enter on the (i,j) feed face k=-1; x and beta on the i=-1 face;
        // y_in and the accumulator seed on the reduction-start face j=-1; the result
        // exits on the terminal reduction face j=N-1.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "A" || in.tensor == "Alpha")     nok &= (in.normal == std::vector<int>{ 0, 0, -1 });
            else if (in.tensor == "X" || in.tensor == "Beta") nok &= (in.normal == std::vector<int>{ -1, 0, 0 });
            else                                              nok &= (in.normal == std::vector<int>{ 0, -1, 0 }); // Yin, seed
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 1, 0 });
        std::cout << "derived face normals: " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: y = beta*y + alpha*A*x (mirrors the spec's data) ----
        const int M = 3, N = 4;
        const double A[3][4] = { { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 } };
        const double X[4] = { 1, 2, 3, 4 };
        const double alpha = 2, beta = 10;
        const double Yin[3] = { 1, 2, 3 };
        double ref[3];
        for (int i = 0; i < M; ++i) {
            double s = 0;
            for (int j = 0; j < N; ++j) s += A[i][j] * X[j];
            ref[i] = beta * Yin[i] + alpha * s;
        }

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        int checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // Y[i]
            double got = out.eval(sim, p);
            bool m = std::abs(got - ref[idx[0]]) < 1e-9;
            std::cout << "  Y[" << idx[0] << "] = " << got << "  (ref " << ref[idx[0]] << ")"
                      << (m ? "" : "  MISMATCH") << "\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == M);   // one result per row
        std::cout << "parsed gemv == beta*y + alpha*A*x reference: " << (numok ? "PASS" : "FAIL") << "\n";
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
