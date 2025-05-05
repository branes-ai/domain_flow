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
			void clear() { activity.clear(); }
            void addActivity(const IndexPoint& p) {
				activity.push_back(p);
            }
            auto front() { return activity.front(); }
            auto back() { return activity.back(); }
            auto at(std::size_t index) { return activity.at(index); }

            ////////////////////////////////////////////////////////////////////////
            /// selectors
            auto size() const noexcept { return activity.size(); }
            auto empty() const noexcept { return activity.empty(); }
            auto front() const { return activity.front(); }
            auto back() const { return activity.back(); }
            auto at(std::size_t index) const { return activity.at(index); }

            ////////////////////////////////////////////////////////////////////////
            /// iterators
			auto begin() { return activity.begin(); }
			auto end() { return activity.end(); }
			auto begin() const { return activity.begin(); }
			auto end() const { return activity.end(); }

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
