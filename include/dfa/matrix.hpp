#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>
#include <initializer_list>
#include <dfa/vector.hpp>

namespace sw {
    namespace dfa {

        // forward definition
        template<typename Scalar> class VectorX;
		template<typename Scalar> class Matrix3;

        ////////////////////////////////////////////////////////////////////////////////
		// generalized matrix class for 2D arrays (matrix of vectors)
        template<typename Scalar>
        class MatrixX {
        private:
            std::vector<VectorX<Scalar>> data;

        public:
            MatrixX(std::initializer_list<std::initializer_list<Scalar>> init) {
                if (init.size() == 0) {
                    throw std::invalid_argument("Matrix cannot be empty.");
                }

                size_t rows = init.size();
                size_t cols = 0;
                for (const auto& row : init) {
                    if (cols == 0) {
                        cols = row.size();
                        if (cols == 0)
                            throw std::invalid_argument("Matrix rows cannot be empty");
                    } else if (row.size() != cols) {
                        throw std::invalid_argument("All rows must have the same number of columns.");
                    }
                }

                data.resize(rows);
                for (size_t i = 0; i < rows; ++i) {
                    data[i].resize(cols);
                    size_t j = 0;
                    for (Scalar val : init.begin()[i]) {
                        data[i][j++] = val;
                    }
                }
            }
            MatrixX(std::vector<std::vector<Scalar>> init) : data(init) {}
            MatrixX(size_t rows, size_t cols, Scalar value = 0) {
                data.resize(rows);
                for (size_t i = 0; i < rows; ++i) {
                    data[i].resize(cols, value);
                }
            }

            /////////////////////////////////////////////////////////////////////////////
            // selectors

            size_t rows() const { return data.size(); }
            size_t cols() const { return data[0].size(); }

			VectorX<Scalar> row(size_t row) const {
				if (row >= data.size()) {
					throw std::out_of_range("Matrix row index out of range.");
				}
				return VectorX<Scalar>(data[row]);
			}
			VectorX<Scalar> col(size_t col) const {
				if (col >= data[0].size()) {
					throw std::out_of_range("Matrix column index out of range.");
				}
				VectorX<Scalar> result(rows());
				for (size_t i = 0; i < rows(); ++i) {
					result[i] = data[i][col];
				}
				return result;
			}

            /////////////////////////////////////////////////////////////////////////////
            // operators
            
            // operator[][] for const access
            const VectorX<Scalar>& operator[](size_t row) const {
                if (row >= data.size()) {
                    throw std::out_of_range("Matrix row index out of range.");
                }
                return data[row];
            }

            // operator[][] for non-const access
            VectorX<Scalar>& operator[](size_t row) {
                if (row >= data.size()) {
                    throw std::out_of_range("Matrix row index out of range.");
                }
                return data[row];
            }

            Scalar operator()(int i, int j) const { return data[i][j]; }
            Scalar& operator()(int i, int j) { return data[i][j]; }

            // Matrix-matrix multiplication
            MatrixX operator*(const MatrixX& other) const {
                if (cols() != other.rows()) {
                    throw std::invalid_argument("Matrices cannot be multiplied.");
                }

                MatrixX result(rows(), other.cols(), 0); // Initialize an zero result matrix
                //result.data.resize(rows());
                for (size_t i = 0; i < rows(); ++i) {
                   // result.data[i].resize(other.cols());
                    for (size_t j = 0; j < other.cols(); ++j) {
                        Scalar sum = 0;
                        for (size_t k = 0; k < cols(); ++k) {
                            sum += data[i][k] * other[k][j];
                        }
                        result.data[i][j] = sum;
                    }
                }

                return result;
            }

			// Specialization for 3x3 matrix multiplication
            MatrixX operator*(const Matrix3<Scalar>& m) const {
                if (cols() != 3) throw std::runtime_error("MatrixX must have 3 columns");
                MatrixX result(rows(), cols());
                for (int i = 0; i < rows(); ++i)
                    for (int j = 0; j < cols(); ++j)
                        result(i, j) = row(i).dot(m.row(j));
                return result;
            }

			// Matrix-vector multiplication
            std::vector<Scalar> operator*(const std::vector<Scalar>& vec) const {
                if (cols() != vec.size()) {
                    throw std::invalid_argument("Matrix and vector dimensions do not match for multiplication.");
                }
                std::vector<Scalar> result(rows());
                for (size_t i = 0; i < rows(); ++i) {
                    double sum = 0;
                    for (size_t j = 0; j < cols(); ++j) {
                        sum += data[i][j] * vec[j];
                    }
                    result[i] = sum;
                }
                return result;
            }
        };
 
