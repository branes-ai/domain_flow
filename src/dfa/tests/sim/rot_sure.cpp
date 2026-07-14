// rot_sure.cpp
//
// BLAS Level-1 Givens rotation as a System of Uniform Recurrence Equations
// (issue #29, docs/SURE/rot.md).  rot applies a plane rotation to a vector pair:
//
//   ( Xout )   ( c  s ) ( x )
//   ( Yout ) = (-s  c ) ( y )   elementwise, for each i.
//
// It is axpy generalized to a 2x2 linear map: two projected scalars (c, s), two
// injected operands (x, y), two result streams (rx, ry):
//
//   rx(i,j) = rx(i,j-1) + c(i-1,j)*x(i,j-1) + s(i-1,j)*y(i,j-1);   //  c*x + s*y
//   ry(i,j) = ry(i,j-1) - s(i-1,j)*x(i,j-1) + c(i-1,j)*y(i,j-1);   // -s*x + c*y
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the six input + two output confluences and their face normals
//   - the rotated result against the reference matrix, per lane
//   - the norm-preserving property Xout^2 + Yout^2 == x^2 + y^2 (c^2+s^2=1)
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

// docs/SURE/rot.sure is loaded from the source tree (SURE_DOCS_DIR is set by
// CMake) so the executable doc stays the single source of truth.
int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/rot.sure");

        // ---- parsed structure: six inputs (C,S,X,Y and two seeds), two outputs ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 6);
        ok &= (spec.outputs.size() == 2);

        // ---- derived confluence orientation: outward face normals ----
        // c, s are projected onto the i = -1 edge; the operands and result seeds
        // enter on the -j halo; both results leave the +j face.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "C" || in.tensor == "S")
                nok &= (in.normal == std::vector<int>{ -1, 0 });   // projected scalars
            else
                nok &= (in.normal == std::vector<int>{ 0, -1 });   // X, Y, rx-seed, ry-seed
        }
        for (const auto& out : spec.outputs)
            nok &= (out.normal == std::vector<int>{ 0, 1 });
        std::cout << "derived face normals (C,S proj; X,Y,seeds in; Xout,Yout out): "
                  << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- rotated result + norm preservation (c, s, X, Y mirror the spec) ----
        const double c = 0.6, s = 0.8;   // c^2 + s^2 = 1
        const double X[4] = { 1, 2, 3, 4 };
        const double Y[4] = { 5, 6, 7, 8 };

        SureSimulator<double> sim(spec.system);
        std::vector<double> Xo(4, 0), Yo(4, 0);
        for (const auto& out : spec.outputs) {
            std::vector<double>& dst = (out.tensor == "Xout") ? Xo : Yo;
            for (const auto& p : out.region.enumerate()) {
                std::vector<long> idx = out.elemIndex(p);
                dst[static_cast<std::size_t>(idx[0])] = out.eval(sim, p);
            }
        }
        for (int i = 0; i < 4; ++i) {
            double refX =  c * X[i] + s * Y[i];
            double refY = -s * X[i] + c * Y[i];
            bool m = std::abs(Xo[i] - refX) < 1e-9 && std::abs(Yo[i] - refY) < 1e-9;
            // a rotation preserves the per-lane norm
            double n0 = X[i] * X[i] + Y[i] * Y[i];
            double n1 = Xo[i] * Xo[i] + Yo[i] * Yo[i];
            bool norm = std::abs(n0 - n1) < 1e-9;
            std::cout << "  (Xout,Yout)[" << i << "] = (" << Xo[i] << "," << Yo[i] << ")"
                      << "  ref (" << refX << "," << refY << ")"
                      << "  ||.||^2 " << n1 << " vs " << n0
                      << (m && norm ? "" : "  MISMATCH") << "\n";
            ok &= (m && norm);
        }
        std::cout << "parsed rot == Givens rotation (+ norm-preserving): " << (ok ? "PASS" : "FAIL") << "\n";

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
