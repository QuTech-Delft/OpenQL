/** \file
 * Type declarations specific to OpenQL that don't really belong elsewhere.
 */

#pragma once

#include "ql/utils/options.h"
#include "ql/utils/compat.h"

namespace ql {
namespace com {

/**
 * The direction in which gates are scheduled. Used by the scheduler passes
 * (obviously), but also by the resource management.
 */
enum class SchedulingDirection {

    /**
     * Schedule the first instruction first, as done by the ASAP algorithm.
     */
    FORWARD,

    /**
     * Schedule the last instruction first, as done by the ALAP algorithm.
     */
    BACKWARD

};

/**
 * Stream operator for SchedulingDirection.
 */
std::ostream &operator<<(std::ostream &os, SchedulingDirection dir);

} // namespace com
} // namespace ql
