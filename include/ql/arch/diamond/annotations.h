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
    utils::UInt amplitude;
};

struct MemSwapParameters {
    utils::UInt nuclear;
};

struct QEntangleParameters {
    utils::UInt nuclear;
};

struct SweepBiasParameters {
    utils::UInt value;
    utils::UInt dacreg;
    utils::UInt start;
    utils::UInt step;
    utils::UInt max;
    utils::UInt memaddress;
};

struct CRCParameters {
    utils::UInt threshold;
    utils::UInt value;
};

struct RabiParameters {
    utils::UInt measurements;
    utils::UInt duration;
    utils::UInt t_max;
};



} // namespace annotations
} // namespace diamond
} // namespace arch
} // namespace ql
