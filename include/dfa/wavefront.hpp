#pragma once
#include <vector>
#include <stdexcept>
#include <dfa/index_point.hpp>

namespace sw {
    namespace dfa {

        class Wavefront {
        public:
            Wavefront() = default;
    
            ////////////////////////////////////////////////////////////////////////
            /// operators
            const IndexPoint& operator[](std::size_t index) const {
                return activity[index];
            }
            IndexPoint& operator[](std::size_t index) {
                return activity[index];
            }
 
            ////////////////////////////////////////////////////////////////////////
            /// modifiers
            void add(const IndexPoint& p) {
				activity.push_back(p);
            }

            ////////////////////////////////////////////////////////////////////////
            /// selectors
    
        private:
			// a vector of index points that represent the activities 
			// that can be executed concurrently
            std::vector<IndexPoint> activity;

            friend inline std::ostream& operator<<(std::ostream& ostr, const Wavefront& w) {
                ostr << "Wavefront: ";
                for (const auto& p : w.activity) {
                    ostr << p << ' ';
                }
                return ostr;
            }
        };

    }
}
