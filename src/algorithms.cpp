#include "algorithms.h"

#include <symengine/constants.h>
#include <symengine/sets.h>
#include <symengine/solve.h>
#include <symengine/symbol.h>
#include <symengine/visitor.h>

#include <algorithm>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace la {

namespace {

std::string rowName(std::size_t row) { return "R" + std::to_string(row + 1); }

bool matrixEqual(const Matrix &a, const Matrix &b)
{
    if (a.rows() != b.rows() || a.cols() != b.cols()) return false;
    for (std::size_t row = 0; row < a.rows(); ++row)
        for (std::size_t col = 0; col < a.cols(); ++col)
            if (!equal(a.at(row, col), b.at(row, col))) return false;
    return true;
}

std::set<std::string> symbolNames(const Matrix &matrix)
{
    std::set<std::string> names;
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t col = 0; col < matrix.cols(); ++col) {
            for (const auto &symbol : SymEngine::free_symbols(*matrix.at(row, col))) names.insert(symbol->__str__());
        }
    }
    return names;
}

SymEngine::RCP<const SymEngine::Symbol> freshSymbol(const std::string &preferred, std::set<std::string> &occupied)
{
    std::string name = preferred;
    for (std::size_t suffix = 2; occupied.count(name); ++suffix) name = preferred + std::to_string(suffix);
    occupied.insert(name);
    return SymEngine::symbol(name);
}

} // namespace

ReductionResult rowReduce(const Matrix &matrix, bool toRref, std::size_t pivotColumnLimit)
{
    Matrix reduced = matrix;
    std::vector<std::size_t> pivots;
    std::vector<std::string> steps{"Initial matrix:\n" + matrixText(reduced)};
    const std::size_t limit = std::min(matrix.cols(), pivotColumnLimit);
    std::size_t pivotRow = 0;

    for (std::size_t col = 0; col < limit && pivotRow < reduced.rows(); ++col) {
        std::size_t found = pivotRow;
        while (found < reduced.rows() && isZero(reduced.at(found, col))) ++found;
        if (found == reduced.rows()) continue;

        if (found != pivotRow) {
            reduced.swapRows(found, pivotRow);
            steps.push_back(rowName(pivotRow) + " <-> " + rowName(found) + "\n" + matrixText(reduced));
        }

        const Scalar pivot = scalarSimplify(reduced.at(pivotRow, col));
        if (!isOne(pivot)) {
            for (std::size_t j = 0; j < reduced.cols(); ++j)
                reduced.at(pivotRow, j) = scalarSimplify(scalarDiv(reduced.at(pivotRow, j), pivot));
            steps.push_back(rowName(pivotRow) + " -> (1/(" + scalarText(pivot) + "))" + rowName(pivotRow)
                            + "\n" + matrixText(reduced));
        }

        for (std::size_t row = pivotRow + 1; row < reduced.rows(); ++row) {
            const Scalar factor = scalarSimplify(reduced.at(row, col));
            if (isZero(factor)) continue;
            for (std::size_t j = 0; j < reduced.cols(); ++j)
                reduced.at(row, j) = scalarSimplify(scalarSub(reduced.at(row, j), scalarMul(factor, reduced.at(pivotRow, j))));
            steps.push_back(rowName(row) + " -> " + rowName(row) + " - (" + scalarText(factor) + ")"
                            + rowName(pivotRow) + "\n" + matrixText(reduced));
        }
        pivots.push_back(col);
        ++pivotRow;
    }

    if (toRref) {
        for (std::size_t source = pivots.size(); source-- > 0;) {
            const std::size_t pivotCol = pivots[source];
            for (std::size_t row = 0; row < source; ++row) {
                const Scalar factor = scalarSimplify(reduced.at(row, pivotCol));
                if (isZero(factor)) continue;
                for (std::size_t j = 0; j < reduced.cols(); ++j)
                    reduced.at(row, j) = scalarSimplify(scalarSub(reduced.at(row, j), scalarMul(factor, reduced.at(source, j))));
                steps.push_back(rowName(row) + " -> " + rowName(row) + " - (" + scalarText(factor) + ")"
                                + rowName(source) + "\n" + matrixText(reduced));
            }
        }
    }

    steps.push_back(std::string("Final ") + (toRref ? "RREF:\n" : "echelon form:\n") + matrixText(reduced));
    return {reduced, pivots, steps};
}

