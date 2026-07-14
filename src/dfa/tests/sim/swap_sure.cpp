// swap_sure.cpp
//
// BLAS Level-1 vector exchange as a System of Uniform Recurrence Equations
// (issue #27, docs/SURE/swap.md).  swap  (x, y) := (y, x)  has no arithmetic:
// two value-preserving flows carry X and Y straight through, and the exchange is
// realized entirely in the OUTPUT confluences -- crossed routing.
//
//   x(i,j) = x(i,j-1);   // carry X
//   y(i,j) = y(i,j-1);   // carry Y
//   output Xout[i] = y(i,j);   // first slot receives Y
//   output Yout[i] = x(i,j);   // second slot receives X
//
// It is the multi-tensor confluence: two input faces in, two output faces out.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the two input + two output confluences and their face normals
//   - the crossed result (Xout == Y, Yout == X) against the inputs
//   - schedule legality (free and the canonical linear tau = [1,1])
//   - memory analysis agrees with the eviction run

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

// docs/SURE/swap.sure is loaded from the source tree (SURE_DOCS_DIR is set by
// CMake) so the executable doc stays the single source of truth.
int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/swap.sure");

        // ---- parsed structure: two inputs, two outputs ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 2);    // X, Y
        ok &= (spec.outputs.size() == 2);   // Xout, Yout

        // ---- derived confluence orientation: outward face normals ----
        // both operands are injected on the -j halo; both results leave the +j face
        bool nok = true;
        for (const auto& in : spec.inputs)
            nok &= (in.normal == std::vector<int>{ 0, -1 });
        for (const auto& out : spec.outputs)
            nok &= (out.normal == std::vector<int>{ 0, 1 });
        std::cout << "derived face normals (X,Y in; Xout,Yout out): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- crossed result: Xout == Y, Yout == X ----
        const double X[4] = {  1,  2,  3,  4 };
        const double Y[4] = { 10, 20, 30, 40 };

        SureSimulator<double> sim(spec.system);
        int checkedXout = 0, checkedYout = 0;
        for (const auto& out : spec.outputs) {
            const double* ref = (out.tensor == "Xout") ? Y : X;   // crossed
            int* counter      = (out.tensor == "Xout") ? &checkedXout : &checkedYout;
            for (const auto& p : out.region.enumerate()) {
                std::vector<long> idx = out.elemIndex(p);
                double got = out.eval(sim, p);
                bool m = std::abs(got - ref[idx[0]]) < 1e-9;
                std::cout << "  " << out.tensor << "[" << idx[0] << "] = " << got
                          << "  (ref " << ref[idx[0]] << ")" << (m ? "" : "  MISMATCH") << "\n";
                ok &= m;
                ++(*counter);
            }
        }
        ok &= (checkedXout == 4 && checkedYout == 4);
        std::cout << "parsed swap == crossed (Xout=Y, Yout=X): " << (ok ? "PASS" : "FAIL") << "\n";

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,1]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "      << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1]: " << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis on parsed system: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;

        // ---- a backward-flux override must be rejected by flux revalidation ----
        bool fok = false;
        try { validateSureFlux(spec, { 1, -1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation for overridden tau=[1,-1]: " << (fok ? "PASS" : "FAIL") << "\n";
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
