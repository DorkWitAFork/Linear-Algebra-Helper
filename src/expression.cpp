#include "expression.h"

#include "algorithms.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace la {

namespace {

enum class TokenKind { End, Number, Identifier, Plus, Minus, Star, Slash, Power, At, LParen, RParen, Comma, Dot, LBracket, RBracket };

struct Token {
    TokenKind kind;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(std::string input) : input_(std::move(input)) {}

    Token next()
    {
        while (position_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[position_]))) ++position_;
        if (position_ == input_.size()) return {TokenKind::End, {}};
        const char current = input_[position_];
        if (std::isdigit(static_cast<unsigned char>(current)) || (current == '.' && position_ + 1 < input_.size()
            && std::isdigit(static_cast<unsigned char>(input_[position_ + 1])))) {
            const std::size_t start = position_++;
            while (position_ < input_.size() && (std::isdigit(static_cast<unsigned char>(input_[position_]))
                || input_[position_] == '.')) ++position_;
            if (position_ < input_.size() && (input_[position_] == 'e' || input_[position_] == 'E')) {
                ++position_;
                if (position_ < input_.size() && (input_[position_] == '+' || input_[position_] == '-')) ++position_;
                while (position_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[position_]))) ++position_;
            }
            return {TokenKind::Number, input_.substr(start, position_ - start)};
        }
        if (std::isalpha(static_cast<unsigned char>(current)) || current == '_') {
            const std::size_t start = position_++;
            while (position_ < input_.size() && (std::isalnum(static_cast<unsigned char>(input_[position_])) || input_[position_] == '_')) ++position_;
            return {TokenKind::Identifier, input_.substr(start, position_ - start)};
        }
        ++position_;
        switch (current) {
        case '+': return {TokenKind::Plus, "+"};
        case '-': return {TokenKind::Minus, "-"};
        case '*':
            if (position_ < input_.size() && input_[position_] == '*') { ++position_; return {TokenKind::Power, "**"}; }
            return {TokenKind::Star, "*"};
        case '^': return {TokenKind::Power, "^"};
        case '/': return {TokenKind::Slash, "/"};
        case '@': return {TokenKind::At, "@"};
        case '(': return {TokenKind::LParen, "("};
        case ')': return {TokenKind::RParen, ")"};
        case ',': return {TokenKind::Comma, ","};
        case '.': return {TokenKind::Dot, "."};
        case '[': return {TokenKind::LBracket, "["};
        case ']': return {TokenKind::RBracket, "]"};
        default: throw std::invalid_argument(std::string("Unexpected character: ") + current);
        }
    }

private:
    std::string input_;
    std::size_t position_ = 0;
};

const Scalar &requireScalar(const ExpressionValue &value)
{
    if (!std::holds_alternative<Scalar>(value)) throw std::invalid_argument("This operation requires a scalar value.");
    return std::get<Scalar>(value);
}

const Matrix &requireMatrix(const ExpressionValue &value)
{
    if (!std::holds_alternative<Matrix>(value)) throw std::invalid_argument("This operation requires a matrix value.");
    return std::get<Matrix>(value);
}

ExpressionValue addValues(const ExpressionValue &left, const ExpressionValue &right, bool subtractValues)
{
    if (std::holds_alternative<Scalar>(left) && std::holds_alternative<Scalar>(right)) {
        return scalarSimplify(subtractValues ? scalarSub(requireScalar(left), requireScalar(right)) : scalarAdd(requireScalar(left), requireScalar(right)));
    }
    if (std::holds_alternative<Matrix>(left) && std::holds_alternative<Matrix>(right)) {
        return subtractValues ? matrixSub(requireMatrix(left), requireMatrix(right)) : matrixAdd(requireMatrix(left), requireMatrix(right));
    }
    throw std::invalid_argument("Addition and subtraction require two scalars or two equally-sized matrices.");
}

ExpressionValue multiplyValues(const ExpressionValue &left, const ExpressionValue &right)
{
    if (std::holds_alternative<Scalar>(left) && std::holds_alternative<Scalar>(right))
        return scalarSimplify(scalarMul(requireScalar(left), requireScalar(right)));
    if (std::holds_alternative<Matrix>(left) && std::holds_alternative<Matrix>(right))
        return matrixMultiply(requireMatrix(left), requireMatrix(right));
    if (std::holds_alternative<Matrix>(left) && std::holds_alternative<Scalar>(right))
        return scalarMultiply(requireMatrix(left), requireScalar(right));
    if (std::holds_alternative<Scalar>(left) && std::holds_alternative<Matrix>(right))
        return scalarMultiply(requireMatrix(right), requireScalar(left));
    throw std::invalid_argument("Text reports cannot participate in arithmetic.");
}

