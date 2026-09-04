#pragma once

#include "matrix.h"

#include <map>
#include <string>
#include <variant>

namespace la {

using ExpressionValue = std::variant<Scalar, Matrix, std::string>;

ExpressionValue evaluateExpression(const std::string &expression,
                                   const std::map<std::string, Matrix> &matrices);
std::string expressionValueText(const ExpressionValue &value);

} // namespace la
