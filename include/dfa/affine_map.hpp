#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <dfa/vector.hpp>
#include <dfa/matrix.hpp>

namespace sw {
    namespace dfa {


        // Represents an affine transformation between recurrence variables
        //class AffineMap {
        //public:
        //    AffineMap(const std::vector<int>& coeffs, const std::vector<int>& translation)
        //        : transform(coeffs), translate(translation) {}
        //
        //    // Composition of affine maps
        //    AffineMap operator*(const AffineMap& other) const;
        //    
        //    // Apply the affine transformation to a point
        //    std::vector<int> apply(const std::vector<int>& point) const;
        //    
        //    // Builder pattern interface for API
        //    AffineMap& addCoefficient(int coeff);
        //    AffineMap& addConstant(int constant);
        //
        //private:
        //    std::vector<int> transform;  // Linear coefficients
        //    std::vector<int> translate;  // Translation vector
        //};

        template<typename Scalar = int>
        class AffineMap {
        public:
            // Constructor
            AffineMap(const Matrix<Scalar>& coeffs, const Vector<Scalar>& consts)
                : coefficients(coeffs), constants(consts) {
                inputDimension = coefficients.cols();
                outputDimension = coefficients.rows();
                validateDimensions();
            }
            AffineMap(size_t rows, size_t cols, Scalar value = 0)
                : coefficients(rows, cols, value), constants(rows, value) {
                inputDimension = cols;
                outputDimension = rows;
            }

            // Composition of affine maps (operator*)
            // If f(x) = Ax + b and g(x) = Cx + d, then (g ∘ f)(x) = C(Ax + b) + d = CAx + (Cb + d)
            AffineMap operator*(const AffineMap& other) const {
                // Check dimension compatibility
                if (other.outputDimension != inputDimension) {
                    throw std::invalid_argument(
                        "Incompatible dimensions for composition: " +
                        std::to_string(other.outputDimension) + " != " +
                        std::to_string(inputDimension)
                    );
                }

                // Initialize result matrices
                Matrix<Scalar> resultCoeffs(other.inputDimension, outputDimension, 0);
                Vector<Scalar> resultConsts(outputDimension, 0);

                // Compute CA (matrix multiplication)
                for (int i = 0; i < outputDimension; i++) {
                    for (int j = 0; j < other.inputDimension; j++) {
                        int sum = 0;
                        for (int k = 0; k < inputDimension; k++) {
                            sum += getCoefficient(i, k) * other.getCoefficient(k, j);
                        }
                        resultCoeffs[i][j] = sum;
                    }
                }

                // Compute Cb + d
                for (int i = 0; i < outputDimension; i++) {
                    int sum = constants[i];  // d term
                    for (int j = 0; j < inputDimension; j++) {
                        sum += getCoefficient(i, j) * other.constants[j];  // Cb term
                    }
                    resultConsts[i] = sum;
                }

                return AffineMap(resultCoeffs, resultConsts);
            }

            // Apply the affine transformation to a point
            Vector<Scalar> apply(const Vector<Scalar>& point) const {
                // Validate input dimension
                if (point.size() != inputDimension) {
                    throw std::invalid_argument(
                        "Input point dimension (" + std::to_string(point.size()) +
                        ") does not match map input dimension (" +
                        std::to_string(inputDimension) + ")"
                    );
                }

                // Initialize result vector
                Vector<Scalar> result(outputDimension);

                // Compute Ax + b
                for (int i = 0; i < outputDimension; i++) {
                    int sum = constants[i];  // b term
                    for (int j = 0; j < inputDimension; j++) {
                        sum += getCoefficient(i, j) * point[j];  // Ax term
                    }
                    result[i] = sum;
                }

                return result;
            }

            // Builder pattern interface for fluent API
            AffineMap& addCoefficient(Scalar coeff, int row, int col) {
                // Validate indices
                if (row < 0 || row >= outputDimension ||
                    col < 0 || col >= inputDimension) {
                    throw std::out_of_range(
                        "Invalid coefficient position: (" + std::to_string(row) +
                        "," + std::to_string(col) + ")"
                    );
                }

                setCoefficient(row, col, coeff);
                return *this;
            }

            AffineMap& addConstant(Scalar constant, int index) {
                // Validate index
                if (index < 0 || index >= outputDimension) {
                    throw std::out_of_range(
                        "Invalid constant index: " + std::to_string(index)
                    );
                }

                constants[index] = constant;
                return *this;
            }

            // Getters
            int getInputDimension() const { return inputDimension; }
            int getOutputDimension() const { return outputDimension; }
            const Matrix<Scalar>& getCoefficients() const { return coefficients; }
            const Vector<Scalar>& getConstants() const { return constants; }

            // Create identity map
            static AffineMap identity(int dimension) {
                Matrix<Scalar> coeffs(dimension, dimension, 0);
                Vector<Scalar> consts(dimension, 0);

                // Set diagonal elements to 1
                for (int i = 0; i < dimension; i++) {
                    coeffs[i][i] = 1;
                }

                return AffineMap(coeffs, consts);
            }

            // Create translation map
            static AffineMap translation(const std::vector<int>& translation) {
                size_t dim = translation.size();
                Matrix<Scalar> coeffs(dim, dim, 0);
                Vector<Scalar> consts(translation);

                // Set diagonal elements to 1
                for (int i = 0; i < dim; i++) {
                    coeffs[i][i] = 1;
                }

                return AffineMap(coeffs, consts);
            }

        private:
            Matrix<Scalar> coefficients;  // Linear coefficients stored in row-major order
            Vector<Scalar> constants;     // Translation vector
            size_t inputDimension;        // Dimension of input space
            size_t outputDimension;       // Dimension of output space

            // Helper to validate dimensions
            void validateDimensions() const {
                /*
                // Check if coefficients vector size matches input and output dimensions
                if (coefficients.size() != inputDimension * outputDimension) {
                    throw std::invalid_argument(
                        "Coefficient matrix size (" + std::to_string(coefficients.size()) +
                        ") does not match dimensions (" + std::to_string(outputDimension) +
                        "x" + std::to_string(inputDimension) + ")"
                    );
                }

                // Check if constants vector size matches output dimension
                if (constants.size() != outputDimension) {
                    throw std::invalid_argument(
                        "Constants vector size (" + std::to_string(constants.size()) +
                        ") does not match output dimension (" +
                        std::to_string(outputDimension) + ")"
                    );
                }
                */
            }

            // Helper to get coefficient at specific matrix position
            int getCoefficient(int row, int col) const {
                return coefficients[row][col];
            }

            // Helper to set coefficient at specific matrix position
            void setCoefficient(int row, int col, int value) {
                coefficients[row][col] = value;
            }

            template<typename _Ty>
            friend std::ostream& operator<<(std::ostream& os, const AffineMap<_Ty>& map);
        };

        template<typename Scalar>
        std::ostream& operator<<(std::ostream& os, const AffineMap<Scalar>& map) {
            os << "AffineMap(" << map.inputDimension << " -> " << map.outputDimension << ")\n";
            os << "Coefficients:\n" << map.coefficients << '\n';
            os << "Constants:\n" << map.constants << '\n';
            return os;
        }

        template<typename Scalar>
        std::string formatAffineMap(const AffineMap<Scalar>& map) {
            std::string str("TBD");
            return str;
        }

    }
}
