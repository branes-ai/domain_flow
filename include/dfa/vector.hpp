#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>
#include <initializer_list>
#include <cmath>

namespace sw {
    namespace dfa {
        
		// forward definition
        template<typename Scalar> class Vector3;

        template<typename Scalar>
        class VectorX {
        private:
            std::vector<Scalar> data;

        public:
			VectorX() = default;
            VectorX(std::initializer_list<Scalar> init) : data(init) {}
            VectorX(const std::vector<Scalar>& init) : data(init) {}
            VectorX(size_t size, Scalar value = 0) : data(size, value) {}

            size_t size() const { return data.size(); }
            void resize(size_t newSize) { data.resize(newSize); }
			void resize(size_t newSize, Scalar value) { data.resize(newSize, value); }

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

            Scalar operator()(int i) const { return data[i]; }
            Scalar& operator()(int i) { return data[i]; }

            // dot product
			Scalar dot(const VectorX& other) const {
				if (size() != other.size()) {
					throw std::invalid_argument("Vectors must be of the same size for dot product.");
				}
				Scalar result = 0;
				for (size_t i = 0; i < size(); ++i) {
					result += data[i] * other[i];
				}
				return result;
			}

            // specialized dot product
            Scalar dot(const Vector3<Scalar>& other) const {
                if (size() != 3) {
                    throw std::invalid_argument("Vector must be 3D.");
                }
                Scalar result = 0;
                for (size_t i = 0; i < size(); ++i) {
                    result += data[i] * other[i];
                }
                return result;
            }

            std::vector<Scalar> toStdVector() const { return data; }
        };

		template<typename Scalar>
        inline std::ostream& operator<<(std::ostream& os, const VectorX<Scalar>& vec) {
            os << "[ ";
            for (size_t i = 0; i < vec.size(); ++i) {
                os << vec[i];
                if (i < vec.size() - 1) {
                    os << ", ";
                }
            }
            return os << " ]";
        }


        // Specialized 3D Vector class
        template<typename Scalar = float>
        class Vector3 {
        public:
            Vector3(Scalar x = 0, Scalar y = 0, Scalar z = 0) : data{ x, y, z } {}

            Scalar operator[](int i) const { return data[i]; }
            Scalar& operator[](int i) { return data[i]; }

            double operator()(int i) const { return data[i]; }
            double& operator()(int i) { return data[i]; }

            Vector3 operator+(const Vector3& other) const {
                return { data[0] + other[0], data[1] + other[1], data[2] + other[2] };
            }

            Vector3 operator-(const Vector3& other) const {
                return { data[0] - other[0], data[1] - other[1], data[2] - other[2] };
            }

            Vector3 operator*(Scalar scalar) const {
                return { data[0] * scalar, data[1] * scalar, data[2] * scalar };
            }

            double dot(const Vector3& other) const {
                return data[0] * other[0] + data[1] * other[1] + data[2] * other[2];
            }

            Vector3 cross(const Vector3& other) const {
                return {
                    data[1] * other[2] - data[2] * other[1],
                    data[2] * other[0] - data[0] * other[2],
                    data[0] * other[1] - data[1] * other[0]
                };
            }

            double norm() const {
                return std::sqrt(dot(*this));
            }

            Vector3 normalized() const {
                double n = norm();
                if (n < 1e-10) throw std::runtime_error("Cannot normalize zero vector");
                return *this * (1.0 / n);
            }

        private:
            Scalar data[3];
        };

        template<typename Scalar>
        inline std::ostream& operator<<(std::ostream& os, const Vector3<Scalar>& vec) {
            return os << "[ " << vec[0] << ", " << vec[1] << ", " << vec[2] << " ]";
        }

    }
}