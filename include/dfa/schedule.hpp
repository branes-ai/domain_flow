#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>
#include <initializer_list>

namespace sw {
    namespace dfa {
        
        // Representing a piece-wise linear schedule
        template<typename Scalar = int>
        class Schedule {
        private:
            std::vector<Scalar> data;

        public:
			// Constructors
			Schedule() = default;
            Schedule(std::initializer_list<Scalar> init) : data(init) {}
            Schedule(std::vector<Scalar> init) : data(init) {}
            Schedule(size_t size, Scalar value = 0) : data(size, value) {}

            size_t size() const { return data.size(); }

            // operator[] for const access
            const Scalar& operator[](size_t index) const {
                if (index >= data.size()) {
                    throw std::out_of_range("Schedule index out of range.");
                }
                return data[index];
            }

            // operator[] for non-const access
            Scalar& operator[](size_t index) {
                if (index >= data.size()) {
                    throw std::out_of_range("Schedule index out of range.");
                }
                return data[index];
            }

            // modifiers
            void clear() { data.clear(); }
			void resize(size_t newSize, Scalar value = 0) {
				data.resize(newSize, value);
			}
			void push_back(Scalar value) noexcept { data.push_back(value); }
			void pop_back() noexcept { data.pop_back(); }
			void assign(size_t size, Scalar value) noexcept { data.assign(size, value); }
			void assign(std::initializer_list<Scalar> init) noexcept { data.assign(init); }
			void assign(std::vector<Scalar> init) noexcept { data.assign(init.begin(), init.end()); }
			void assign(const Schedule<Scalar>& other) noexcept { data.assign(other.data.begin(), other.data.end()); }

            //selectors
            std::vector<Scalar> toStdVector() const { return data; }
        };

	template<typename Scalar>
        std::ostream& operator<<(std::ostream& os, const Schedule<Scalar>& schedule) {
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
