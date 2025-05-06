#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>
#include <initializer_list>
#include <dfa/wavefront.hpp>

namespace sw {
    namespace dfa {
        
        template<typename ConstraintCoefficientType>
        class ScheduleVector {
			using Scalar = ConstraintCoefficientType;
        private:
            std::vector<ConstraintCoefficientType> tau;

        public:
            ScheduleVector() = default;
            ScheduleVector(std::initializer_list<Scalar> init) : tau(init) {}
            ScheduleVector(std::vector<Scalar> init) : tau(init) {}
            ScheduleVector(size_t size, Scalar value = 0) : tau(size, value) {}

            ////////////////////////////////////////////////////////////////////////
            /// modifiers
            void clear() { tau.clear(); }
            void resize(size_t newSize, Scalar value = 0) { tau.resize(newSize, value); }
            void push_back(Scalar value) noexcept { tau.push_back(value); }
            void pop_back() noexcept { tau.pop_back(); }

            void assign(size_t size, Scalar value) noexcept { tau.assign(size, value); }
            void assign(std::initializer_list<Scalar> init) noexcept { tau.assign(init); }
            void assign(std::vector<Scalar> init) noexcept { tau.assign(init.begin(), init.end()); }
            void assign(const ScheduleVector<Scalar>& other) noexcept { tau.assign(other.tau.begin(), other.tau.end()); }

			////////////////////////////////////////////////////////////////////////
            /// selectors

			long dot(const IndexPoint& p) const noexcept {
				if (p.size() != tau.size()) {
					std::cerr << "index point size does not match schedule vector size\n";
                    return -LONG_MAX;
				}
				long result = 0;
				for (size_t i = 0; i < tau.size(); ++i) {
					result += tau[i] * p[i];
				}
				return result;
			}
        };


        // Representing a piece-wise linear schedule
        template<typename ConstraintCoefficientType = int>
        class Schedule {
            using Scalar = ConstraintCoefficientType;
        private:
            // for each time step, we have a wavefront consisting of independent
            // activities that can execute concurrently
            std::map<std::size_t, Wavefront> wavefronts;

        public:
            // Constructors
            Schedule() = default;

            // operator[] for const access
            const Wavefront& operator[](std::size_t timestep) const {
                auto it = wavefronts.find(timestep);
                if (it != wavefronts.end()) {
                    return it->second;
                }
                throw std::out_of_range("Index out of range");
            }
            // operator[] for non-const access
            Wavefront& operator[](std::size_t timestep) {
	            auto it = wavefronts.find(timestep);
	            if (it != wavefronts.end()) {
		            return it->second;
	            }
	            throw std::out_of_range("Index out of range");
            }

            ////////////////////////////////////////////////////////////////////////
            /// modifiers
            void clear() { wavefronts.clear(); }

			void addActivity(std::size_t timestep, const IndexPoint& p) {
				auto it = wavefronts.find(timestep);
				if (it != wavefronts.end()) {
					it->second.addActivity(p);
				}
				else {
					Wavefront wf;
					wf.addActivity(p);
					wavefronts[timestep] = wf;
				}
			}
            void addWavefront(std::size_t timestep, const Wavefront& wf) {
                wavefronts[timestep] = wf;
            }
            // Remove a wavefront at a specific time
            bool removeWavefront(size_t timestep) {
                return wavefronts.erase(timestep) > 0;
            }

            ////////////////////////////////////////////////////////////////////////
            /// selectors
            // Get wavefront at a specific time
            const Wavefront* getWavefront(size_t time) const noexcept {
                auto it = wavefronts.find(time);
                if (it != wavefronts.end()) {
                    return &(it->second);
                }
                return nullptr;
            }

            void enumerateWavefronts() const {
                for (const auto& [time, wavefront] : wavefronts) {
                    std::cout << "Time: " << time << " - ";
                    std::cout <<  wavefront << '\n';
                }
            }

            ////////////////////////////////////////////////////////////////////////
            // iterators Provide access to iterators for external enumeration
            auto begin() const { return wavefronts.begin(); }
            auto end() const { return wavefronts.end(); }
        };

        template<typename Scalar>
        inline std::ostream& operator<<(std::ostream& os, const Schedule<Scalar>& schedule) {
            os << "[ ";
            for (size_t i = 0; i < schedule.size(); ++i) {
                os << schedule[i];
                if (i < schedule.size() - 1) {
                    os << ", ";
                }
            }
            return os << " ]";
        }

    }
}
