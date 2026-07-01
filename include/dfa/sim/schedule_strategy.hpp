#pragma once
#include <iostream>
#include <functional>
#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <dfa/index_point.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // A schedule answers one question: at what timestep does the value
            // <var, p> fire?  The simulator evaluates wavefronts in nondecreasing
            // time, and the liveness analysis measures memory against this ordering.
            struct ISchedule {
                virtual ~ISchedule() = default;
                virtual long time(const std::string& var, const IndexPoint& p) const = 0;
            };

            // Linear (affine-by-statement) schedule:  t_var(p) = tau . p + beta[var].
            // tau is the classic SURE scheduling vector (ScheduleVector::dot); beta is an
            // optional per-variable constant offset (pipeline stage delay).
            class LinearSchedule : public ISchedule {
                std::vector<int> tau_;
                std::map<std::string, long> beta_;
            public:
                explicit LinearSchedule(std::vector<int> tau, std::map<std::string, long> beta = {})
                    : tau_(std::move(tau)), beta_(std::move(beta)) {}

                long time(const std::string& var, const IndexPoint& p) const override {
                    long t = 0;
                    for (std::size_t i = 0; i < tau_.size() && i < p.size(); ++i)
                        t += static_cast<long>(tau_[i]) * p[i];
                    auto it = beta_.find(var);
                    if (it != beta_.end()) t += it->second;
                    return t;
                }
            };

            // Piecewise-linear schedule: the first piece whose region contains the
            // point supplies the linear timing.  Models index-set splitting where
            // different cones of the domain run under different scheduling vectors.
            class PiecewiseLinearSchedule : public ISchedule {
            public:
                struct Piece {
                    std::function<bool(const std::string&, const IndexPoint&)> region;
                    std::vector<int> tau;
                    long offset;
                };
                std::vector<Piece> pieces;

                long time(const std::string& var, const IndexPoint& p) const override {
                    for (const auto& pc : pieces) {
                        if (pc.region(var, p)) {
                            long t = pc.offset;
                            for (std::size_t i = 0; i < pc.tau.size() && i < p.size(); ++i)
                                t += static_cast<long>(pc.tau[i]) * p[i];
                            return t;
                        }
                    }
                    throw std::runtime_error("PiecewiseLinearSchedule: no piece covers the point");
                }
            };

            // Explicit table.  Used to carry the *free* (ASAP) schedule that the
            // simulator derives by data-flow depth: t = 0 at the boundaries, and
            // t = 1 + max(producer times) for every computed value.  It is the
            // minimum-latency / maximum-parallelism schedule.
            class ExplicitSchedule : public ISchedule {
                std::map<std::pair<std::string, IndexPoint>, long> table_;
            public:
                void set(const std::string& var, const IndexPoint& p, long t) { table_[{var, p}] = t; }

                long time(const std::string& var, const IndexPoint& p) const override {
                    auto it = table_.find({ var, p });
                    return (it != table_.end()) ? it->second : 0;   // boundaries default to t=0
                }
            };

        }
    }
}
