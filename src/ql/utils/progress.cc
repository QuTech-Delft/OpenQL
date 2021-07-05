/** \file
 * Provides utilities for working with strings that the STL fails to
 * satisfactorily provide.
 */

#include "ql/utils/progress.h"

#include <iomanip>
#include "ql/utils/logger.h"

namespace ql {
namespace utils {

/**
 * Constructor that doesn't print anything.
 */
Progress::Progress() :
    prefix(),
    interval(0)
{
    start = Clock::now();
    prev = start;
}

/**
 * Progress monitor constructor. Also starts the timer.
 */
Progress::Progress(
    const Str &prefix,
    UInt interval
) :
    prefix(prefix),
    interval(interval)
{
    QL_IOUT(prefix << ": starting...");
    start = Clock::now();
    prev = start;
}

/**
 * Prints the current progress and ETA, if enough time has passed since
 * the previous print.
 */
void Progress::feed(Real progress) {
    if (prefix.empty()) return;
    auto now = Clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - prev).count() > (Int)interval) {
        UInt millis_thus_far = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        UInt millis_eta = 0;
        if (progress > 0.01 && progress < 1) {
            millis_eta = millis_thus_far / progress - millis_thus_far;
        }
        utils::StrStrm ss;
        ss << std::fixed << std::setprecision(2) << (progress * 100) << "%";
        ss << " after " << (millis_thus_far / 1000) << "s";
        if (millis_eta) {
            ss << ", ETA " << (millis_eta / 1000) << "s";
        }
        QL_IOUT(prefix << ": " << ss.str());
        prev = now;
    }
}

/**
 * Prints a completion message that includes total time taken.
 */
void Progress::complete() {
    if (prefix.empty()) return;
    auto now = Clock::now();
    UInt millis_thus_far = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    QL_IOUT(prefix << ": completed within " << millis_thus_far << "ms");
    prefix.clear();
}

} // namespace utils
} // namespace ql
