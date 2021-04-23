/** \file
 * Platform header for target-specific compilation.
 */

#pragma once

#include <iostream>
#include "ql/utils/num.h"
#include "ql/utils/str.h"

namespace ql {
namespace arch {

/**
 * The architecture class that a platform belongs to. Different architectures
 * may have their own passes and resources, and probably have their own code
 * generator. The entries in this enumeration (none aside) should be equal to
 * the corresponding namespace (if any) aside from capitalization.
 */
enum class Architecture {

    /**
     * No particular architecture. No shorthands are generated for arch.* in the
     * resource and pass factories.
     */
    NONE,

    /**
     * QuTech Central Controller architecture.
     */
    CC,

    /**
     * CC-light architecture. The backend itself is no longer supported as it
     * has been phased out in our labs; only remnants of it remain because it
     * happens to be the architecture most *compiler* research is still using.
     */
    CC_LIGHT

};

/**
 * String conversion for Architecture. Returns a string exactly equal to the
 * corresponding namespace, or an empty string for NONE.
 */
std::ostream &operator<<(std::ostream &os, Architecture a);

/**
 * Inverse of operator<< for architecture.
 */
Architecture architecture_from_string(const utils::Str &s);

/**
 * Dumps help information about the available architectures.
 */
void dump_architectures(std::ostream &os = std::cout, const utils::Str &line_prefix = "");

} // namespace arch
} // namespace ql
