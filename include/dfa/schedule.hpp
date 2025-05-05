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
            const Wavefront& operator[](std::size_t index) const {
                auto it = wavefronts.find(index);
                if (it != wavefronts.end()) {
                    return it->second;
                }
                throw std::out_of_range("Index out of range");
            }
            // operator[] for non-const access
            Wavefront& operator[](std::size_t index) {
	            auto it = wavefronts.find(index);
	            if (it != wavefronts.end()) {
		            return it->second;
	            }
	            throw std::out_of_range("Index out of range");
            }

            ////////////////////////////////////////////////////////////////////////
            /// modifiers
            void clear() { wavefronts.clear(); }

            void addWavefront(std::size_t index, const Wavefront& wf) {
                wavefronts[index] = wf;
            }

            ////////////////////////////////////////////////////////////////////////
            /// selectors
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
