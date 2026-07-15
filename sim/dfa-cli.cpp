// dfa-cli.cpp  --  dfactl, the Domain Flow Architecture SURE simulator CLI.
//
// Builds a System of Uniform/Affine Recurrence Equations, evaluates it to numeric
// output, then reports the schedule legality and the memory cardinality (peak
// number of simultaneously live values) under the chosen schedule.
//
// Usage:
//   dfactl <spec> [--schedule free|linear] [--tau t0,t1,...] [--quiet]
//   dfactl --list
//   dfactl --help
//
// <spec> is one of the built-in recurrence systems (matmul, matvec, qr).

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <dfa/sim/specs.hpp>
#include <dfa/sim/legality.hpp>
#include <dfa/sim/dfg_import.hpp>
#include <dfa/sim/sure_parser.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

namespace {

    void usage(std::ostream& os) {
        os <<
            "dfactl -- Domain Flow Architecture SURE simulator\n"
            "\n"
            "Usage:\n"
            "  dfactl <spec> [--schedule free|linear] [--tau t0,t1,...] [--quiet]\n"
            "  dfactl --dfg <file.dfg>      import a Domain Flow Graph and run it\n"
            "  dfactl --sure <file.sure>    parse and run a SURE DSL program (docs/SURE notation)\n"
            "  dfactl <file.dfg|file.sure> (same; positional files are auto-detected)\n"
            "  dfactl --list\n"
            "  dfactl --help\n"
            "\n"
            "Options:\n"
            "  --schedule free|linear   free schedule [default], or the spec's linear tau\n"
            "  --tau t0,t1,...          override the linear scheduling vector (implies --schedule linear)\n"
            "  --emit-schedule <file>   (--sure only) write the schedule as JSON for the docs-site\n"
            "                           3-D wavefront animation, then exit\n"
            "  --quiet                  suppress the numeric output, report schedule/memory only\n";
    }

    void listSpecs(std::ostream& os) {
        os << "Available specs:\n";
        for (const auto& name : specNames()) {
            try {
                Spec s = buildSpec(name);
                os << "  " << s.name << "\t" << s.description << "\n";
            } catch (const std::exception& e) {
                std::cerr << "error: spec '" << name << "': " << e.what() << "\n";
            }
        }
    }

