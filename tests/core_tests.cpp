#include "algorithms.h"
#include "expression.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void check(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

void checkScalar(const la::Scalar &actual, const std::string &expected, const std::string &message)
{
    check(la::equal(actual, la::parseScalar(expected)), message + ": got " + la::scalarText(actual));
}

} // namespace

int main()
{
    try {
        checkScalar(la::parseScalar("0.5 + 1/3"), "5/6", "exact scalar parsing");

        const la::Matrix a{{1, 2}, {3, 4}};
        const la::Matrix b{{2, 0}, {1, 2}};
        const la::Matrix product = la::matrixMultiply(a, b);
        checkScalar(product.at(0, 0), "4", "matrix multiplication [0,0]");
        checkScalar(product.at(1, 1), "8", "matrix multiplication [1,1]");
        checkScalar(la::determinant(a), "-2", "determinant");

        const la::Matrix inv = la::inverse(a);
        checkScalar(inv.at(0, 0), "-2", "inverse [0,0]");
        checkScalar(inv.at(0, 1), "1", "inverse [0,1]");
        checkScalar(inv.at(1, 0), "3/2", "inverse [1,0]");

        const la::Matrix dependent{{1, 2, 3}, {2, 4, 6}};
        const auto reduction = la::rowReduce(dependent, true);
        check(reduction.pivotColumns.size() == 1, "RREF pivot count");
        check(la::nullspace(dependent).size() == 2, "nullity");
        check(la::columnspace(dependent).size() == 1, "column-space basis");

        const la::Matrix uniqueSystem{{1, 1, 3}, {1, -1, 1}};
        const auto unique = la::solveAugmentedSystem(uniqueSystem);
        check(unique.consistent && unique.unique, "unique system classification");
        checkScalar(unique.rref.at(0, 2), "2", "unique x1");
        checkScalar(unique.rref.at(1, 2), "1", "unique x2");

        const la::Matrix inconsistentSystem{{1, 1, 1}, {1, 1, 2}};
        check(!la::solveAugmentedSystem(inconsistentSystem).consistent, "inconsistent system classification");

        std::map<std::string, la::Matrix> matrices{{"A", a}, {"B", b}};
        const auto expression = la::evaluateExpression("det(A) + A[0,1]", matrices);
        checkScalar(std::get<la::Scalar>(expression), "0", "expression evaluator");
        const auto matrixExpression = la::evaluateExpression("A*B + eye(2)", matrices);
        checkScalar(std::get<la::Matrix>(matrixExpression).at(0, 0), "5", "matrix expression evaluator");
        checkScalar(std::get<la::Scalar>(la::evaluateExpression("-2**2", matrices)), "-4", "Python-compatible power precedence");
        checkScalar(std::get<la::Scalar>(la::evaluateExpression("Abs(-2)", matrices)), "2", "absolute-value function");
        checkScalar(std::get<la::Scalar>(la::evaluateExpression("Rational(2, 3)", matrices)), "2/3", "Rational function");

        la::Matrix parameterSystem(2, 3);
        parameterSystem.at(0, 0) = la::one();
        parameterSystem.at(0, 1) = la::one();
        parameterSystem.at(0, 2) = la::integer(2);
        parameterSystem.at(1, 0) = la::integer(2);
        parameterSystem.at(1, 1) = la::integer(2);
        parameterSystem.at(1, 2) = la::parseScalar("h");
        const std::string consistency = la::consistencyReport(parameterSystem);
        check(consistency.find("h - 4 = 0") != std::string::npos
                  || consistency.find("-4 + h = 0") != std::string::npos,
              "symbolic consistency condition: " + consistency);

        const std::string eigen = la::eigenReport(la::Matrix{{2, 0}, {0, 3}});
        check(eigen.find("Eigenvalue: 2") != std::string::npos && eigen.find("Eigenvalue: 3") != std::string::npos,
              "finite eigenvalue and eigenvector report");

        la::Matrix collidingSystem(1, 3);
        collidingSystem.at(0, 0) = la::parseScalar("x2");
        collidingSystem.at(0, 1) = la::one();
        collidingSystem.at(0, 2) = la::zero();
        const auto collisionSolution = la::solveAugmentedSystem(collidingSystem);
        check(collisionSolution.solutionLines.back().find("t1") != std::string::npos,
              "free variable must not collide with an input symbol");

        la::Matrix lambdaMatrix(1, 1);
        lambdaMatrix.at(0, 0) = la::parseScalar("lambda");
        check(la::eigenReport(lambdaMatrix).find("lambda2") != std::string::npos,
              "eigenvalue variable must not collide with an input symbol");

        bool rejectedMinimumExponent = false;
        try { (void)la::matrixPower(a, std::numeric_limits<long>::min()); }
        catch (const std::out_of_range &) { rejectedMinimumExponent = true; }
        check(rejectedMinimumExponent, "minimum matrix exponent must be rejected safely");

        std::cout << "All LA Helper core tests passed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception &error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
