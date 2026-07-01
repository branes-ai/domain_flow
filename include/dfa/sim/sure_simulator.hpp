#pragma once
#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <dfa/sim/recurrence_system.hpp>
#include <dfa/sim/schedule_strategy.hpp>
#include <dfa/sim/legality.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // Result of the cardinality / memory analysis for a given schedule.
            //   peakLiveValues : the maximum number of values that must be held
            //                    simultaneously -- the number of memories the schedule needs.
            //   peakPerVar     : the same, attributed per recurrence variable.
            //   latency        : span of the schedule in timesteps.
            //   totalValues    : number of computed values (work).
            struct LivenessReport {
                long peakLiveValues = 0;
                std::map<std::string, long> peakPerVar;
                long latency = 0;
                long totalValues = 0;
            };

            template<typename Value = double>
            class SureSimulator {
            public:
                struct Key {
                    std::string var;
                    IndexPoint p;
                    bool operator<(const Key& o) const {
                        return var < o.var || (var == o.var && p < o.p);
                    }
                };

            private:
                const RecurrenceSystem<Value>& sys_;
                std::map<Key, Value> memo_;

                // last-use (death) time of every computed value under a schedule
                std::map<Key, long> deathTimes(const ISchedule& sched) const {
                    std::map<Key, long> death;
                    for (const auto& [name, eq] : sys_.equations()) {
                        for (const auto& q : eq.domain.enumerate()) {
                            long tq = sched.time(name, q);
                            for (const auto& t : eq.taps) {
                                IndexPoint prod = t.map.apply(q);
                                if (sys_.has(t.source) && sys_.at(t.source).domain.isInside(prod)) {
                                    Key pk{ t.source, prod };
                                    auto it = death.find(pk);
                                    if (it == death.end() || tq > it->second) death[pk] = tq;
                                }
                            }
                        }
                    }
                    return death;
                }

            public:
                explicit SureSimulator(const RecurrenceSystem<Value>& sys) : sys_(sys) {}

                // ---------------------------------------------------------------
                // Correctness core: lazy, memoized evaluation of the recurrence.
                // A tap that resolves outside its producer's domain is a boundary
                // (operand value / initial condition).  Correct for any legal SURE
                // regardless of schedule.
                // ---------------------------------------------------------------
                Value eval(const std::string& var, const IndexPoint& p) {
                    const auto& eq = sys_.at(var);
                    if (!eq.domain.isInside(p)) return eq.boundary(p);

                    Key k{ var, p };
                    auto it = memo_.find(k);
                    if (it != memo_.end()) return it->second;

                    std::vector<Value> tv;
                    tv.reserve(eq.taps.size());
                    for (const auto& t : eq.taps)
                        tv.push_back(eval(t.source, t.map.apply(p)));

                    Value v = eq.compute(tv, p);
                    memo_[k] = v;
                    return v;
                }
                void clearMemo() { memo_.clear(); }

                // ---------------------------------------------------------------
                // Free schedule (ASAP): the data-flow earliest time of every value.
                // t = 0 at boundaries, t = 1 + max(producer times) otherwise.
                // Minimum latency, maximum parallelism (widest wavefronts).
                // ---------------------------------------------------------------
                ExplicitSchedule computeFreeSchedule() const {
                    ExplicitSchedule sched;
                    std::map<Key, long> depth;
                    std::function<long(const std::string&, const IndexPoint&)> visit =
                        [&](const std::string& var, const IndexPoint& p) -> long {
                            const auto& eq = sys_.at(var);
                            if (!eq.domain.isInside(p)) return 0;          // boundary ready at t=0
                            Key k{ var, p };
                            auto it = depth.find(k);
                            if (it != depth.end()) return it->second;
                            long d = 0;
                            for (const auto& t : eq.taps)
                                d = std::max(d, visit(t.source, t.map.apply(p)) + 1);
                            depth[k] = d;
                            sched.set(var, p, d);
                            return d;
                        };
                    for (const auto& [name, eq] : sys_.equations())
                        for (const auto& p : eq.domain.enumerate())
                            visit(name, p);
                    return sched;
                }

                // ---------------------------------------------------------------
                // Cardinality measure: peak number of simultaneously live values.
                // A value is live from its birth (production) to its death (last
                // consumer).  We sweep +1 at birth, -1 just past death.
                // ---------------------------------------------------------------
                LivenessReport analyzeMemory(const ISchedule& sched) const {
                    std::map<Key, long> birth;
                    for (const auto& [name, eq] : sys_.equations())
                        for (const auto& p : eq.domain.enumerate())
                            birth[{ name, p }] = sched.time(name, p);

                    std::map<Key, long> death = deathTimes(sched);

                    std::map<long, long> delta;
                    std::map<std::string, std::map<long, long>> deltaVar;
                    long tmin = std::numeric_limits<long>::max();
                    long tmax = std::numeric_limits<long>::min();
                    long total = 0;
                    for (const auto& [k, tb] : birth) {
                        auto dit = death.find(k);
                        long td = (dit != death.end()) ? dit->second : tb;  // outputs: live only at birth
                        delta[tb] += 1;            delta[td + 1] -= 1;
                        deltaVar[k.var][tb] += 1;  deltaVar[k.var][td + 1] -= 1;
                        tmin = std::min(tmin, tb);
                        tmax = std::max(tmax, td);
                        ++total;
                    }

                    LivenessReport r;
                    r.totalValues = total;
                    r.latency = total ? (tmax - tmin + 1) : 0;
                    long live = 0, peak = 0;
                    for (const auto& [t, d] : delta) { live += d; peak = std::max(peak, live); }
                    r.peakLiveValues = peak;
                    for (const auto& [var, dv] : deltaVar) {
                        long lv = 0, pk = 0;
                        for (const auto& [t, d] : dv) { lv += d; pk = std::max(pk, lv); }
                        r.peakPerVar[var] = pk;
                    }
                    return r;
                }

                // ---------------------------------------------------------------
                // Schedule-ordered execution with eviction.  Holds only live values,
                // so the realized footprint equals analyzeMemory().peakLiveValues.
                // A value that is never consumed is an output: it is drained through a
                // boundary face at its birth timestep (death := birth), so it never
                // inflates the resident set.  Returns the drained outputs.
                // ---------------------------------------------------------------
                std::map<Key, Value> run(const ISchedule& sched, long* observedPeak = nullptr) {
                    // Reject an illegal schedule up front with the full dependency report,
                    // rather than failing at the first out-of-order read.
                    LegalityReport lr = checkLegality(sys_, sched);
                    if (!lr.legal) {
                        std::ostringstream oss;
                        oss << "SureSimulator::run refusing illegal schedule -- " << lr;
                        throw std::runtime_error(oss.str());
                    }

                    std::map<long, std::vector<Key>> born;
                    for (const auto& [name, eq] : sys_.equations())
                        for (const auto& p : eq.domain.enumerate())
                            born[sched.time(name, p)].push_back({ name, p });

                    std::map<Key, long> death = deathTimes(sched);
                    std::map<long, std::vector<Key>> dieAt;
                    for (const auto& [t, keys] : born)
                        for (const auto& k : keys) {
                            auto it = death.find(k);
                            dieAt[it != death.end() ? it->second : t].push_back(k);  // output drains at birth
                        }

                    std::map<Key, Value> store;     // live (resident) values
                    std::map<Key, Value> outputs;   // drained, never-consumed values
                    long peak = 0;
                    for (const auto& [t, keys] : born) {
                        for (const auto& k : keys) {
                            const auto& eq = sys_.at(k.var);
                            std::vector<Value> tv;
                            tv.reserve(eq.taps.size());
                            for (const auto& tp : eq.taps) {
                                IndexPoint prod = tp.map.apply(k.p);
                                const auto& peq = sys_.at(tp.source);
                                if (!peq.domain.isInside(prod)) {
                                    tv.push_back(peq.boundary(prod));
                                } else {
                                    // Legality was validated up front, so the producer is
                                    // guaranteed resident; this is a defensive invariant.
                                    auto it = store.find({ tp.source, prod });
                                    if (it == store.end())
                                        throw std::runtime_error(
                                            "schedule violation: producer " + tp.source +
                                            " not resident when " + k.var + " fired");
                                    tv.push_back(it->second);
                                }
                            }
                            Value v = eq.compute(tv, k.p);
                            store[k] = v;
                            if (death.find(k) == death.end()) outputs[k] = v;   // never consumed
                        }
                        peak = std::max(peak, static_cast<long>(store.size()));
                        auto it = dieAt.find(t);
                        if (it != dieAt.end())
                            for (const auto& k : it->second) store.erase(k);
                    }
                    if (observedPeak) *observedPeak = peak;
                    return outputs;
                }
            };

        }
    }
}
