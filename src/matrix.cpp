#include "matrix.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace la {

Matrix::Matrix(std::size_t rows, std::size_t cols, Scalar fill)
    : rows_(rows), cols_(cols)
{
    if (rows == 0 || cols == 0) {
        throw std::invalid_argument("A matrix must have at least one row and one column.");
    }
    if (rows > std::vector<Scalar>().max_size() / cols) {
        throw std::length_error("Matrix dimensions are too large.");
    }
    data_.assign(rows * cols, std::move(fill));
}

Matrix::Matrix(std::initializer_list<std::initializer_list<long>> values)
{
    rows_ = values.size();
    if (rows_ == 0) {
        throw std::invalid_argument("A matrix cannot be empty.");
    }
    cols_ = values.begin()->size();
    if (cols_ == 0) {
        throw std::invalid_argument("A matrix cannot have empty rows.");
    }
    for (const auto &row : values) {
        if (row.size() != cols_) {
            throw std::invalid_argument("All matrix rows must have the same length.");
        }
        for (long value : row) {
            data_.push_back(integer(value));
        }
    }
}

const Scalar &Matrix::at(std::size_t row, std::size_t col) const
{
    if (row >= rows_ || col >= cols_) {
        throw std::out_of_range("Matrix index is out of range.");
    }
    return data_[row * cols_ + col];
}

Scalar &Matrix::at(std::size_t row, std::size_t col)
{
    return const_cast<Scalar &>(std::as_const(*this).at(row, col));
}

void Matrix::swapRows(std::size_t first, std::size_t second)
{
    if (first >= rows_ || second >= rows_) {
        throw std::out_of_range("Matrix row is out of range.");
    }
    for (std::size_t col = 0; col < cols_; ++col) {
        std::swap(at(first, col), at(second, col));
    }
}

Matrix Matrix::identity(std::size_t size)
{
    Matrix result(size, size);
    for (std::size_t i = 0; i < size; ++i) {
        result.at(i, i) = one();
    }
    return result;
}

Matrix Matrix::diagonal(const std::vector<Scalar> &values)
{
    if (values.empty()) {
        throw std::invalid_argument("diag requires at least one value.");
    }
    Matrix result(values.size(), values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        result.at(i, i) = values[i];
    }
    return result;
}

static void requireSameShape(const Matrix &a, const Matrix &b)
{
    if (a.rows() != b.rows() || a.cols() != b.cols()) {
        throw std::invalid_argument("Matrices must have the same dimensions.");
    }
}

Matrix matrixAdd(const Matrix &a, const Matrix &b)
{
    requireSameShape(a, b);
    Matrix result(a.rows(), a.cols());
    for (std::size_t i = 0; i < a.rows(); ++i)
        for (std::size_t j = 0; j < a.cols(); ++j)
            result.at(i, j) = scalarSimplify(scalarAdd(a.at(i, j), b.at(i, j)));
    return result;
}

Matrix matrixSub(const Matrix &a, const Matrix &b)
{
    requireSameShape(a, b);
    Matrix result(a.rows(), a.cols());
    for (std::size_t i = 0; i < a.rows(); ++i)
        for (std::size_t j = 0; j < a.cols(); ++j)
            result.at(i, j) = scalarSimplify(scalarSub(a.at(i, j), b.at(i, j)));
    return result;
}

Matrix matrixMultiply(const Matrix &a, const Matrix &b)
{
    if (a.cols() != b.rows()) {
        throw std::invalid_argument("Matrix multiplication requires columns of A to equal rows of B.");
    }
    Matrix result(a.rows(), b.cols());
    for (std::size_t i = 0; i < result.rows(); ++i) {
        for (std::size_t j = 0; j < result.cols(); ++j) {
            Scalar sum = zero();
            for (std::size_t k = 0; k < a.cols(); ++k)
                sum = scalarAdd(sum, scalarMul(a.at(i, k), b.at(k, j)));
            result.at(i, j) = scalarSimplify(sum);
        }
    }
    return result;
}

Matrix scalarMultiply(const Matrix &matrix, const Scalar &scalar)
{
    Matrix result(matrix.rows(), matrix.cols());
    for (std::size_t i = 0; i < matrix.rows(); ++i)
        for (std::size_t j = 0; j < matrix.cols(); ++j)
            result.at(i, j) = scalarSimplify(scalarMul(matrix.at(i, j), scalar));
    return result;
}

Matrix scalarDivide(const Matrix &matrix, const Scalar &scalar)
{
    if (isZero(scalar)) throw std::invalid_argument("Division by zero.");
    Matrix result(matrix.rows(), matrix.cols());
    for (std::size_t i = 0; i < matrix.rows(); ++i)
        for (std::size_t j = 0; j < matrix.cols(); ++j)
            result.at(i, j) = scalarSimplify(scalarDiv(matrix.at(i, j), scalar));
    return result;
}

Matrix transpose(const Matrix &matrix)
{
    Matrix result(matrix.cols(), matrix.rows());
    for (std::size_t i = 0; i < matrix.rows(); ++i)
        for (std::size_t j = 0; j < matrix.cols(); ++j)
            result.at(j, i) = matrix.at(i, j);
    return result;
}

Matrix augment(const Matrix &left, const Matrix &right)
{
    if (left.rows() != right.rows()) {
        throw std::invalid_argument("Matrices must have equal row counts to be augmented.");
    }
    Matrix result(left.rows(), left.cols() + right.cols());
    for (std::size_t i = 0; i < result.rows(); ++i) {
        for (std::size_t j = 0; j < left.cols(); ++j) result.at(i, j) = left.at(i, j);
        for (std::size_t j = 0; j < right.cols(); ++j) result.at(i, left.cols() + j) = right.at(i, j);
    }
    return result;
}

Matrix sliceColumns(const Matrix &matrix, std::size_t first, std::size_t count)
{
    if (first + count > matrix.cols() || count == 0) {
        throw std::out_of_range("Requested matrix columns are out of range.");
    }
    Matrix result(matrix.rows(), count);
    for (std::size_t i = 0; i < matrix.rows(); ++i)
        for (std::size_t j = 0; j < count; ++j)
            result.at(i, j) = matrix.at(i, first + j);
    return result;
}

std::string matrixText(const Matrix &matrix)
{
    std::vector<std::vector<std::string>> cells(matrix.rows(), std::vector<std::string>(matrix.cols()));
    std::vector<std::size_t> widths(matrix.cols(), 1);
    for (std::size_t i = 0; i < matrix.rows(); ++i) {
        for (std::size_t j = 0; j < matrix.cols(); ++j) {
            cells[i][j] = scalarText(matrix.at(i, j));
            widths[j] = std::max(widths[j], cells[i][j].size());
        }
    }
    std::ostringstream out;
    for (std::size_t i = 0; i < matrix.rows(); ++i) {
        out << "[ ";
        for (std::size_t j = 0; j < matrix.cols(); ++j) {
            if (j) out << "  ";
            out << std::setw(static_cast<int>(widths[j])) << cells[i][j];
        }
        out << " ]";
        if (i + 1 < matrix.rows()) out << '\n';
    }
    return out.str();
}

} // namespace la
