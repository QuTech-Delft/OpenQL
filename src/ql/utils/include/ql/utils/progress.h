/** \file
 * Provides utilities for working with strings that the STL fails to
 * satisfactorily provide.
 */

#pragma once

#include <chrono>
#include "ql/utils/num.h"
#include "ql/utils/str.h"

namespace ql {
namespace utils {

/**
 * Progress monitor. Prints progress every N milliseconds (as long as progress
 * is called often enough), including time taken thus far and ETA, using
 * INFO loglevel.
 */
class Progress {
private:

    /**
     * The clock source to use.
     */
    using Clock = std::chrono::high_resolution_clock;

    /**
     * Time points for the clock source.
     */
    using TimePoint = Clock::time_point;

    /**
     * Prefix for printing.
     */
    Str prefix;

    /**
     * Minimum printing interval.
     */
    UInt interval;

    /**
     * Start time (when we were constructed).
     */
    TimePoint start;

    /**
     * The previous time we printed our progress.
     */
    TimePoint prev;

public:

    /**
     * Constructor that doesn't print anything.
     */
    Progress();

    /**
     * Progress monitor constructor. Also starts the timer.
     */
    explicit Progress(const Str &prefix, UInt interval=0);

    /**
     * Prints the current progress and ETA, if enough time has passed since
     * the previous print.
     */
    void feed(Real progress);

    /**
     * Prints a completion message that includes total time taken.
     */
    void complete();

};

} // namespace utils
} // namespace ql
