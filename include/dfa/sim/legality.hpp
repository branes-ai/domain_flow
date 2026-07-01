#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <cstddef>
#include <dfa/index_point.hpp>
#include <dfa/sim/recurrence_system.hpp>
#include <dfa/sim/schedule_strategy.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // A dependency edge that a schedule failed to order correctly.
            //   slack = time(consumer, at) - time(producer, from)
            // A legal schedule needs slack >= 1 on every edge (producer strictly
            // precedes consumer).  For a linear schedule t = tau.p + beta this slack
            // equals  tau.theta + (beta[consumer] - beta[producer])  where theta is the
            // dependency vector (at - from); with equal/zero beta it is exactly tau.theta.
            struct DependencyViolation {
                std::string consumer;
                IndexPoint  at;
                std::string producer;
                IndexPoint  from;
                long        slack;
            };

            struct LegalityReport {
                bool legal = true;
                long edgesChecked = 0;
                long minSlack = std::numeric_limits<long>::max();   // tightest edge; >=1 => legal
                std::vector<DependencyViolation> violations;        // capped sample
            };

            // Validate that a schedule respects every dependency of the system:
            // tau.theta >= 1 (generalized to time(consumer) - time(producer) >= 1).
            //
            // Only edges to *computed* producers are constrained; a tap that lands on a
            // boundary (an input operand / initial condition) is available from the start
            // and imposes no ordering.
            template<typename Value>
            LegalityReport checkLegality(const RecurrenceSystem<Value>& sys,
                                         const ISchedule& sched,
                                         std::size_t maxViolations = 16) {
                LegalityReport rep;
                for (const auto& [name, eq] : sys.equations()) {
                    for (const auto& q : eq.domain.enumerate()) {
                        long tc = sched.time(name, q);
                        for (const auto& tap : eq.taps) {
                            IndexPoint prod = tap.map.apply(q);
                            if (!sys.has(tap.source)) continue;
                            if (!sys.at(tap.source).domain.isInside(prod)) continue;  // boundary
                            long tp = sched.time(tap.source, prod);
                            long slack = tc - tp;
                            ++rep.edgesChecked;
                            rep.minSlack = std::min(rep.minSlack, slack);
                            if (slack < 1) {
                                rep.legal = false;
                                if (rep.violations.size() < maxViolations)
                                    rep.violations.push_back({ name, q, tap.source, prod, slack });
                            }
                        }
                    }
                }
                if (rep.edgesChecked == 0) rep.minSlack = 0;   // nothing to constrain
                return rep;
            }

            inline std::ostream& operator<<(std::ostream& os, const LegalityReport& r) {
                os << (r.legal ? "LEGAL" : "ILLEGAL")
                   << "  edges=" << r.edgesChecked
                   << "  minSlack(tau.theta)=" << r.minSlack;
                if (!r.legal) {
                    os << "  violations=" << r.violations.size() << ":\n";
                    for (const auto& v : r.violations)
                        os << "    " << v.consumer << v.at << "reads " << v.producer << v.from
                           << " slack=" << v.slack << " (need >= 1)\n";
                }
                return os;
            }

        }
    }
}
