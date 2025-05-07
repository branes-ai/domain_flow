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
        inline std::ostream& operator<<(std::ostream& os, const Vector<Scalar>& vec) {
            os << "[ ";
            for (size_t i = 0; i < vec.size(); ++i) {
                os << vec[i];
                if (i < vec.size() - 1) {
                    os << ", ";
                }
            }
            return os << " ]";
        }


        struct Vector3d {
            double x, y, z;
            Vector3d(double x_ = 0, double y_ = 0, double z_ = 0) : x(x_), y(y_), z(z_) {}

			double operator[](size_t index) const {
				if (index == 0) return x;
				else if (index == 1) return y;
				else if (index == 2) return z;
				throw std::out_of_range("Index out of range for Vector3d");
			}
            double& operator[](size_t index) {
                if (index == 0) return x;
                else if (index == 1) return y;
                else if (index == 2) return z;
                throw std::out_of_range("Index out of range for Vector3d");
            }

            // Vector addition
            Vector3d operator+(const Vector3d& other) const {
                return Vector3d(x + other.x, y + other.y, z + other.z);
            }

            // Vector subtraction
            Vector3d operator-(const Vector3d& other) const {
                return Vector3d(x - other.x, y - other.y, z - other.z);
            }

            // Scalar multiplication
            Vector3d operator*(double scalar) const {
                return Vector3d(x * scalar, y * scalar, z * scalar);
            }

            // Dot product
            double dot(const Vector3d& other) const {
                return x * other.x + y * other.y + z * other.z;
            }

            // Cross product
            Vector3d cross(const Vector3d& other) const {
                return Vector3d(
                    y * other.z - z * other.y,
                    z * other.x - x * other.z,
                    x * other.y - y * other.x
                );
            }

            // Norm (magnitude)
            double norm() const {
                return std::sqrt(x * x + y * y + z * z);
            }

            // Normalize
            Vector3d normalized() const {
                double n = norm();
                if (n < 1e-6) throw std::runtime_error("Cannot normalize zero vector");
                return Vector3d(x / n, y / n, z / n);
            }
		};

        inline Vector3d cross(const Vector3d& a, const Vector3d& b) {
            return {
                a[1] * b[2] - a[2] * b[1],
                a[2] * b[0] - a[0] * b[2],
                a[0] * b[1] - a[1] * b[0]
            };
        }
        inline double dot(const Vector3d& a, const Vector3d& b) {
            return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        }
        inline Vector3d normalize(const Vector3d& v) {
            double mag = std::sqrt(dot(v, v));
            if (mag == 0) {
                throw std::runtime_error("Cannot normalize zero vector");
            }
            return { v[0] / mag, v[1] / mag, v[2] / mag };
        }
        inline std::ostream& operator<<(std::ostream& os, const Vector3d& vec) {
            os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
            return os;
        }
    }
}