/** \file
 * Utilities for working with booleans and numbers.
 *
 * This basically just wraps a few common std functions and types using our code
 * style. Besides code style, providing just a few types here also promotes
 * uniformity, and allows types to be wrapped or changed later on for platform
 * independence or avoiding undefined behavior.
 */

#pragma once

#include <cinttypes>
#include <cmath>
#include <complex>
#include <limits>

namespace ql {
namespace utils {

/**
 * Typedef for doubles. This just maps to a primitive bool.
 */
using Bool = bool;

/**
 * Shorthand for unsigned integers. This is based on uint64_t.
 */
using UInt = uint64_t;

/**
 * Shorthand for unsigned integers. This is based on int64_t.
 */
using Int = int64_t;

/**
 * Typedef for real numbers. This just maps to a primitive double.
 */
using Real = double;

/**
 * Complex number. This maps to std::complex<double>.
 */
class Complex : public std::complex<double> {
public:
    Complex(Real re = 0.0, Real im = 0.0) noexcept : complex(re, im) {}
    Complex(const complex &other) noexcept : complex(other.real(), other.imag()) {}
};

/**
 * Maximum value for an Int.
 */
const Int MAX = std::numeric_limits<Int>::max();

/**
 * Constant for pi.
 */
const Real PI = M_PI;

/**
 * Euler's constant.
 */
const Real EU = M_E;

/**
 * Imaginary constant.
 */
const Complex IM = Complex(0.0, 1.0);

/**
 * Returns the sign of the given number.
 */
template<typename T>
int sign_of(T val) noexcept {
    return (T(0) < val) - (val < T(0));
}

/**
 * Rounds the given double toward positive infinity.
 */
inline Real ceil(Real x) noexcept {
    return std::ceil(x);
}

/**
 * Rounds the given double toward the nearest integer.
 */
inline Real round(Real x) noexcept {
    return std::round(x);
}

/**
 * Rounds the given double toward negative infinity.
 */
inline Real floor(Real x) noexcept {
    return std::floor(x);
}

/**
 * Rounds the given double away from zero.
 */
inline Real round_away_from_zero(Real x) noexcept {
    return std::ceil(std::abs(x)) * sign_of(x);
}

/**
 * Rounds the given double toward zero.
 */
inline Real round_toward_zero(Real x) noexcept {
    return std::floor(std::abs(x)) * sign_of(x);
}

/**
 * Returns the absolute value of two numbers.
 */
template <typename T>
inline T abs(T x) noexcept {
    return std::abs(x);
}

/**
 * Returns the maximum value of two numbers.
 */
template <typename T>
inline T max(T x, T y) noexcept {
    return std::max(x, y);
}

/**
 * Returns the minimum value of two numbers.
 */
template <typename T>
inline T min(T x, T y) noexcept {
    return std::min(x, y);
}

/**
 * Integer version of a base-2 logarithm, rounding down.
 */
inline UInt log2(UInt n) noexcept {
#define S(k) if (n >= (UInt(1) << k)) { i += k; n >>= k; }
    UInt i = -(n == 0); S(32); S(16); S(8); S(4); S(2); S(1); return i;
#undef S
}

/**
 * Integer version of a base-2 exponent, rounding down.
 */
inline UInt pow2(UInt n) noexcept {
    return UInt(1) << n;
}

/**
 * Natural exponent.
 */
inline Real exp(Real e) noexcept {
    return std::exp(e);
}

/**
 * Natural exponent with imaginary argument.
 */
inline Complex expi(Real e) noexcept {
    return std::exp(IM * e);
}

/**
 * Natural logarithm.
 */
inline Real log(Real e) noexcept {
    return std::log(e);
}

/**
 * Square.
 */
inline Real sqr(Real e) noexcept {
    return e * e;
}

/**
 * Square-root.
 */
inline Real sqrt(Real e) noexcept {
    return std::sqrt(e);
}

} // namespace utils
} // namespace ql
