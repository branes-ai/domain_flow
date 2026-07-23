#pragma once
#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <dfa/index_point.hpp>
#include <dfa/sim/affine_dependency.hpp>
#include <dfa/sim/domain.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // One equation of a System of Uniform/Affine Recurrence Equations.
            //
            //   - name:     the recurrence variable
            //   - domain:   the index set over which it is *computed* (a DomainOfComputation)
            //   - taps:     the values it reads, each via an affine dependency map
            //   - compute:  the recurrence body, given the resolved tap values (in order)
            //   - boundary: the value when a point is outside the domain -- this is where
            //               operand data and initial conditions enter the system.
            //
            // An *input operand* is just an equation with an empty domain (rank-0): every
            // access falls through to boundary(), so it streams data without being stored.
            template<typename Value = double>
            struct Equation {
                using Compute  = std::function<Value(const std::vector<Value>& taps, const IndexPoint& p)>;
                using Boundary = std::function<Value(const IndexPoint& p)>;

                struct Tap {
                    std::string source;
                    AffineDependency map;
                };

                std::string name;
                Domain domain;
                std::vector<Tap> taps;
                Compute compute;
                Boundary boundary;
            };

            template<typename Value = double>
            // A variable may have MORE THAN ONE equation, each on a disjoint sub-domain -- a
            // piecewise / conditional recurrence, e.g. a pipelined state routed differently for
            // i>j, i<j, i=j.  eqs_[name] is the list of branches; resolve(name,p) picks the branch
            // whose domain contains p, and coversAny(name,p) is the union membership test.
            class RecurrenceSystem {
                std::map<std::string, std::vector<Equation<Value>>> eqs_;
            public:
                RecurrenceSystem& add(Equation<Value> e) {
                    eqs_[e.name].push_back(std::move(e));
                    return *this;
                }
                bool has(const std::string& n) const { return eqs_.count(n) > 0; }
                // a representative branch (the first) -- for single-equation variables this is THE equation
                const Equation<Value>& at(const std::string& n) const { return eqs_.at(n).front(); }
                // the branch computing point p (its domain contains p), or nullptr if p is a boundary
                const Equation<Value>* resolve(const std::string& n, const IndexPoint& p) const {
                    auto it = eqs_.find(n);
                    if (it == eqs_.end()) return nullptr;
                    for (const auto& e : it->second) if (e.domain.isInside(p)) return &e;
                    return nullptr;
                }
                // p lies in the domain of SOME branch of n (i.e. n is computed there, not a boundary)
                bool coversAny(const std::string& n, const IndexPoint& p) const { return resolve(n, p) != nullptr; }
                const std::map<std::string, std::vector<Equation<Value>>>& equations() const { return eqs_; }
            };

        }
    }
}