    bool parseTau(const std::string& csv, std::vector<int>& out) {
        out.clear();
        std::stringstream ss(csv);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            try { out.push_back(std::stoi(tok)); }
            catch (...) { return false; }
        }
        return !out.empty();
    }

    // Import a .dfg / MLIR-derived graph, report coverage, and (if small enough)
    // execute it under the free schedule.
    int runDfg(const std::string& path, std::ostream& os, bool quiet) {
        ImportResult ir;
        try { ir = importDfgFile(path); }
        catch (const std::exception& e) {
            std::cerr << "error: failed to load '" << path << "': " << e.what() << "\n";
            return 2;
        }
        os << "imported .dfg: " << path << "\n" << ir.report;
        os << "  index-space size (points): " << ir.indexSpaceSize << "\n";
        os << "  outputs:";
        for (const auto& o : ir.outputs) {
            os << " " << o.var << "[";
            for (std::size_t i = 0; i < o.shape.size(); ++i) os << o.shape[i] << (i + 1 < o.shape.size() ? "x" : "");
            os << "](" << o.nodeName << ")";
        }
        os << "\n";
        if (ir.outputs.empty()) { os << "no runnable outputs found.\n"; return 1; }

        constexpr long long CAP = 200000;
        if (ir.indexSpaceSize > CAP) {
            os << "\nindex space exceeds " << CAP << " points; structural import only (skipping execution).\n";
            return 0;
        }

        SureSimulator<double> sim(ir.system);
        if (!quiet) for (const auto& o : ir.outputs) {
            long long total = 1;
            for (int d : o.shape) total *= (d > 0 ? d : 1);
            long long limit = std::min<long long>(total, 16);
            os << "\noutput " << o.var << " (" << total << " elements, showing " << limit << "):\n";
            std::vector<int> idx(o.shape.size(), 0);
            for (long long n = 0; n < limit; ++n) {
                double v = sim.eval(o.var, IndexPoint(idx));
                os << "  " << o.var << "(";
                for (std::size_t i = 0; i < idx.size(); ++i) os << idx[i] << (i + 1 < idx.size() ? "," : "");
                os << ") = " << v << "\n";
                for (int d = static_cast<int>(idx.size()) - 1; d >= 0; --d) {
                    if (++idx[d] < o.shape[d]) break;
                    idx[d] = 0;
                }
            }
        }

        SureSimulator<double> sim2(ir.system);
        ExplicitSchedule s = sim2.computeFreeSchedule();
        os << "\nfree schedule: " << checkLegality(ir.system, s) << "\n";
        LivenessReport r = sim2.analyzeMemory(s);
        long peak = 0;
        sim2.run(s, &peak);
        os << "memory: peakLiveValues=" << r.peakLiveValues << "  latency=" << r.latency
           << "  work=" << r.totalValues << "  (footprint=" << peak << ")\n";
        return (peak == r.peakLiveValues) ? 0 : 2;
    }

    // Report legality + memory cardinality for a schedule, and execute it if legal.
    int analyzeAndRun(SureSimulator<double>& sim, const RecurrenceSystem<double>& sys,
                      const ISchedule& sched, std::ostream& os) {
        LegalityReport lr = checkLegality(sys, sched);
        os << "\nSchedule legality: " << lr << "\n";
        if (!lr.legal) {
            os << "Schedule is illegal; skipping execution.\n";
            return 1;
        }
        LivenessReport r = sim.analyzeMemory(sched);
        os << "Memory cardinality: peakLiveValues=" << r.peakLiveValues
           << "  latency=" << r.latency << "  work=" << r.totalValues << "\n";
        os << "  per-variable peak live: ";
        for (const auto& [var, pk] : r.peakPerVar) os << var << "=" << pk << " ";
        os << "\n";
        long observed = 0;
        sim.run(sched, &observed);   // eviction-based execution; re-validates legality
        os << "  realized footprint (with eviction): " << observed << "\n";
        return (observed == r.peakLiveValues) ? 0 : 2;
    }

    // Emit the schedule as JSON for the docs-site 3-D wavefront animation (issue
    // #64): every variable's index points tagged with their firing time -- the
    // domain-flow signature t = tau.p under a linear schedule, or the data-flow-
    // earliest time under the free schedule -- plus the index-space bounds and the
    // total latency, so a viewer can sweep the wavefront through the lattice.
    void emitScheduleJson(std::ostream& js, const SureSpec& sure, const ISchedule& sched,
                          const std::string& kind, const std::vector<int>& tau,
                          const std::string& opName) {
        const std::size_t rank = sure.indexNames.size();
        std::vector<long> lo(rank, std::numeric_limits<long>::max());
        std::vector<long> hi(rank, std::numeric_limits<long>::min());
        long tmin = std::numeric_limits<long>::max(), tmax = std::numeric_limits<long>::min();

        // A dependence tap: which producer this variable reads, and the affine map
        // f(p) = A*p + b from a consumer point to the producer point it reads. The
        // viewer replays these to find edges that cross a tile boundary (halo vs
        // collective) — uniform taps give A = I (short, nearest-neighbour), affine
        // taps (a projection/permutation) can jump across many tiles (long-range).
        struct TapDump { std::string source; std::vector<std::vector<int>> A; std::vector<int> b; };
        struct VarDump { std::string name; std::vector<std::pair<IndexPoint, long>> pts; std::vector<TapDump> taps; };
        std::vector<VarDump> vars;
        for (const auto& [name, eq] : sure.system.equations()) {
            VarDump vd; vd.name = name;
            for (const auto& tap : eq.taps)
                vd.taps.push_back({ tap.source, tap.map.matrix(), tap.map.offset() });
            for (const auto& p : eq.domain.enumerate()) {
                long t = sched.time(name, p);
                vd.pts.emplace_back(p, t);
                tmin = std::min(tmin, t); tmax = std::max(tmax, t);
                for (std::size_t d = 0; d < rank && d < p.size(); ++d) {
                    lo[d] = std::min(lo[d], static_cast<long>(p[d]));
                    hi[d] = std::max(hi[d], static_cast<long>(p[d]));
                }
            }
            vars.push_back(std::move(vd));
        }
        if (tmin > tmax) { tmin = tmax = 0; }
        for (std::size_t d = 0; d < rank; ++d)
            if (lo[d] > hi[d]) { lo[d] = 0; hi[d] = 0; }

        auto arr = [&js](const auto& v) {
            js << "[";
            for (std::size_t i = 0; i < v.size(); ++i) js << v[i] << (i + 1 < v.size() ? "," : "");
            js << "]";
        };

        js << "{\n";
        js << "  \"operator\": \"" << opName << "\",\n";
        js << "  \"rank\": " << rank << ",\n";
        js << "  \"indexNames\": [";
        for (std::size_t i = 0; i < sure.indexNames.size(); ++i)
            js << "\"" << sure.indexNames[i] << "\"" << (i + 1 < sure.indexNames.size() ? "," : "");
        js << "],\n";
        js << "  \"schedule\": {\"kind\": \"" << kind << "\"";
        if (kind == "linear") { js << ", \"tau\": "; arr(tau); }
        js << "},\n";
        js << "  \"bounds\": {\"lo\": "; arr(lo); js << ", \"hi\": "; arr(hi); js << "},\n";
        js << "  \"latency\": " << (tmax - tmin + 1) << ",\n";
        js << "  \"variables\": [\n";
        for (std::size_t v = 0; v < vars.size(); ++v) {
            js << "    {\"name\": \"" << vars[v].name << "\", \"taps\": [";
            for (std::size_t k = 0; k < vars[v].taps.size(); ++k) {
                const TapDump& t = vars[v].taps[k];
                js << "{\"source\":\"" << t.source << "\",\"A\":[";
                for (std::size_t r = 0; r < t.A.size(); ++r) { arr(t.A[r]); js << (r + 1 < t.A.size() ? "," : ""); }
                js << "],\"b\":"; arr(t.b); js << "}" << (k + 1 < vars[v].taps.size() ? "," : "");
            }
            js << "], \"points\": [";
            for (std::size_t k = 0; k < vars[v].pts.size(); ++k) {
                const IndexPoint& p = vars[v].pts[k].first;
                js << "{\"p\":[";
                for (std::size_t d = 0; d < p.size(); ++d) js << p[d] << (d + 1 < p.size() ? "," : "");
                js << "],\"t\":" << vars[v].pts[k].second << "}" << (k + 1 < vars[v].pts.size() ? "," : "");
            }
            js << "]}" << (v + 1 < vars.size() ? "," : "") << "\n";
        }
        js << "  ]\n";
        js << "}\n";
    }

    // Parse a .sure program and write its schedule JSON (--emit-schedule).
    int runSureEmit(const std::string& path, const std::string& emitPath,
                    const std::string& schedKind, const std::vector<int>& tauOverride, bool haveTau) {
        SureSpec sure;
        try { sure = parseSureFile(path); }
        catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 2; }

        std::string op = path;
        auto slash = op.find_last_of("/\\");
        if (slash != std::string::npos) op = op.substr(slash + 1);
        if (op.size() > 5 && op.substr(op.size() - 5) == ".sure") op = op.substr(0, op.size() - 5);

        SureSimulator<double> sim(sure.system);
        std::vector<int> tau = haveTau ? tauOverride : sure.tau;

        std::ofstream out(emitPath);
        if (!out) { std::cerr << "error: cannot write '" << emitPath << "'\n"; return 2; }

        if (schedKind == "free" || tau.empty()) {
            ExplicitSchedule s = sim.computeFreeSchedule();
            emitScheduleJson(out, sure, s, "free", {}, op);
        } else {
            LinearSchedule s(tau);
            emitScheduleJson(out, sure, s, "linear", tau, op);
        }
        std::cout << "wrote schedule JSON: " << emitPath << "\n";
        return 0;
    }

    // Parse a .sure DSL program (the docs/SURE notation), evaluate its outputs,
    // and analyze the requested schedule.
    int runSure(const std::string& path, std::ostream& os, bool quiet,
                const std::string& schedKind, const std::vector<int>& tauOverride, bool haveTau) {
        SureSpec sure;
        try { sure = parseSureFile(path); }
        catch (const std::exception& e) {
            std::cerr << "error: " << e.what() << "\n";
            return 2;
        }
        os << "sure program: " << path << "\n";
        os << "  system indices: (";
        for (std::size_t i = 0; i < sure.indexNames.size(); ++i)
            os << sure.indexNames[i] << (i + 1 < sure.indexNames.size() ? "," : "");
        os << ")  variables:";
        for (const auto& kv : sure.system.equations()) os << " " << kv.first;
        os << "\n";

        // confluence structure: tensors bound to oriented faces of the domain
        auto printNormal = [&os](const std::vector<int>& n) {
            os << "normal (";
            for (std::size_t i = 0; i < n.size(); ++i) os << n[i] << (i + 1 < n.size() ? "," : "");
            os << ")";
        };
        for (const auto& in : sure.inputs) {
            os << "  input  " << (in.tensor.empty() ? "(const)" : in.tensor)
               << " -> " << in.var << "  ";
            printNormal(in.normal);
            os << "\n";
        }
        for (const auto& out : sure.outputs) {
            os << "  output " << out.tensor << " <- " << out.var << "  ";
            printNormal(out.normal);
            os << "\n";
        }

        SureSimulator<double> sim(sure.system);
        if (!quiet) {
            for (const auto& out : sure.outputs) {
                for (const auto& p : out.region.enumerate()) {
                    std::vector<long> idx = out.elemIndex(p);
                    double v = out.eval(sim, p);
                    os << "  " << out.tensor;
                    for (long ix : idx) os << "[" << ix << "]";
                    os << " = " << v << "\n";
                }
            }
        }

        int rc = 0;
        std::vector<int> tau = haveTau ? tauOverride : sure.tau;
        if (schedKind == "free" || tau.empty()) {
            if (schedKind != "free" && tau.empty())
                os << "\nnote: program declares no tau; using free.\n";
            os << "\nschedule: free\n";
            ExplicitSchedule s = sim.computeFreeSchedule();
            rc = analyzeAndRun(sim, sure.system, s, os);
        } else {
            os << "\nschedule: linear tau=[";
            for (std::size_t i = 0; i < tau.size(); ++i) os << tau[i] << (i + 1 < tau.size() ? "," : "");
            os << "]\n";
            try {
                // an overriding --tau must also respect the declared confluence
                // flow directions, not just the dependency slacks
                validateSureFlux(sure, tau);
                LinearSchedule s(tau);
                rc = analyzeAndRun(sim, sure.system, s, os);
            } catch (const std::exception& e) {
                os << "\n" << e.what() << "\n";   // illegal schedule or flux violation
                rc = 1;
            }
        }
        os << "\n" << (rc == 0 ? "OK" : "FAILED") << "\n";
        return rc;
    }

} // namespace

