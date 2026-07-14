// copy_sure.cpp
//
// BLAS Level-1 vector copy as a System of Uniform Recurrence Equations (issue
// #28, docs/SURE/copy.md).  copy  y := x  is the identity flow -- the simplest
// operator in the catalog:
//
//   x(i,j) = x(i,j-1);         // value-preserving flow carrying X
//   output Y[i] = x(i,j);      // X leaves the terminal face as Y
//
// A single value-preserving recurrence carries X[i] from the j = -1 input halo
// to the terminal j = 1 face; no arithmetic, one tensor in, one tensor out.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the single input + single output confluence and their face normals
//   - the identity result (Y == X)
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

// docs/SURE/copy.sure is loaded from the source tree (SURE_DOCS_DIR is set by
// CMake) so the executable doc stays the single source of truth.
int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/copy.sure");

        // ---- parsed structure: one input, one output ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 1);    // X
        ok &= (spec.outputs.size() == 1);   // Y

        // ---- derived confluence orientation: outward face normals ----
        bool nok = true;
        nok &= (spec.inputs.front().normal == std::vector<int>{ 0, -1 });    // inject X
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 1 });    // drain Y
        std::cout << "derived face normals (X in; Y out): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- identity result: Y == X ----
        const double X[4] = { 1, 2, 3, 4 };

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        int checked = 0;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            double got = out.eval(sim, p);
            bool m = std::abs(got - X[idx[0]]) < 1e-9;
            std::cout << "  Y[" << idx[0] << "] = " << got << "  (ref " << X[idx[0]] << ")"
                      << (m ? "" : "  MISMATCH") << "\n";
            ok &= m;
            ++checked;
        }
        ok &= (checked == 4);
        std::cout << "parsed copy == identity (Y == X): " << (ok ? "PASS" : "FAIL") << "\n";

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
