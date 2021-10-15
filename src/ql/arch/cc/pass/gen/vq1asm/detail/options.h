/**
 * @file    arch/cc/pass/gen/vq1asm/detail/options.h
 * @date    20201007
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   options for Central Controller backend
 * @note
 */

#pragma once

#include "types.h"

// constants
#define CC_BACKEND_VERSION_STRING       "0.4.0"

// options
#define OPT_CC_SCHEDULE_RC              1   // 1=use resource constraint scheduler
#define OPT_SUPPORT_STATIC_CODEWORDS    1   // support (currently: require) static codewords, instead of allocating them on demand
#define OPT_STATIC_CODEWORDS_ARRAYS     1   // JSON field static_codeword_override is an array with one element per qubit parameter
#define OPT_VECTOR_MODE                 0   // 1=generate single code word for all output groups together (requires codewords allocated by backend)

// NOTE JvS: I added these to the documentation string for the backend pass, so
//  you can see these values at runtime.

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

/**
 * Maximum number of instruments in config file.
 */
static const UInt MAX_INSTRS = 12;

/**
 * Parsed configuration option structure for the backend/code generator.
 */
struct Options {

    /**
     * Filename prefix for all output files.
     */
    Str output_prefix;

    /**
     * Input map filename.
     */
    Str map_input_file;

    /**
     * Whether verbose comments should be added to the generated .vq1asm file.
     */
    Bool verbose;

    /**
     * When set, create a .vq1asm program that runs once instead of repeating
     * indefinitely.
     */
    Bool run_once;

};

/**
 * Reference to a constant options object.
 */
using OptionsRef = Ptr<const Options>;

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