Scalar determinant(const Matrix &matrix)
{
    if (matrix.rows() != matrix.cols()) throw std::invalid_argument("The determinant requires a square matrix.");
    if (matrix.rows() == 1) return matrix.at(0, 0);
    Matrix work = matrix;
    Scalar previousPivot = one();
    bool negateResult = false;
    for (std::size_t pivotCol = 0; pivotCol + 1 < work.rows(); ++pivotCol) {
        std::size_t pivotRow = pivotCol;
        while (pivotRow < work.rows() && isZero(work.at(pivotRow, pivotCol))) ++pivotRow;
        if (pivotRow == work.rows()) return zero();
        if (pivotRow != pivotCol) {
            work.swapRows(pivotRow, pivotCol);
            negateResult = !negateResult;
        }
        const Scalar pivot = work.at(pivotCol, pivotCol);
        for (std::size_t row = pivotCol + 1; row < work.rows(); ++row) {
            for (std::size_t col = pivotCol + 1; col < work.cols(); ++col) {
                const Scalar numerator = scalarSub(scalarMul(work.at(row, col), pivot),
                                                   scalarMul(work.at(row, pivotCol), work.at(pivotCol, col)));
                work.at(row, col) = scalarSimplify(scalarDiv(numerator, previousPivot));
            }
            work.at(row, pivotCol) = zero();
        }
        previousPivot = pivot;
    }
    const Scalar result = scalarSimplify(work.at(work.rows() - 1, work.cols() - 1));
    return negateResult ? scalarNeg(result) : result;
}

Scalar trace(const Matrix &matrix)
{
    if (matrix.rows() != matrix.cols()) throw std::invalid_argument("Trace requires a square matrix.");
    Scalar result = zero();
    for (std::size_t i = 0; i < matrix.rows(); ++i) result = scalarAdd(result, matrix.at(i, i));
    return scalarSimplify(result);
}

Matrix inverse(const Matrix &matrix)
{
    if (matrix.rows() != matrix.cols()) throw std::invalid_argument("Only square matrices can be inverted.");
    const std::size_t size = matrix.rows();
    const auto reduced = rowReduce(augment(matrix, Matrix::identity(size)), true, size).reduced;
    const Matrix left = sliceColumns(reduced, 0, size);
    if (!matrixEqual(left, Matrix::identity(size))) throw std::invalid_argument("The matrix is singular and has no inverse.");
    return sliceColumns(reduced, size, size);
}

Matrix matrixPower(const Matrix &matrix, long exponent)
{
    if (matrix.rows() != matrix.cols()) throw std::invalid_argument("Matrix powers require a square matrix.");
    if (exponent == std::numeric_limits<long>::min()) throw std::out_of_range("The matrix exponent is too small.");
    Matrix base = exponent < 0 ? inverse(matrix) : matrix;
    unsigned long remaining = static_cast<unsigned long>(exponent < 0 ? -exponent : exponent);
    Matrix result = Matrix::identity(matrix.rows());
    while (remaining) {
        if (remaining & 1UL) result = matrixMultiply(result, base);
        remaining >>= 1UL;
        if (remaining) base = matrixMultiply(base, base);
    }
    return result;
}

std::size_t rank(const Matrix &matrix)
{
    return rowReduce(matrix, false).pivotColumns.size();
}

std::vector<Matrix> nullspace(const Matrix &matrix)
{
    const auto reduction = rowReduce(matrix, true);
    std::vector<std::size_t> freeColumns;
    for (std::size_t col = 0; col < matrix.cols(); ++col) {
        if (std::find(reduction.pivotColumns.begin(), reduction.pivotColumns.end(), col) == reduction.pivotColumns.end())
            freeColumns.push_back(col);
    }
    std::vector<Matrix> basis;
    for (const std::size_t freeCol : freeColumns) {
        Matrix vector(matrix.cols(), 1);
        vector.at(freeCol, 0) = one();
        for (std::size_t row = 0; row < reduction.pivotColumns.size(); ++row)
            vector.at(reduction.pivotColumns[row], 0) = scalarSimplify(scalarNeg(reduction.reduced.at(row, freeCol)));
        basis.push_back(std::move(vector));
    }
    return basis;
}