class Parser {
public:
    Parser(std::string input, const std::map<std::string, Matrix> &matrices)
        : lexer_(std::move(input)), matrices_(matrices), current_(lexer_.next()) {}

    ExpressionValue parse()
    {
        auto result = parseAdditive();
        if (current_.kind != TokenKind::End) throw std::invalid_argument("Unexpected token: " + current_.text);
        return result;
    }

private:
    void advance() { current_ = lexer_.next(); }
    void expect(TokenKind kind, const std::string &message)
    {
        if (current_.kind != kind) throw std::invalid_argument(message);
        advance();
    }

    ExpressionValue parseAdditive()
    {
        auto value = parseMultiplicative();
        while (current_.kind == TokenKind::Plus || current_.kind == TokenKind::Minus) {
            const bool subtractValues = current_.kind == TokenKind::Minus;
            advance();
            value = addValues(value, parseMultiplicative(), subtractValues);
        }
        return value;
    }

    ExpressionValue parseMultiplicative()
    {
        auto value = parseUnary();
        while (current_.kind == TokenKind::Star || current_.kind == TokenKind::At || current_.kind == TokenKind::Slash) {
            const TokenKind operation = current_.kind;
            advance();
            auto right = parseUnary();
            if (operation == TokenKind::Slash) {
                const Scalar divisor = requireScalar(right);
                if (std::holds_alternative<Scalar>(value)) value = scalarSimplify(scalarDiv(requireScalar(value), divisor));
                else value = scalarDivide(requireMatrix(value), divisor);
            } else {
                value = multiplyValues(value, right);
            }
        }
        return value;
    }

    ExpressionValue parsePower()
    {
        auto value = parsePostfix();
        if (current_.kind == TokenKind::Power) {
            advance();
            const auto exponent = parseUnary();
            if (std::holds_alternative<Scalar>(value)) value = scalarPower(requireScalar(value), requireScalar(exponent));
            else value = matrixPower(requireMatrix(value), asInteger(requireScalar(exponent)));
        }
        return value;
    }

    ExpressionValue parseUnary()
    {
        if (current_.kind == TokenKind::Plus) { advance(); return parseUnary(); }
        if (current_.kind == TokenKind::Minus) {
            advance();
            auto value = parseUnary();
            if (std::holds_alternative<Scalar>(value)) return scalarNeg(requireScalar(value));
            return scalarMultiply(requireMatrix(value), integer(-1));
        }
        return parsePower();
    }

    ExpressionValue parsePostfix()
    {
        auto value = parsePrimary();
        while (current_.kind == TokenKind::Dot || current_.kind == TokenKind::LBracket) {
            if (current_.kind == TokenKind::LBracket) {
                advance();
                const long row = asInteger(requireScalar(parseAdditive()));
                expect(TokenKind::Comma, "Matrix indexing requires a row and column, such as A[0, 1].");
                const long col = asInteger(requireScalar(parseAdditive()));
                expect(TokenKind::RBracket, "Expected ']' after matrix index.");
                if (row < 0 || col < 0) throw std::out_of_range("Matrix indices cannot be negative.");
                value = requireMatrix(value).at(static_cast<std::size_t>(row), static_cast<std::size_t>(col));
                continue;
            }
            advance();
            if (current_.kind != TokenKind::Identifier) throw std::invalid_argument("Expected an attribute after '.'.");
            const std::string method = current_.text;
            advance();
            if (method == "T") {
                value = transpose(requireMatrix(value));
                continue;
            }
            expect(TokenKind::LParen, "Matrix methods must be called with parentheses.");
            expect(TokenKind::RParen, "Matrix methods do not accept arguments.");
            value = callFunction(method, {value});
        }
        return value;
    }

    ExpressionValue parsePrimary()
    {
        if (current_.kind == TokenKind::Number) {
            const auto value = parseScalar(current_.text);
            advance();
            return value;
        }
        if (current_.kind == TokenKind::LParen) {
            advance();
            auto value = parseAdditive();
            expect(TokenKind::RParen, "Expected ')'.");
            return value;
        }
        if (current_.kind != TokenKind::Identifier) throw std::invalid_argument("Expected a number, symbol, matrix, or function call.");
        const std::string name = current_.text;
        advance();
        if (current_.kind == TokenKind::LParen) {
            advance();
            std::vector<ExpressionValue> arguments;
            if (current_.kind != TokenKind::RParen) {
                while (true) {
                    arguments.push_back(parseAdditive());
                    if (current_.kind != TokenKind::Comma) break;
                    advance();
                }
            }
            expect(TokenKind::RParen, "Expected ')' after function arguments.");
            return callFunction(name, arguments);
        }
        const auto found = matrices_.find(name);
        if (found != matrices_.end()) return found->second;
        return parseScalar(name);
    }

