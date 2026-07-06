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
            class RecurrenceSystem {
                std::map<std::string, Equation<Value>> eqs_;
            public:
                RecurrenceSystem& add(Equation<Value> e) {
                    if (eqs_.count(e.name))
                        throw std::invalid_argument("RecurrenceSystem::add: duplicate equation name '" + e.name + "'");
                    eqs_[e.name] = std::move(e);
                    return *this;
                }
                const Equation<Value>& at(const std::string& n) const { return eqs_.at(n); }
                bool has(const std::string& n) const { return eqs_.count(n) > 0; }
                const std::map<std::string, Equation<Value>>& equations() const { return eqs_; }
            };

        }
    }
}
