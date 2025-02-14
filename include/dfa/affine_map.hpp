#pragma once
#include <string>
#include <vector>
#include <stdexcept>

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


class AffineMap {
private:
    std::vector<int> coefficients;  // Linear coefficients stored in row-major order
    std::vector<int> constants;     // Translation vector
    int inputDimension;            // Dimension of input space
    int outputDimension;           // Dimension of output space

    // Helper to validate dimensions
    void validateDimensions() const {
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
    }

    // Helper to get coefficient at specific matrix position
    int getCoefficient(int row, int col) const {
        return coefficients[row * inputDimension + col];
    }

    // Helper to set coefficient at specific matrix position
    void setCoefficient(int row, int col, int value) {
        coefficients[row * inputDimension + col] = value;
    }

public:
    // Constructor
    AffineMap(const std::vector<int>& coeffs, const std::vector<int>& consts,
        int inDim, int outDim)
        : coefficients(coeffs), constants(consts),
        inputDimension(inDim), outputDimension(outDim) {
        validateDimensions();
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
        std::vector<int> resultCoeffs(other.inputDimension * outputDimension, 0);
        std::vector<int> resultConsts(outputDimension);

        // Compute CA (matrix multiplication)
        for (int i = 0; i < outputDimension; i++) {
            for (int j = 0; j < other.inputDimension; j++) {
                int sum = 0;
                for (int k = 0; k < inputDimension; k++) {
                    sum += getCoefficient(i, k) * other.getCoefficient(k, j);
                }
                resultCoeffs[i * other.inputDimension + j] = sum;
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

        return AffineMap(resultCoeffs, resultConsts,
            other.inputDimension, outputDimension);
    }

    // Apply the affine transformation to a point
    std::vector<int> apply(const std::vector<int>& point) const {
        // Validate input dimension
        if (point.size() != inputDimension) {
            throw std::invalid_argument(
                "Input point dimension (" + std::to_string(point.size()) +
                ") does not match map input dimension (" +
                std::to_string(inputDimension) + ")"
            );
        }

        // Initialize result vector
        std::vector<int> result(outputDimension);

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
    AffineMap& addCoefficient(int coeff, int row, int col) {
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

    AffineMap& addConstant(int constant, int index) {
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
    const std::vector<int>& getCoefficients() const { return coefficients; }
    const std::vector<int>& getConstants() const { return constants; }

    // Create identity map
    static AffineMap identity(int dimension) {
        std::vector<int> coeffs(dimension * dimension, 0);
        std::vector<int> consts(dimension, 0);

        // Set diagonal elements to 1
        for (int i = 0; i < dimension; i++) {
            coeffs[i * dimension + i] = 1;
        }

        return AffineMap(coeffs, consts, dimension, dimension);
    }

    // Create translation map
    static AffineMap translation(const std::vector<int>& translation) {
        int dim = translation.size();
        std::vector<int> coeffs(dim * dim, 0);

        // Set diagonal elements to 1
        for (int i = 0; i < dim; i++) {
            coeffs[i * dim + i] = 1;
        }

        return AffineMap(coeffs, translation, dim, dim);
    }
};


inline std::string formatAffineMap(const AffineMap& map) {
	std::string str("TBD");
	return str;
}