int main(int argc, char** argv) {
    std::string specName;
    std::string dfgPath;
    std::string surePath;
    std::string schedKind = "free";
    std::string emitPath;
    std::vector<int> tauOverride;
    bool haveTau = false;
    bool quiet = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") { usage(std::cout); return 0; }
        if (a == "--list")              { listSpecs(std::cout); return 0; }
        if (a == "--quiet")             { quiet = true; continue; }
        if (a == "--dfg") {
            if (++i >= argc) { std::cerr << "error: --dfg needs a file path\n"; return 2; }
            dfgPath = argv[i];
            continue;
        }
        if (a == "--sure") {
            if (++i >= argc) { std::cerr << "error: --sure needs a file path\n"; return 2; }
            surePath = argv[i];
            continue;
        }
        if (a == "--emit-schedule") {
            if (++i >= argc) { std::cerr << "error: --emit-schedule needs a file path\n"; return 2; }
            emitPath = argv[i];
            continue;
        }
        if (a == "--schedule") {
            if (++i >= argc) { std::cerr << "error: --schedule needs an argument\n"; return 2; }
            schedKind = argv[i];
            if (schedKind != "free" && schedKind != "linear") {
                std::cerr << "error: --schedule must be 'free' or 'linear'\n"; return 2;
            }
            continue;
        }
        if (a == "--tau") {
            if (++i >= argc) { std::cerr << "error: --tau needs an argument\n"; return 2; }
            if (!parseTau(argv[i], tauOverride)) { std::cerr << "error: bad --tau '" << argv[i] << "'\n"; return 2; }
            haveTau = true;
            schedKind = "linear";
            continue;
        }
        if (!a.empty() && a[0] == '-') { std::cerr << "error: unknown option '" << a << "'\n"; usage(std::cerr); return 2; }
        if (specName.empty()) { specName = a; continue; }
        std::cerr << "error: unexpected argument '" << a << "'\n"; return 2;
    }

    // A positional argument ending in .dfg or .sure is a file request.
    if (dfgPath.empty() && specName.size() > 4 && specName.substr(specName.size() - 4) == ".dfg") {
        dfgPath = specName;
        specName.clear();
    }
    if (surePath.empty() && specName.size() > 5 && specName.substr(specName.size() - 5) == ".sure") {
        surePath = specName;
        specName.clear();
    }
    if (!dfgPath.empty() && !surePath.empty()) {
        std::cerr << "error: --dfg and --sure are mutually exclusive\n";
        return 2;
    }
    if (!emitPath.empty()) {
        if (surePath.empty()) { std::cerr << "error: --emit-schedule requires --sure <file>\n"; return 2; }
        return runSureEmit(surePath, emitPath, schedKind, tauOverride, haveTau);
    }
    if (!dfgPath.empty()) return runDfg(dfgPath, std::cout, quiet);
    if (!surePath.empty()) return runSure(surePath, std::cout, quiet, schedKind, tauOverride, haveTau);

    if (specName.empty()) { usage(std::cerr); return 2; }

    Spec spec;
    try { spec = buildSpec(specName); }
    catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 2; }

    std::cout << "spec: " << spec.name << "  (" << spec.description << ")\n";

    SureSimulator<double> sim(spec.system);

    // 1. numeric evaluation (schedule-independent, via the memoized core)
    bool outputsOk = true;
    if (!quiet) outputsOk = spec.printOutputs(sim, std::cout);

    // 2. build the requested schedule
    int rc = 0;
    if (schedKind == "free") {
        std::cout << "\nschedule: free\n";
        ExplicitSchedule s = sim.computeFreeSchedule();
        rc = analyzeAndRun(sim, spec.system, s, std::cout);
    } else {
        std::vector<int> tau = haveTau ? tauOverride : spec.tau;
        if (tau.empty()) {
            std::cout << "\nnote: '" << spec.name
                      << "' has no canonical linear schedule (heterogeneous index spaces); using free.\n";
            ExplicitSchedule s = sim.computeFreeSchedule();
            rc = analyzeAndRun(sim, spec.system, s, std::cout);
        } else {
            std::cout << "\nschedule: linear tau=[";
            for (std::size_t i = 0; i < tau.size(); ++i) std::cout << tau[i] << (i + 1 < tau.size() ? "," : "");
            std::cout << "]";
            if (!spec.beta.empty()) {
                std::cout << " beta{";
                for (const auto& [v, b] : spec.beta) std::cout << v << "=" << b << " ";
                std::cout << "}";
            }
            std::cout << "\n";
            try {
                LinearSchedule s(tau, spec.beta);
                rc = analyzeAndRun(sim, spec.system, s, std::cout);
            } catch (const std::exception& e) {
                std::cout << "\n" << e.what() << "\n";   // run() rejected an illegal schedule
                rc = 1;
            }
        }
    }

    bool ok = outputsOk && (rc == 0);
    std::cout << "\n" << (ok ? "OK" : "FAILED") << "\n";
    return ok ? 0 : 1;
}
