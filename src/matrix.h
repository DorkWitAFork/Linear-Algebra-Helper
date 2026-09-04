#pragma once

#include "symbolic.h"

#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

namespace la {

class Matrix {
public:
    Matrix() = default;
    Matrix(std::size_t rows, std::size_t cols, Scalar fill = zero());
    Matrix(std::initializer_list<std::initializer_list<long>> values);

    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }
    bool empty() const { return rows_ == 0 || cols_ == 0; }

    const Scalar &at(std::size_t row, std::size_t col) const;
    Scalar &at(std::size_t row, std::size_t col);
    void swapRows(std::size_t first, std::size_t second);

    static Matrix identity(std::size_t size);
    static Matrix diagonal(const std::vector<Scalar> &values);

private:
    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    std::vector<Scalar> data_;
};

Matrix matrixAdd(const Matrix &a, const Matrix &b);
Matrix matrixSub(const Matrix &a, const Matrix &b);
Matrix matrixMultiply(const Matrix &a, const Matrix &b);
Matrix scalarMultiply(const Matrix &matrix, const Scalar &scalar);
Matrix scalarDivide(const Matrix &matrix, const Scalar &scalar);
Matrix transpose(const Matrix &matrix);
Matrix augment(const Matrix &left, const Matrix &right);
Matrix sliceColumns(const Matrix &matrix, std::size_t first, std::size_t count);
std::string matrixText(const Matrix &matrix);

} // namespace la
