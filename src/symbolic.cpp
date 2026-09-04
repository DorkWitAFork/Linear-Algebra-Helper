#include "symbolic.h"

#include <symengine/add.h>
#include <symengine/integer.h>
#include <symengine/mul.h>
#include <symengine/parser.h>
#include <symengine/pow.h>
#include <symengine/simplify.h>

#include <cctype>
#include <stdexcept>

namespace {

std::string exactDecimalLiterals(const std::string &text)
{
    std::string result;
    for (std::size_t i = 0; i < text.size();) {
        const bool startsNumber = std::isdigit(static_cast<unsigned char>(text[i]))
            || (text[i] == '.' && i + 1 < text.size()
                && std::isdigit(static_cast<unsigned char>(text[i + 1])));
        const bool followsIdentifier = i > 0
            && (std::isalnum(static_cast<unsigned char>(text[i - 1])) || text[i - 1] == '_');
        if (!startsNumber || followsIdentifier) {
            result += text[i++];
            continue;
        }

        const std::size_t start = i;
        std::string digits;
        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
            digits += text[i++];
        }

        std::size_t fractionalDigits = 0;
        bool hasDecimal = false;
        if (i < text.size() && text[i] == '.') {
            hasDecimal = true;
            ++i;
            while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
                digits += text[i++];
                ++fractionalDigits;
            }
        }

        long long exponent = 0;
        bool hasExponent = false;
        if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
            const std::size_t exponentStart = i++;
            if (i < text.size() && (text[i] == '+' || text[i] == '-')) ++i;
            const std::size_t exponentDigits = i;
            while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) ++i;
            if (i == exponentDigits) {
                i = exponentStart;
            } else {
                hasExponent = true;
                exponent = std::stoll(text.substr(exponentStart + 1, i - exponentStart - 1));
            }
        }

        if (!hasDecimal && !hasExponent) {
            result.append(text, start, i - start);
            continue;
        }

        const auto firstNonzero = digits.find_first_not_of('0');
        digits = firstNonzero == std::string::npos ? "0" : digits.substr(firstNonzero);
        if (digits == "0") {
            result += '0';
            continue;
        }

        const long long scale = static_cast<long long>(fractionalDigits) - exponent;
        if (scale > 0) {
            result += "(" + digits + "/10**" + std::to_string(scale) + ")";
        } else if (scale < 0) {
            result += "(" + digits + "*10**" + std::to_string(-scale) + ")";
        } else {
            result += digits;
        }
    }
    return result;
}

} // namespace

namespace la {

Scalar parseScalar(const std::string &text)
{
    if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument("Expression cannot be blank.");
    }
    return SymEngine::parse(exactDecimalLiterals(text), true);
}

Scalar scalarAdd(const Scalar &a, const Scalar &b) { return SymEngine::add(a, b); }
Scalar scalarSub(const Scalar &a, const Scalar &b) { return SymEngine::sub(a, b); }
Scalar scalarMul(const Scalar &a, const Scalar &b) { return SymEngine::mul(a, b); }
Scalar scalarDiv(const Scalar &a, const Scalar &b) { return SymEngine::div(a, b); }
Scalar scalarNeg(const Scalar &value) { return SymEngine::neg(value); }
Scalar scalarPower(const Scalar &base, const Scalar &exponent) { return SymEngine::pow(base, exponent); }

Scalar scalarSimplify(const Scalar &value)
{
    return SymEngine::simplify(SymEngine::expand(value));
}

bool isZero(const Scalar &value)
{
    return SymEngine::eq(*scalarSimplify(value), *SymEngine::zero);
}

bool isOne(const Scalar &value)
{
    return SymEngine::eq(*scalarSimplify(value), *SymEngine::one);
}

bool equal(const Scalar &a, const Scalar &b)
{
    return isZero(scalarSub(a, b));
}

long asInteger(const Scalar &value)
{
    const auto reduced = scalarSimplify(value);
    if (!SymEngine::is_a<SymEngine::Integer>(*reduced)) {
        throw std::invalid_argument("Expected an integer, got " + reduced->__str__() + ".");
    }
    return SymEngine::down_cast<const SymEngine::Integer &>(*reduced).as_int();
}

std::string scalarText(const Scalar &value)
{
    return value->__str__();
}

} // namespace la
