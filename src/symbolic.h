#pragma once

#ifdef _MSC_VER
// FLINT and Qt define incompatible global ulong aliases on 64-bit Windows.
#define ulong symengine_ulong
#endif

#include <symengine/basic.h>
#include <symengine/constants.h>
#include <symengine/integer.h>

#ifdef _MSC_VER
#undef ulong
#endif

#include <string>

namespace la {

using Scalar = SymEngine::RCP<const SymEngine::Basic>;

Scalar parseScalar(const std::string &text);
Scalar scalarAdd(const Scalar &a, const Scalar &b);
Scalar scalarSub(const Scalar &a, const Scalar &b);
Scalar scalarMul(const Scalar &a, const Scalar &b);
Scalar scalarDiv(const Scalar &a, const Scalar &b);
Scalar scalarNeg(const Scalar &value);
Scalar scalarPower(const Scalar &base, const Scalar &exponent);
Scalar scalarSimplify(const Scalar &value);
bool isZero(const Scalar &value);
bool isOne(const Scalar &value);
bool equal(const Scalar &a, const Scalar &b);
long asInteger(const Scalar &value);
std::string scalarText(const Scalar &value);

inline Scalar zero() { return SymEngine::zero; }
inline Scalar one() { return SymEngine::one; }
inline Scalar integer(long value) { return SymEngine::integer(value); }

} // namespace la
