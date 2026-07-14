// qr_givens_sure.cpp
//
// QR by Givens rotations (the Gentleman-Kung systolic array) as a recurrence
// system (issue #72, docs/SURE/qr_givens.sure) — the uniform-in-hardware
// alternative to the Modified Gram-Schmidt qr.sure.
//
// Each row of A is merged into the accumulating upper-triangular R by a sweep of
// Givens rotations over the index space (i,p,q), p <= q:
//   r(i,p,q) = (rold*R + ain*a)/den ;  a(i,p,q) = (rold*a - ain*R)/den
// with rold = r(i-1,p,p), ain = a(i,p-1,p), den = sqrt(rold^2+ain^2). There is no
// reduction — the rotations are nearest-neighbour.
//
// This test parses the executable doc and checks:
//   - R is upper triangular and matches the exact reference for the classic 3x3
//   - the sign-robust invariant R^T R == A^T A (R is a valid QR factor)
//   - free-schedule legality of the parsed system

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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/qr_givens.sure");
        const int N = 3;
        const double A[3][3] = { { 12, -51, 4 }, { 6, 167, -68 }, { -4, 24, -41 } };

        // read R from the output confluence (lower triangle stays 0)
        double R[3][3] = {};
        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            R[idx[0]][idx[1]] = out.eval(sim, p);
        }

        // exact reference R for this A (Givens reproduces it here)
        const double Rref[3][3] = { { 14, 21, -14 }, { 0, 175, -70 }, { 0, 0, 35 } };
        bool exact = true, upper = true;
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j) {
                if (j < i && std::abs(R[i][j]) > 1e-9) upper = false;
                if (std::abs(R[i][j] - Rref[i][j]) > 1e-6) exact = false;
            }
        std::cout << "R upper-triangular: " << (upper ? "PASS" : "FAIL")
                  << " ;  R == exact reference: " << (exact ? "PASS" : "FAIL") << "\n";
        ok &= upper && exact;

        // sign-robust: R^T R == A^T A (R is a valid QR factor for any QR)
        double maxErr = 0;
        for (int a = 0; a < N; ++a)
            for (int b = 0; b < N; ++b) {
                double rtr = 0, ata = 0;
                for (int k = 0; k < N; ++k) { rtr += R[k][a] * R[k][b]; ata += A[k][a] * A[k][b]; }
                maxErr = std::max(maxErr, std::abs(rtr - ata));
            }
        bool factor = maxErr < 1e-6;
        std::cout << "R^T R == A^T A: " << (factor ? "PASS" : "FAIL") << "  (max err " << maxErr << ")\n";
        ok &= factor;

        // free-schedule legality of the parsed (affine) system
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        bool legal = checkLegality(spec.system, freeSched).legal;
        std::cout << "free schedule: " << checkLegality(spec.system, freeSched) << "\n";
        ok &= legal;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
