#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>
#include <initializer_list>

namespace sw {
    namespace dfa {
        
        template<typename Scalar>
        class Vector {
        private:
            std::vector<Scalar> data;

        public:
            Vector(std::initializer_list<Scalar> init) : data(init) {}
            Vector(std::vector<Scalar> init) : data(init) {}
            Vector(size_t size, Scalar value = 0) : data(size, value) {}

            size_t size() const { return data.size(); }

            // operator[] for const access
            const Scalar& operator[](size_t index) const {
                if (index >= data.size()) {
                    throw std::out_of_range("Vector index out of range.");
                }
                return data[index];
            }

            // operator[] for non-const access
            Scalar& operator[](size_t index) {
                if (index >= data.size()) {
                    throw std::out_of_range("Vector index out of range.");
                }
                return data[index];
            }

            std::vector<Scalar> toStdVector() const { return data; }
        };

		template<typename Scalar>
        std::ostream& operator<<(std::ostream& os, const Vector<Scalar>& vec) {
            os << "[ ";
            for (size_t i = 0; i < vec.size(); ++i) {
                os << vec[i];
                if (i < vec.size() - 1) {
                    os << ", ";
                }
            }
            return os << " ]";
        }
    }
}