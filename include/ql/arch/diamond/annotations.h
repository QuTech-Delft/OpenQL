/** \file
 * Defines annotations about the diamond architecture.
 */

#pragma once

#include "ql/utils/num.h"

namespace ql {
namespace arch {
namespace diamond {
namespace annotations {

struct ExciteMicrowaveParameters {
    utils::UInt envelope;
    utils::UInt duration;
    utils::UInt frequency;
    utils::UInt phase;
};



} // namespace annotations
} // namespace diamond
} // namespace arch
} // namespace ql
