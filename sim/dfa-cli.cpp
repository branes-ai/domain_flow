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
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <dfa/sim/specs.hpp>
#include <dfa/sim/legality.hpp>
#include <dfa/sim/dfg_import.hpp>

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
            "  dfactl <file.dfg>           (same; positional .dfg is auto-detected)\n"
            "  dfactl --list\n"
            "  dfactl --help\n"
            "\n"
            "Options:\n"
            "  --schedule free|linear   free (ASAP) schedule [default], or the spec's linear tau\n"
            "  --tau t0,t1,...          override the linear scheduling vector (implies --schedule linear)\n"
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

} // namespace

int main(int argc, char** argv) {
    std::string specName;
    std::string dfgPath;
    std::string schedKind = "free";
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

    // A positional argument ending in .dfg is an import request.
    if (dfgPath.empty() && specName.size() > 4 && specName.substr(specName.size() - 4) == ".dfg") {
        dfgPath = specName;
        specName.clear();
    }
    if (!dfgPath.empty()) return runDfg(dfgPath, std::cout, quiet);

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
        std::cout << "\nschedule: free (ASAP)\n";
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
