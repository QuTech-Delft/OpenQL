/** \file
 * Utilities for working with booleans and numbers.
 *
 * This basically just wraps a few common std functions and types using our code
 * style. Besides code style, providing just a few types here also promotes
 * uniformity, and allows types to be wrapped or changed later on for platform
 * independence or avoiding undefined behavior.
 */

#include "utils/num.h"
#include "utils/str.h"
#include "utils/exception.h"

namespace ql {
namespace utils {

/**
 * Converts an Int to a UInt with a range check.
 */
UInt itou(Int x) {
    if (x < 0) {
        throw Exception("Int " + to_string(x) + " out of UInt range");
    }
    return (UInt)x;
}

/**
 * Converts a UInt to an Int with a range check.
 */
Int utoi(UInt x) {
    if (x > (UInt)std::numeric_limits<Int>::max()) {
        throw Exception("UInt " + to_string(x) + " out of Int range");
    }
    return (Int)x;
}

} // namespace utils
} // namespace ql