std::vector<Matrix> columnspace(const Matrix &matrix)
{
    const auto pivots = rowReduce(matrix, false).pivotColumns;
    std::vector<Matrix> basis;
    for (const std::size_t pivot : pivots) {
        Matrix vector(matrix.rows(), 1);
        for (std::size_t row = 0; row < matrix.rows(); ++row) vector.at(row, 0) = matrix.at(row, pivot);
        basis.push_back(std::move(vector));
    }
    return basis;
}

SystemResult solveAugmentedSystem(const Matrix &augmented)
{
    if (augmented.cols() < 2) throw std::invalid_argument("An augmented matrix needs at least two columns.");
    const std::size_t variables = augmented.cols() - 1;
    const auto reduction = rowReduce(augmented, true);
    SystemResult result;
    result.rref = reduction.reduced;
    result.steps = reduction.steps;
    for (const auto col : reduction.pivotColumns) if (col < variables) result.pivotColumns.push_back(col);
    for (std::size_t col = 0; col < variables; ++col)
        if (std::find(result.pivotColumns.begin(), result.pivotColumns.end(), col) == result.pivotColumns.end())
            result.freeColumns.push_back(col);

    for (std::size_t row = 0; row < result.rref.rows(); ++row) {
        bool coefficientsZero = true;
        for (std::size_t col = 0; col < variables; ++col) coefficientsZero &= isZero(result.rref.at(row, col));
        if (coefficientsZero && !isZero(result.rref.at(row, variables))) result.consistent = false;
    }
    if (!result.consistent) {
        result.solutionLines.push_back("The system is inconsistent and has no solution.");
        return result;
    }

    result.unique = result.freeColumns.empty();
    auto occupiedNames = symbolNames(augmented);
    std::vector<SymEngine::RCP<const SymEngine::Symbol>> freeParameters;
    for (std::size_t index = 0; index < result.freeColumns.size(); ++index)
        freeParameters.push_back(freshSymbol("t" + std::to_string(index + 1), occupiedNames));
    if (result.unique) {
        result.solutionLines.push_back("The system has a unique solution:");
    } else {
        std::string names;
        for (std::size_t index = 0; index < result.freeColumns.size(); ++index) {
            const auto col = result.freeColumns[index];
            if (!names.empty()) names += ", ";
            names += "x" + std::to_string(col + 1) + " = " + freeParameters[index]->__str__();
        }
        result.solutionLines.push_back("The system has infinitely many solutions. Free variable(s): " + names);
    }
    for (std::size_t row = 0; row < result.pivotColumns.size(); ++row) {
        const std::size_t pivot = result.pivotColumns[row];
        Scalar expression = result.rref.at(row, variables);
        for (std::size_t index = 0; index < result.freeColumns.size(); ++index) {
            const auto freeCol = result.freeColumns[index];
            expression = scalarSub(expression, scalarMul(result.rref.at(row, freeCol), freeParameters[index]));
        }
        result.solutionLines.push_back("x" + std::to_string(pivot + 1) + " = " + scalarText(scalarSimplify(expression)));
    }
    return result;
}

std::string multiplicationReport(const Matrix &a, const Matrix &b, Matrix *resultOut)
{
    const Matrix result = matrixMultiply(a, b);
    if (resultOut) *resultOut = result;
    std::ostringstream out;
    out << "Matrix A:\n" << matrixText(a) << "\n\nMatrix B:\n" << matrixText(b)
        << "\n\nEntry-by-entry multiplication:\n";
    for (std::size_t i = 0; i < a.rows(); ++i) {
        for (std::size_t j = 0; j < b.cols(); ++j) {
            out << "C[" << i + 1 << ',' << j + 1 << "] = ";
            for (std::size_t k = 0; k < a.cols(); ++k) {
                if (k) out << " + ";
                out << '(' << scalarText(a.at(i, k)) << ")(" << scalarText(b.at(k, j)) << ')';
            }
            out << " = " << scalarText(result.at(i, j)) << '\n';
        }
    }
    out << "\nResulting Matrix C:\n" << matrixText(result);
    return out.str();
}

