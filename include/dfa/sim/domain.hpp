#pragma once
#include <iostream>
#include <functional>
#include <vector>
#include <dfa/index_point.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // A single affine half-space (or equality):  a . x  {<=,>=,==}  rhs
            // This is the lightweight twin of sw::dfa::Hyperplane; a Domain built from
            // these is the twin of sw::dfa::ConstraintSet / DomainOfComputation.
            struct HalfSpace {
                enum Rel { LE, GE, EQ };
                std::vector<int> a;
                int rhs;
                Rel rel;

                bool satisfied(const IndexPoint& p) const {
                    long s = 0;
                    for (std::size_t c = 0; c < a.size(); ++c) s += static_cast<long>(a[c]) * p[c];
                    switch (rel) {
                        case LE: return s <= rhs;
                        case GE: return s >= rhs;
                        case EQ: return s == rhs;
                    }
                    return false;
                }
            };

            // The set of integer points over which a recurrence variable is *computed*.
            //
            // In the full library this role is played by DomainOfComputation: its
            // elaborateConstraintSet() emits the constraints and IndexSpace::instantiate()
            // enumerates the points.  This class is a drop-in standalone equivalent:
            //   isInside(p)  ==  DomainOfComputation::isInside(p)
            //   enumerate()  ==  IndexSpace::getPoints()
            // A point that fails isInside() is a *boundary* (an input operand or an
            // initial condition) rather than a computed value.
            class Domain {
                int dim_ = 0;
                std::vector<HalfSpace> cons_;
                std::vector<int> lo_, hi_;   // bounding box used for enumeration

            public:
                Domain() = default;
                explicit Domain(int dim) : dim_(dim), lo_(dim, 0), hi_(dim, 0) {}

                int dimension() const noexcept { return dim_; }

                // 0 <= x_axis < extent : adds the two half-spaces *and* widens the
                // enumeration box along that axis.
                Domain& axis(int axis, int loV, int hiV) {
                    HalfSpace lo{ std::vector<int>(dim_, 0), loV,     HalfSpace::GE };  lo.a[axis] = 1;
                    HalfSpace hi{ std::vector<int>(dim_, 0), hiV - 1, HalfSpace::LE };  hi.a[axis] = 1;
                    cons_.push_back(lo);
                    cons_.push_back(hi);
                    lo_[axis] = loV;
                    hi_[axis] = hiV - 1;
                    return *this;
                }

                // Arbitrary affine constraint (e.g. k <= j for a triangular domain).
                Domain& add(HalfSpace h) { cons_.push_back(std::move(h)); return *this; }

                bool isInside(const IndexPoint& p) const {
                    if (static_cast<int>(p.size()) != dim_) return false;   // wrong rank => boundary
                    for (const auto& c : cons_) if (!c.satisfied(p)) return false;
                    return true;
                }

                // Enumerate the box, keep the points that satisfy every constraint.
                // (Same strategy as IndexSpace::enumerate: bounding box then filter.)
                std::vector<IndexPoint> enumerate() const {
                    std::vector<IndexPoint> pts;
                    if (dim_ == 0) return pts;
                    std::vector<int> cur = lo_;
                    while (true) {
                        IndexPoint p(cur);
                        if (isInside(p)) pts.push_back(p);
                        int d = dim_ - 1;
                        while (d >= 0) {
                            if (++cur[d] <= hi_[d]) break;
                            cur[d] = lo_[d];
                            --d;
                        }
                        if (d < 0) break;
                    }
                    return pts;
                }
            };

        }
    }
}