    ExpressionValue callFunction(const std::string &name, const std::vector<ExpressionValue> &args)
    {
        auto oneMatrix = [&]() -> const Matrix & {
            if (args.size() != 1) throw std::invalid_argument(name + " requires one argument.");
            return requireMatrix(args[0]);
        };
        if (name == "det") return determinant(oneMatrix());
        if (name == "rank") return integer(static_cast<long>(rank(oneMatrix())));
        if (name == "nullity") return integer(static_cast<long>(oneMatrix().cols() - rank(oneMatrix())));
        if (name == "trace") return trace(oneMatrix());
        if (name == "transpose") return transpose(oneMatrix());
        if (name == "inv" || name == "inverse") return inverse(oneMatrix());
        if (name == "rref") return rowReduce(oneMatrix(), true).reduced;
        if (name == "echelon" || name == "echelon_form") return rowReduce(oneMatrix(), false).reduced;
        if (name == "nullspace") return vectorsText(nullspace(oneMatrix()), "The nullspace basis is empty.");
        if (name == "columnspace") return vectorsText(columnspace(oneMatrix()), "The column space basis is empty.");
        if (name == "eigenvals" || name == "eigenvects") return eigenReport(oneMatrix());
        if (name == "simplify") {
            if (args.size() != 1) throw std::invalid_argument("simplify requires one argument.");
            if (std::holds_alternative<Scalar>(args[0])) return scalarSimplify(requireScalar(args[0]));
            const Matrix &source = requireMatrix(args[0]);
            Matrix result(source.rows(), source.cols());
            for (std::size_t i = 0; i < source.rows(); ++i)
                for (std::size_t j = 0; j < source.cols(); ++j) result.at(i, j) = scalarSimplify(source.at(i, j));
            return result;
        }
        if (name == "zeros" || name == "ones") {
            if (args.size() != 2) throw std::invalid_argument(name + " requires rows and columns.");
            const long rows = asInteger(requireScalar(args[0])), cols = asInteger(requireScalar(args[1]));
            if (rows < 1 || cols < 1) throw std::invalid_argument("Matrix dimensions must be positive.");
            return Matrix(static_cast<std::size_t>(rows), static_cast<std::size_t>(cols), name == "ones" ? one() : zero());
        }
        if (name == "eye") {
            if (args.size() != 1) throw std::invalid_argument("eye requires one size argument.");
            const long size = asInteger(requireScalar(args[0]));
            if (size < 1) throw std::invalid_argument("Matrix dimensions must be positive.");
            return Matrix::identity(static_cast<std::size_t>(size));
        }
        if (name == "diag") {
            std::vector<Scalar> values;
            for (const auto &arg : args) values.push_back(requireScalar(arg));
            return Matrix::diagonal(values);
        }
        if (name == "Rational") {
            if (args.size() != 2) throw std::invalid_argument("Rational requires a numerator and denominator.");
            return scalarSimplify(scalarDiv(requireScalar(args[0]), requireScalar(args[1])));
        }

        std::ostringstream scalarExpression;
        const std::string scalarFunction = name == "ln" ? "log" : (name == "Abs" ? "abs" : name);
        scalarExpression << scalarFunction << '(';
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) scalarExpression << ',';
            scalarExpression << scalarText(requireScalar(args[i]));
        }
        scalarExpression << ')';
        return parseScalar(scalarExpression.str());
    }

    Lexer lexer_;
    const std::map<std::string, Matrix> &matrices_;
    Token current_;
};

} // namespace

ExpressionValue evaluateExpression(const std::string &expression, const std::map<std::string, Matrix> &matrices)
{
    return Parser(expression, matrices).parse();
}

std::string expressionValueText(const ExpressionValue &value)
{
    if (std::holds_alternative<Scalar>(value)) return scalarText(std::get<Scalar>(value));
    if (std::holds_alternative<Matrix>(value)) return matrixText(std::get<Matrix>(value));
    return std::get<std::string>(value);
}

} // namespace la
