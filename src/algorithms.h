#pragma once

#include "matrix.h"

#include <cstddef>
#include <string>
#include <vector>

namespace la {

struct ReductionResult {
    Matrix reduced;
    std::vector<std::size_t> pivotColumns;
    std::vector<std::string> steps;
};

struct SystemResult {
    Matrix rref;
    std::vector<std::size_t> pivotColumns;
    std::vector<std::size_t> freeColumns;
    bool consistent = true;
    bool unique = false;
    std::vector<std::string> solutionLines;
    std::vector<std::string> steps;
};

ReductionResult rowReduce(const Matrix &matrix, bool toRref = true,
                          std::size_t pivotColumnLimit = static_cast<std::size_t>(-1));
Scalar determinant(const Matrix &matrix);
Scalar trace(const Matrix &matrix);
Matrix inverse(const Matrix &matrix);
Matrix matrixPower(const Matrix &matrix, long exponent);
std::size_t rank(const Matrix &matrix);
std::vector<Matrix> nullspace(const Matrix &matrix);
std::vector<Matrix> columnspace(const Matrix &matrix);
SystemResult solveAugmentedSystem(const Matrix &augmented);

std::string multiplicationReport(const Matrix &a, const Matrix &b, Matrix *result = nullptr);
std::string solveAxBReport(const Matrix &a, const Matrix &b);
std::string consistencyReport(const Matrix &augmented);
std::string luReport(const Matrix &matrix);
std::string spacesReport(const Matrix &matrix);
std::string eigenReport(const Matrix &matrix);
std::string vectorsText(const std::vector<Matrix> &vectors, const std::string &emptyMessage);

} // namespace la