std::string solveAxBReport(const Matrix &a, const Matrix &b)
{
    if (b.cols() != 1 || a.rows() != b.rows()) throw std::invalid_argument("b must be a column vector with the same row count as A.");
    const auto solution = solveAugmentedSystem(augment(a, b));
    std::ostringstream out;
    out << "Solving Ax = b\n\nA:\n" << matrixText(a) << "\n\nb:\n" << matrixText(b)
        << "\n\nRREF of [A | b]:\n" << matrixText(solution.rref) << "\n\n";
    for (const auto &line : solution.solutionLines) out << line << '\n';
    return out.str();
}

std::string consistencyReport(const Matrix &augmented)
{
    if (augmented.cols() < 2) throw std::invalid_argument("An augmented matrix needs at least two columns.");
    const auto reduction = rowReduce(augmented, true, augmented.cols() - 1);
    std::vector<Scalar> conditions;
    for (std::size_t row = 0; row < reduction.reduced.rows(); ++row) {
        bool coefficientsZero = true;
        for (std::size_t col = 0; col + 1 < reduction.reduced.cols(); ++col)
            coefficientsZero &= isZero(reduction.reduced.at(row, col));
        if (coefficientsZero && !isZero(reduction.reduced.at(row, reduction.reduced.cols() - 1)))
            conditions.push_back(scalarSimplify(reduction.reduced.at(row, reduction.reduced.cols() - 1)));
    }
    std::ostringstream out;
    out << "Augmented matrix:\n" << matrixText(augmented)
        << "\n\nCoefficient-column row reduction:\n" << matrixText(reduction.reduced) << "\n\n";
    if (conditions.empty()) {
        out << "No contradiction-row conditions remain. The system is consistent for generic parameter values.";
    } else {
        out << "Consistency requires:\n";
        for (const auto &condition : conditions) out << "  " << scalarText(condition) << " = 0\n";
        SymEngine::set_basic parameters;
        for (const auto &condition : conditions) {
            const auto symbols = SymEngine::free_symbols(*condition);
            parameters.insert(symbols.begin(), symbols.end());
        }
        if (parameters.empty()) {
            out << "\nAt least one condition is a nonzero constant, so no parameter value can make the system consistent.";
        } else if (conditions.size() == 1 && parameters.size() == 1) {
            const auto parameter = SymEngine::rcp_static_cast<const SymEngine::Symbol>(*parameters.begin());
            try {
                const auto solutions = SymEngine::solve(conditions.front(), parameter, SymEngine::complexes());
                out << "\nSupported solution set for " << parameter->__str__() << ": " << solutions->__str__();
            } catch (const std::exception &) {
                out << "\nSymEngine could not solve this equation explicitly; the exact condition is preserved above.";
            }
        } else {
            out << "\nSymEngine may leave multi-parameter equation systems unsolved; the exact conditions are preserved above.";
        }
    }
    out << "\n\nNote: symbolic reduction assumes a pivot expression is nonzero unless it simplifies exactly to zero; special parameter cases may need separate checking.";
    return out.str();
}

