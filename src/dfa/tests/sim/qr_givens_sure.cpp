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
// This test parses the executable docs and checks:
//   - square 3x3: R is upper triangular and matches the exact reference
//   - the sign-robust invariant R^T R == A^T A (R is a valid QR factor) on BOTH
//     the square 3x3 (qr_givens.sure) and a tall 4x3 (qr_givens_tall.sure) input
//   - free-schedule legality of the parsed system

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

// Parse a Givens-QR spec, run the free schedule, and read the N x N upper factor
// R out of its output confluence (lower triangle stays 0). Also reports whether
// the parsed system's free schedule is legal.
static std::vector<std::vector<double>> factorR(const std::string& file, size_t N, bool& legal) {
    SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/" + file);
    SureSimulator<double> sim(spec.system);
    std::vector<std::vector<double>> R(N, std::vector<double>(N, 0.0));
    const SureOutput& out = spec.outputs.front();
    for (const auto& p : out.region.enumerate()) {
        std::vector<long> idx = out.elemIndex(p);
        R[idx[0]][idx[1]] = out.eval(sim, p);
    }
    ExplicitSchedule freeSched = sim.computeFreeSchedule();
    legal = checkLegality(spec.system, freeSched).legal;
    return R;
}

// R^T R == A^T A (the sign-robust QR invariant): max abs error over the Gram matrix.
static double gramError(const std::vector<std::vector<double>>& R,
                        const std::vector<std::vector<double>>& A, size_t N) {
    double maxErr = 0;
    for (size_t a = 0; a < N; ++a)
        for (size_t b = 0; b < N; ++b) {
            double rtr = 0, ata = 0;
            for (size_t k = 0; k < R.size(); ++k) rtr += R[k][a] * R[k][b];
            for (size_t k = 0; k < A.size(); ++k) ata += A[k][a] * A[k][b];
            maxErr = std::max(maxErr, std::abs(rtr - ata));
        }
    return maxErr;
}

int main() {
    bool ok = true;
    try {
        // ── square 3x3: exact R + upper-triangular + R^T R == A^T A ──
        const size_t N = 3;
        const std::vector<std::vector<double>> A3 = {
            { 12, -51, 4 }, { 6, 167, -68 }, { -4, 24, -41 } };
        const double Rref[3][3] = { { 14, 21, -14 }, { 0, 175, -70 }, { 0, 0, 35 } };

        bool legal3 = false;
        auto R3 = factorR("qr_givens.sure", N, legal3);
        bool exact = true, upper = true;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) {
                if (j < i && std::abs(R3[i][j]) > 1e-9) upper = false;
                if (std::abs(R3[i][j] - Rref[i][j]) > 1e-6) exact = false;
            }
        std::cout << "3x3  R upper-triangular: " << (upper ? "PASS" : "FAIL")
                  << " ;  R == exact reference: " << (exact ? "PASS" : "FAIL") << "\n";
        ok &= upper && exact;

        double err3 = gramError(R3, A3, N);
        std::cout << "3x3  R^T R == A^T A: " << (err3 < 1e-6 ? "PASS" : "FAIL")
                  << "  (max err " << err3 << ")\n";
        ok &= err3 < 1e-6;

        // ── tall 4x3 (over-determined): upper-triangular + R^T R == A^T A ──
        const std::vector<std::vector<double>> A4 = {
            { 12, -51, 4 }, { 6, 167, -68 }, { -4, 24, -41 }, { 1, 2, 3 } };
        bool legal4 = false;
        auto R4 = factorR("qr_givens_tall.sure", N, legal4);
        bool upper4 = true;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j)
                if (j < i && std::abs(R4[i][j]) > 1e-9) upper4 = false;
        double err4 = gramError(R4, A4, N);
        std::cout << "4x3  R upper-triangular: " << (upper4 ? "PASS" : "FAIL")
                  << " ;  R^T R == A^T A: " << (err4 < 1e-6 ? "PASS" : "FAIL")
                  << "  (max err " << err4 << ")\n";
        ok &= upper4 && err4 < 1e-6;

        // ── free-schedule legality of both parsed (affine) systems ──
        std::cout << "free schedule legal: 3x3 " << (legal3 ? "PASS" : "FAIL")
                  << " ;  4x3 " << (legal4 ? "PASS" : "FAIL") << "\n";
        ok &= legal3 && legal4;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
