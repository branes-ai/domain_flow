// nrm2_sure.cpp
//
// BLAS Level-1 Euclidean norm as a System of Uniform Recurrence Equations (issue
// #24, docs/SURE/nrm2.md).  nrm2  ||x||_2 = sqrt(sum_i X[i]^2)  is a reduction
// with a pointwise epilogue: the sum of squares accumulates along +i exactly
// like the dot product,
//
//   s(i,j) = s(i-1,j) + x(i,j-1) * x(i,j-1);   // seed s(-1,.) = 0
//
// and the scalar norm leaves through the single terminal point i = N-1, where
// the OUTPUT FACE applies sqrt as a fused epilogue (R[0] = sqrt(s)).  The single
// operand X is indexed by the reduction coordinate, so it enters on a depth-1
// FEED axis j (X[i] injected on the j = -1 halo).
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the derived confluence structure (input/output face normals)
//   - the numeric result ||x||_2 (with the sqrt epilogue) against a reference
//   - schedule legality (free and the canonical linear tau = [1,1])
//   - the reduction footprint: peak live values is O(1) (an accumulator)

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

// docs/SURE/nrm2.sure is loaded from the source tree (SURE_DOCS_DIR is set by
// CMake) so the executable doc stays the single source of truth.
int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/nrm2.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 2);   // X and the accumulator seed
        ok &= (spec.outputs.size() == 1);

        // ---- derived confluence orientation: outward face normals ----
        // X is fed on the -j halo; the accumulator seed enters on the -i face;
        // the scalar norm exits on the +i terminal face.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "X") nok &= (in.normal == std::vector<int>{ 0, -1 });   // feed
            else                  nok &= (in.normal == std::vector<int>{ -1, 0 });   // seed (const)
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 1, 0 });            // result
        std::cout << "derived face normals (X,seed,R): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: ||x||_2 = sqrt(sum_i X[i]^2) (X mirrors the spec) ----
        const double X[4] = { 1, 2, 2, 4 };
        double sumsq = 0;
        for (int i = 0; i < 4; ++i) sumsq += X[i] * X[i];
        const double ref = std::sqrt(sumsq);   // sqrt(25) = 5

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        int checked = 0;
        for (const auto& p : out.region.enumerate()) {
            double got = out.eval(sim, p);     // the output face applies the sqrt epilogue
            bool m = std::abs(got - ref) < 1e-9;
            std::cout << "  R[0] = " << got << "  (ref " << ref << ")" << (m ? "" : "  MISMATCH") << "\n";
            ok &= m;
            ++checked;
        }
        ok &= (checked == 1);   // a scalar leaves through a single terminal point
        std::cout << "parsed nrm2 == ||x||_2 reference (sqrt epilogue): " << (ok ? "PASS" : "FAIL") << "\n";

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
        try { validateSureFlux(spec, { -1, 1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation for overridden tau=[-1,1]: " << (fok ? "PASS" : "FAIL") << "\n";
        ok &= fok;

        // ---- reduction footprint: an O(1) accumulator, analysis == run ----
        LivenessReport lr = sim.analyzeMemory(good);
        long peak = 0;
        sim.run(good, &peak);
        bool mok = (peak == lr.peakLiveValues) && (lr.peakLiveValues <= 2);
        std::cout << "memory analysis vs eviction run: " << (mok ? "PASS" : "FAIL")
                  << "  (peakLiveValues=" << lr.peakLiveValues << ", accumulator footprint)\n";
        ok &= mok;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
