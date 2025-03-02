#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>
#include <initializer_list>

namespace sw {
    namespace dfa {

        template<typename Scalar>
        class Matrix {
        private:
            std::vector<std::vector<Scalar>> data;

        public:
            Matrix(std::initializer_list<std::initializer_list<Scalar>> init) {
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
            Matrix(std::vector<std::vector<Scalar>> init) : data(init) {}
            Matrix(size_t rows, size_t cols, Scalar value = 0) {
                data.resize(rows);
                for (size_t i = 0; i < rows; ++i) {
                    data[i].resize(cols, value);
                }
            }
            size_t rows() const { return data.size(); }
            size_t cols() const { return data[0].size(); }

            // operator[][] for const access
            const std::vector<Scalar>& operator[](size_t row) const {
                if (row >= data.size()) {
                    throw std::out_of_range("Matrix row index out of range.");
                }
                return data[row];
            }

            // operator[][] for non-const access
            std::vector<Scalar>& operator[](size_t row) {
                if (row >= data.size()) {
                    throw std::out_of_range("Matrix row index out of range.");
                }
                return data[row];
            }

            Matrix operator*(const Matrix& other) const {
                if (cols() != other.rows()) {
                    throw std::invalid_argument("Matrices cannot be multiplied.");
                }

                Matrix result(rows(), other.cols(), 0); // Initialize an zero result matrix
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
        std::ostream& operator<<(std::ostream& ostr, const Matrix<_T>& M) {
            ostr << '{';
            for (int i = 0; i < M.rows(); ++i) {
                ostr << '{';
                for (int j = 0; j < M.cols(); ++j) {
                    ostr << M[i][j];
                    if (j < M.cols() - 1) ostr << ", ";
                }
                if (i < M.cols() - 1) ostr << "},\n"; else ostr << "}\n";
            }
            return ostr << "}\n";
        }
    }
}