        template<typename _T>
        inline std::ostream& operator<<(std::ostream& ostr, const MatrixX<_T>& M) {
            ostr << "{\n";
            for (int i = 0; i < M.rows(); ++i) {
                ostr << " { ";
                for (int j = 0; j < M.cols(); ++j) {
                    ostr << M[i][j];
                    if (j < M.cols() - 1) ostr << ", ";
                }
                if (i < M.cols() - 1) ostr << "},\n"; else ostr << "}\n";
            }
            return ostr << "}\n";
        }


        ////////////////////////////////////////////////////////////////////////////////
		// Specialized 3x3 Matrix class for 3D transformations
        
        // forward reference
		template<typename Scalar> class Vector3;

        // Specialized 3x3 Matrix class
        template<typename Scalar = float>
        class Matrix3 {
        public:
            Matrix3() {
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        data[i][j] = (i == j) ? 1.0 : 0.0;
            }

            Matrix3(const std::vector<std::vector<Scalar>>& dataIn) {
                if (data.size() != 3 || data[0].size() != 3)
                    throw std::invalid_argument("Invalid matrix dimensions");
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        data[i][j] = dataIn[i][j];
            }

            Scalar operator()(int i, int j) const { return data[i][j]; }
            Scalar& operator()(int i, int j) { return data[i][j]; }

			// operator[][] for const access
			const Scalar* operator[](size_t row) const {
				if (row >= 3) {
					throw std::out_of_range("Matrix row index out of range.");
				}
				return data[row];
			}

			// operator[][] for non-const access
			Scalar* operator[](size_t row) {
				if (row >= 3) {
					throw std::out_of_range("Matrix row index out of range.");
				}
				return data[row];
			}

            Matrix3 operator-(const Matrix3& other) const {
                Matrix3 result;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        result(i, j) = data[i][j] - other(i, j);
                return result;
            }

            Vector3<Scalar> operator*(const Vector3<Scalar>& v) const {
                Vector3<Scalar> result;
                for (int i = 0; i < 3; ++i) {
                    result[i] = 0;
                    for (int j = 0; j < 3; ++j)
                        result[i] += data[i][j] * v[j];
                }
                return result;
            }

            Matrix3 operator*(const Matrix3& other) const {
                Matrix3 result;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j) {
                        result(i, j) = 0;
                        for (int k = 0; k < 3; ++k)
                            result(i, j) += data[i][k] * other(k, j);
                    }
                return result;
            }

            static Matrix3 identity() {
				Matrix3 result;
				for (int i = 0; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						result(i, j) = (i == j) ? 1.0 : 0.0;
				return result;
            }

           Matrix3 transpose() const {
                Matrix3 result;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        result(i, j) = data[j][i];
                return result;
            }

            Vector3<Scalar> row(int i) const {
                return { data[i][0], data[i][1], data[i][2] };
            }

        private:
            Scalar data[3][3];
        };

        template<typename _T>
        inline std::ostream& operator<<(std::ostream& ostr, const Matrix3<_T>& M) {
            ostr << "{\n";
            for (int i = 0; i < 3; ++i) {
                ostr << " { ";
                for (int j = 0; j < 3; ++j) {
                    ostr << M[i][j];
                    if (j < 2) ostr << ", ";
                }
                if (i <2) ostr << "},\n"; else ostr << "}\n";
            }
            return ostr << "}\n";
        }


        ////////////////////////////////////////////////////////////////////////////////
        // Specialized Matrix4 class for 4x4 Homogeneous Transformations
		template<typename Scalar = float>
        class Matrix4 {
        public:
            Matrix4() {
                for (int i = 0; i < 4; ++i)
                    for (int j = 0; j < 4; ++j)
                        data[i][j] = (i == j) ? 1.0 : 0.0;
            }

            Matrix4(const std::vector<std::vector<Scalar>>& dataIn) {
                if (dataIn.size() != 4 || dataIn[0].size() != 4)
                    throw std::invalid_argument("Invalid matrix dimensions");
                for (int i = 0; i < 4; ++i)
                    for (int j = 0; j < 4; ++j)
                        data[i][j] = dataIn[i][j];
            }

            Scalar operator()(int i, int j) const { return data[i][j]; }
            Scalar& operator()(int i, int j) { return data[i][j]; }

            Vector3<Scalar> transformPoint(const Vector3<Scalar>& v) const {
                Scalar x = data[0][0] * v[0] + data[0][1] * v[1] + data[0][2] * v[2] + data[0][3];
                Scalar y = data[1][0] * v[0] + data[1][1] * v[1] + data[1][2] * v[2] + data[1][3];
                Scalar z = data[2][0] * v[0] + data[2][1] * v[1] + data[2][2] * v[2] + data[2][3];
                Scalar w = data[3][0] * v[0] + data[3][1] * v[1] + data[3][2] * v[2] + data[3][3];
                if (std::abs(w) < 1e-10) throw std::runtime_error("Invalid homogeneous coordinate");
                return Vector3<Scalar>(x / w, y / w, z / w);
            }

        private:
            Scalar data[4][4];
        };
    }
}