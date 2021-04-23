/** \file
 * Platform header for target-specific compilation.
 */

#include "ql/arch/architecture.h"

#include "ql/utils/exception.h"

namespace ql {
namespace arch {

/**
 * String conversion for Architecture.
 */
std::ostream &operator<<(std::ostream &os, Architecture a) {
    switch (a) {
        case Architecture::NONE:     os << "";         break;
        case Architecture::CC:       os << "cc";       break;
        case Architecture::CC_LIGHT: os << "cc_light"; break;
    }
    return os;
}

/**
 * Inverse of operator<< for architecture.
 */
Architecture architecture_from_string(const utils::Str &s) {
    if (s.empty()) {
        return Architecture::NONE;
    } else if (s == "cc") {
        return Architecture::CC;
    } else if (s == "cc_light") {
        return Architecture::CC_LIGHT;
    } else {
        throw utils::Exception("unknown architecture name " + s);
    }
}

/**
 * Dumps help information about the available architectures.
 */
void dump_architectures(std::ostream &os, const utils::Str &line_prefix) {
    os << line_prefix << "TODO" << std::endl;
}

} // namespace arch
} // namespace ql
