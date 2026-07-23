// rdg.hpp -- the Reduced Dependency Graph (RDG) of a SURE/SARE (issue #103).
//
// The RDG collapses the (unbounded) EXPANDED dependence graph -- one node per
// computed value, i.e. per lattice point per variable -- into a finite graph:
//   - one NODE per recurrence variable, and
//   - one ARC per variable-to-variable dependence.
// An equation  v(p) = f( ..., s(A*p + b), ... )  contributes, for each value it
// reads (each "tap"), an arc  s -> v  carrying the affine dependency map
// f(p) = A*p + b -- the producer point that a consumer at p reads. The arc is:
//   - UNIFORM  when A = I: a constant translation theta = p - (I*p + b) = -b, a
//     nearest-neighbour wire (Karp-Miller-Winograd UREs);
//   - AFFINE   when A != I: the operand index is an affine function of p (a
//     projection / permutation / broadcast), so the dependence vector p - (A*p + b)
//     grows with p. A single affine arc makes the whole system a SARE.
// The graph is schedule- and size-independent -- it is a property of the recurrence
// structure alone.
//
// This is the shared model behind `dfactl --emit-rdg` and the RDG regression test;
// the docs-site rdg.js viewer renders the emitted JSON.

#ifndef DFA_SIM_RDG_HPP
#define DFA_SIM_RDG_HPP

#include <string>
#include <vector>
#include <ostream>
#include <cstddef>
#include <dfa/sim/sure_parser.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // One arc s -> v of the RDG, labeled with its affine dependency map (A, b).
            struct RdgArc {
                std::string from;                       // producer variable (tap source)
                std::string to;                         // consumer variable (equation LHS)
                std::vector<std::vector<int>> A;        // dependency matrix
                std::vector<int> b;                     // dependency offset
                bool uniform;                           // A == I ?
                std::vector<int> theta;                 // -b when uniform; empty when affine
            };

            // The reduced dependency graph: variables (nodes) and dependences (arcs).
            struct Rdg {
                std::string op;
                std::size_t rank = 0;
                std::vector<std::string> indexNames;
                std::vector<std::string> variables;     // one node per recurrence variable
                std::vector<RdgArc> arcs;

                // A genuine SURE iff every arc is a uniform translation; any affine arc
                // (a projection/broadcast) marks the system a SARE.
                bool isSare() const {
                    for (const auto& a : arcs) if (!a.uniform) return true;
                    return false;
                }
            };

            // Build the RDG of a parsed SURE/SARE. Identical arcs (same source and same
            // map) are merged; discovery order is preserved for stable output.
            inline Rdg buildRdg(const SureSpec& sure, const std::string& opName) {
                Rdg g;
                g.op = opName;
                g.rank = sure.indexNames.size();
                g.indexNames = sure.indexNames;

                auto isIdentity = [rank = g.rank](const std::vector<std::vector<int>>& A) {
                    if (A.size() != rank) return false;
                    for (std::size_t r = 0; r < rank; ++r) {
                        if (A[r].size() != rank) return false;
                        for (std::size_t c = 0; c < rank; ++c)
                            if (A[r][c] != static_cast<int>(r == c ? 1 : 0)) return false;
                    }
                    return true;
                };

                for (const auto& kv : sure.system.equations()) {
                    g.variables.push_back(kv.first);
                    const std::string& name = kv.first;
                    for (const auto& eq : kv.second)            // a variable may have several branches
                    for (const auto& tap : eq.taps) {
                        const std::vector<std::vector<int>> A = tap.map.matrix();
                        const std::vector<int> b = tap.map.offset();
                        bool dup = false;
                        for (const auto& a : g.arcs)
                            if (a.from == tap.source && a.to == name && a.A == A && a.b == b) { dup = true; break; }
                        if (dup) continue;
                        RdgArc arc;
                        arc.from = tap.source; arc.to = name; arc.A = A; arc.b = b;
                        arc.uniform = isIdentity(A);
                        if (arc.uniform) {
                            arc.theta.resize(b.size());
                            for (std::size_t d = 0; d < b.size(); ++d) arc.theta[d] = -b[d];   // theta = -b
                        }
                        g.arcs.push_back(std::move(arc));
                    }
                }
                return g;
            }

            // Emit the RDG as JSON for the docs-site rdg.js viewer.
            inline void emitRdgJson(std::ostream& js, const Rdg& g) {
                auto arr = [&js](const auto& v) {
                    js << "[";
                    for (std::size_t i = 0; i < v.size(); ++i) js << v[i] << (i + 1 < v.size() ? "," : "");
                    js << "]";
                };
                js << "{\n";
                js << "  \"operator\": \"" << g.op << "\",\n";
                js << "  \"rank\": " << g.rank << ",\n";
                js << "  \"indexNames\": [";
                for (std::size_t i = 0; i < g.indexNames.size(); ++i)
                    js << "\"" << g.indexNames[i] << "\"" << (i + 1 < g.indexNames.size() ? "," : "");
                js << "],\n";
                js << "  \"kind\": \"" << (g.isSare() ? "SARE" : "SURE") << "\",\n";
                js << "  \"variables\": [";
                for (std::size_t i = 0; i < g.variables.size(); ++i)
                    js << "\"" << g.variables[i] << "\"" << (i + 1 < g.variables.size() ? "," : "");
                js << "],\n";
                js << "  \"arcs\": [\n";
                for (std::size_t i = 0; i < g.arcs.size(); ++i) {
                    const RdgArc& a = g.arcs[i];
                    js << "    {\"from\":\"" << a.from << "\",\"to\":\"" << a.to << "\",\"kind\":\""
                       << (a.uniform ? "uniform" : "affine") << "\",";
                    if (a.uniform) { js << "\"theta\":"; arr(a.theta); js << ","; }
                    js << "\"map\":{\"A\":[";
                    for (std::size_t r = 0; r < a.A.size(); ++r) { arr(a.A[r]); js << (r + 1 < a.A.size() ? "," : ""); }
                    js << "],\"b\":"; arr(a.b); js << "}}" << (i + 1 < g.arcs.size() ? "," : "") << "\n";
                }
                js << "  ]\n";
                js << "}\n";
            }

        } // namespace sim
    } // namespace dfa
} // namespace sw

#endif // DFA_SIM_RDG_HPP
