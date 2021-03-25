/**
 * @file    arch/cc/options_cc.h
 * @date    20201007
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   options for Central Controller backend
 * @note
 */

#pragma once

#include "types_cc.h"

// constants
#define CC_BACKEND_VERSION_STRING       "0.3.1"

// options
#define OPT_CC_SCHEDULE_RC              0   // 1=use resource constraint scheduler
#define OPT_SUPPORT_STATIC_CODEWORDS    1   // support (currently: require) static codewords, instead of allocating them on demand
#define OPT_STATIC_CODEWORDS_ARRAYS     1   // JSON field static_codeword_override is an array with one element per qubit parameter
#define OPT_VECTOR_MODE                 0   // 1=generate single code word for all output groups together (requires codewords allocated by backend)
#define OPT_FEEDBACK                    1   // feedback support. New feature in being developed
#define OPT_PRAGMA                      1   // pragma support, initially for Repeat Until Success only


namespace ql {
namespace arch {
namespace cc {

static const UInt MAX_INSTRS = 12;   // maximum number of instruments in config file

} // namespace cc
} // namespace arch
} // namespace ql
