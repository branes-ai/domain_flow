// lu_neighbor_sure.cpp
//
// LU neighbour pivoting -- the pivot compare-exchange as a System of Uniform Recurrence
// Equations (issue #42, docs/SURE/lu_neighbor.sure).
//
// Partial pivoting selects a pivot by a global argmax over the column and moves row
// argmax to the diagonal -- an affine (broadcast) dependence that makes pivoted LU a
// SARE. NEIGHBOUR pivoting restricts every comparison and interchange to ADJACENT rows,
// so the pivot is chosen and moved by nearest-neighbour compare-exchanges: a bubble-down
// pass that carries the max-|.| value (and its row index) to the pivot position while the
// loser of each comparison settles in place. Every tap is a constant offset -- a genuine
// SURE (all translation vectors, no affine arc).
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - the selected pivot value and its ORIGINAL row index (nearest-neighbour argmax)
//   - the compare-exchanged column reconstructed from (Lo, Piv) equals a bubble-down pass
//     that brings the max magnitude to the pivot position
//   - the recurrence system is UNIFORM: buildRdg classifies it a SURE (0 affine arcs)
//   - free and the canonical linear tau = [1,1] are both legal

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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/lu_neighbor.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 4);   // X, Xi, piv seed, pidx seed
        ok &= (spec.outputs.size() == 3);  // Piv, Pidx, Lo
        std::cout << "parsed structure (indices, tau, 4 inputs, 3 outputs): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        // ---- derived confluence orientation: outward face normals ----
        // X/Xi enter on j=-1 (0,-1); the piv/pidx seeds on i=-1 (-1,0); the pivot leaves
        // on i=N-1 (1,0); the settled losers drain on j=1 (0,1).
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "X" || in.tensor == "Xi") nok &= (in.normal == std::vector<int>{ 0, -1 });
            else                                       nok &= (in.normal == std::vector<int>{ -1, 0 });
        }
        for (const auto& out : spec.outputs) {
            if (out.tensor == "Lo") nok &= (out.normal == std::vector<int>{ 0, 1 });
            else                    nok &= (out.normal == std::vector<int>{ 1, 0 });   // Piv, Pidx
        }
        std::cout << "derived face normals (feed j=-1, pivot i=N-1, drain j=1): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- evaluate the outputs ----
        const size_t N = 4;
        const double X[4] = { 1, -5, 3, -2 };
        double Piv = 0, Pidx = -1;
        double Lo[4] = {};

        SureSimulator<double> sim(spec.system);
        for (const auto& out : spec.outputs) {
            for (const auto& p : out.region.enumerate()) {
                std::vector<long> idx = out.elemIndex(p);
                double v = out.eval(sim, p);
                if (out.tensor == "Piv")       Piv = v;
                else if (out.tensor == "Pidx") Pidx = v;
                else                           Lo[idx[0]] = v;   // Lo[i]
            }
        }

        // ---- reference: bubble-down pass by magnitude (max -> pivot position N-1) ----
        double ref[4] = { X[0], X[1], X[2], X[3] };
        for (size_t i = 1; i < N; ++i)                 // one bubble-down pass
            if (std::abs(ref[i - 1]) > std::abs(ref[i])) std::swap(ref[i - 1], ref[i]);
        // reference pivot = the largest magnitude, from its original row
        size_t argmax = 0;
        for (size_t i = 1; i < N; ++i) if (std::abs(X[i]) > std::abs(X[argmax])) argmax = i;

        bool pivOk = (std::abs(Piv - X[argmax]) < 1e-9) && (static_cast<size_t>(Pidx + 0.5) == argmax);
        std::cout << "pivot selection: value " << Piv << " from row " << Pidx
                  << "  (expect " << X[argmax] << " from row " << argmax << "): "
                  << (pivOk ? "PASS" : "FAIL") << "\n";
        ok &= pivOk;

        // reconstruct the compare-exchanged column: [ Lo[1..N-1], Piv ]
        double y[4];
        for (size_t i = 0; i + 1 < N; ++i) y[i] = Lo[i + 1];
        y[N - 1] = Piv;
        double maxErr = 0;
        for (size_t i = 0; i < N; ++i) maxErr = std::max(maxErr, std::abs(y[i] - ref[i]));
        bool permOk = maxErr < 1e-9;
        std::cout << "compare-exchanged column [" << y[0] << "," << y[1] << "," << y[2] << "," << y[3]
                  << "] == bubble-down pass: " << (permOk ? "PASS" : "FAIL") << "  (max err " << maxErr << ")\n";
        ok &= permOk;

        // ---- the recurrence system is UNIFORM: a genuine SURE, no affine arc ----
        Rdg g = buildRdg(spec, "lu_neighbor");
        int affine = 0;
        for (const auto& a : g.arcs) if (!a.uniform) ++affine;
        bool sure = !g.isSare() && affine == 0;
        std::cout << "RDG is a SURE (neighbour pivoting uniformizes the pivot broadcast): "
                  << (sure ? "PASS" : "FAIL") << "  (" << g.arcs.size() << " arcs, " << affine << " affine)\n";
        ok &= sure;

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,1]
        bool sok = checkLegality(spec.system, freeSched).legal && checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1]: "   << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