std::string luReport(const Matrix &matrix)
{
    if (matrix.rows() != matrix.cols()) throw std::invalid_argument("LU factorization requires a square matrix.");
    const std::size_t n = matrix.rows();
    Matrix l = Matrix::identity(n), u = matrix, p = Matrix::identity(n);
    std::vector<std::string> swaps;
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        while (pivot < n && isZero(u.at(pivot, col))) ++pivot;
        if (pivot == n) continue;
        if (pivot != col) {
            u.swapRows(pivot, col);
            p.swapRows(pivot, col);
            for (std::size_t previous = 0; previous < col; ++previous)
                std::swap(l.at(pivot, previous), l.at(col, previous));
            swaps.push_back(rowName(col) + " <-> " + rowName(pivot));
        }
        for (std::size_t row = col + 1; row < n; ++row) {
            if (isZero(u.at(row, col))) continue;
            const Scalar factor = scalarSimplify(scalarDiv(u.at(row, col), u.at(col, col)));
            l.at(row, col) = factor;
            for (std::size_t j = col; j < n; ++j)
                u.at(row, j) = scalarSimplify(scalarSub(u.at(row, j), scalarMul(factor, u.at(col, j))));
        }
    }
    const Matrix pa = matrixMultiply(p, matrix);
    const Matrix lu = matrixMultiply(l, u);
    std::ostringstream out;
    out << "Original Matrix A:\n" << matrixText(matrix) << "\n\nRow swaps: ";
    if (swaps.empty()) out << "none";
    else for (std::size_t i = 0; i < swaps.size(); ++i) out << (i ? ", " : "") << swaps[i];
    out << "\n\nPermutation Matrix P:\n" << matrixText(p)
        << "\n\nLower Triangular Matrix L:\n" << matrixText(l)
        << "\n\nUpper Triangular Matrix U:\n" << matrixText(u)
        << "\n\nP * A:\n" << matrixText(pa) << "\n\nL * U:\n" << matrixText(lu)
        << "\n\nPA = LU verified: " << (matrixEqual(pa, lu) ? "true" : "false");
    return out.str();
}

std::string vectorsText(const std::vector<Matrix> &vectors, const std::string &emptyMessage)
{
    if (vectors.empty()) return emptyMessage;
    std::ostringstream out;
    for (std::size_t i = 0; i < vectors.size(); ++i) {
        if (i) out << "\n\n";
        out << "Vector " << i + 1 << ":\n" << matrixText(vectors[i]);
    }
    return out.str();
}

std::string spacesReport(const Matrix &matrix)
{
    const auto nullBasis = nullspace(matrix);
    const auto columnBasis = columnspace(matrix);
    std::ostringstream out;
    out << "Matrix A:\n" << matrixText(matrix) << "\n\nRank(A): " << columnBasis.size()
        << "\n\nNullity(A): " << nullBasis.size()
        << "\n\nNullspace basis:\n" << vectorsText(nullBasis, "The basis is empty; the nullspace contains only the zero vector.")
        << "\n\nColumn space basis:\n" << vectorsText(columnBasis, "The column space basis is empty.");
    return out.str();
}

std::string eigenReport(const Matrix &matrix)
{
    if (matrix.rows() != matrix.cols()) throw std::invalid_argument("Eigenvalues require a square matrix.");
    auto occupiedNames = symbolNames(matrix);
    const auto lambda = freshSymbol("lambda", occupiedNames);
    Matrix characteristicMatrix = matrix;
    for (std::size_t i = 0; i < matrix.rows(); ++i)
        characteristicMatrix.at(i, i) = scalarSub(characteristicMatrix.at(i, i), lambda);
    const Scalar polynomial = scalarSimplify(determinant(characteristicMatrix));

    std::ostringstream out;
    out << "Matrix A:\n" << matrixText(matrix) << "\n\nCharacteristic polynomial det(A - " << lambda->__str__() << " I):\n"
        << scalarText(polynomial) << "\n\nEigenvalue solution set:\n";
    try {
        const auto roots = SymEngine::solve_poly(polynomial, lambda, SymEngine::complexes());
        out << roots->__str__();

        if (SymEngine::is_a<SymEngine::FiniteSet>(*roots)) {
            const auto &values = SymEngine::down_cast<const SymEngine::FiniteSet &>(*roots).get_container();
            for (const auto &value : values) {
                Matrix shifted = matrix;
                for (std::size_t i = 0; i < shifted.rows(); ++i)
                    shifted.at(i, i) = scalarSub(shifted.at(i, i), value);
                out << "\n\nEigenvalue: " << scalarText(value) << "\nEigenspace basis:\n"
                    << vectorsText(nullspace(shifted), "No basis was produced.");
            }
        } else {
            out << "\n\nSymEngine did not return a finite explicit root set, so eigenvectors cannot be expanded. The symbolic equation above is preserved.";
        }
    } catch (const std::exception &) {
        out << "No explicit solution set was produced. Solve the characteristic equation above for lambda.";
    }
    return out.str();
}

} // namespace la
