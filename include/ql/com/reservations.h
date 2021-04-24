/** \file
 * Bookkeeping class for tracking cycle range reservations.
 *
 * Primarily intended to be used by resources.
 */

#pragma once

#include <functional>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/map.h"
#include "ql/utils/logger.h"

namespace ql {
namespace com {
namespace reservations {

/**
 * Dummy type for Reservations with no mapped value.
 */
struct Empty {};

/**
 * String conversion for Empty.
 */
std::ostream &operator<<(std::ostream &os, Empty e);

/**
 * A range of cycles, from first (inclusive) to second (exclusive).
 */
using CycleRange = utils::Pair<utils::UInt, utils::UInt>;

/**
 * Result enumeration of find().
 */
enum class Result {

    /**
     * No overlap between the incoming range and any preexisting
     * reservations. No reservation is returned.
     */
    NONE,

    /**
     * The incoming range overlaps partially with a single preexisting
     * reservation, and this reservation is returned.
     */
    PARTIAL,

    /**
     * The incoming range overlaps partially with multiple preexisting
     * reservation. No reservation is returned.
     */
    MULTIPLE,

    /**
     * The incoming range completely envelops a single preexisting
     * reservation, and this reservation is returned.
     */
    SUPER,

    /**
     * The incoming range is completely contained by a single preexisting
     * reservation, and this reservation is returned.
     */
    SUB,

    /**
     * The incoming range matches exactly with a preexisting reservation,
     * and this reservation is returned.
     */
    EXACT

};

/**
 * Tracker for cycle range reservations. Represents a set of non-overlapping
 * cycle ranges optionally mapping to template type T.
 */
template <class T = Empty>
class Tracker {
public:

    /**
     * The map used to store the reservations.
     */
    using ReservationMap = utils::Map<CycleRange, T>;

protected:

    /**
     * The reservations made thus far.
     */
    ReservationMap reservations;

public:

    /**
     * Given a cycle range, returns how it compares to existing reservations.
     * iter receives an iterator to a preexisting reservation, if non-null and
     * so indicated by the return value. See Result for additional information.
     */
    Result find(
        CycleRange range,
        typename ReservationMap::iterator *iter = nullptr
    ) {
        QL_ASSERT(range.second >= range.first);

        // Short-circuit when we have no reservations yet.
        if (reservations.empty()) {
            return Result::NONE;
        }

        // Look for the reservation of which the range immediately follows the
        // given range or is exactly equal to it, as far as std::tie is
        // concerned.
        auto it = reservations.lower_bound(range);

        // If there is no such reservation (end), we may still be overlapping
        // with the range before ours.
        if (it == reservations.end()) {

            // Move one reservation back. We know there must be at least one
            // reservation (because we checked that the map is non-empty), so
            // this should always be valid.
            --it;
            QL_ASSERT(it->first.first <= range.first);

            // Check whether the incoming range is completely enveloped by
            // this reservation. We already know the start of the reservation is
            // before or at the same cycle as the start of the incoming range,
            // so we only have to check the end of the reserved range to do
            // this.
            if (it->first.second >= range.second) {
                // --[===========]--|
                //      ^-----^
                if (iter) *iter = it;
                return Result::SUB;
            }

            // If the incoming range is not completely enveloped, there may
            // still be a partial overlap. This is the case when the start of
            // the incoming range is before the end of the reservation.
            if (it->first.second > range.first) {
                // --[=====]--------|
                //      ^-----^
                if (iter) *iter = it;
                return Result::PARTIAL;
            }

            // No overlap with this one. We also know nothing comes after, so
            // we're done.

            // --[=]------------|
            //      ^-----^
            return Result::NONE;

        }

        // If the range matches, we have an exact match.
        if (it->first == range) {

            // -----[=====]-----
            //      ^-----^
            if (iter) *iter = it;
            return Result::EXACT;
        }

        // We know the reservation in it is ordered after range, so
        // it->first.first >= range.first. Therefore, we can easily check
        // whether the incoming range completely envelops the reservation by
        // checking its end as well.
        QL_ASSERT(it->first.first >= range.first);
        if (it->first.second <= range.second) {

            // --?????[=]?????--
            //      ^-----^

            // In that case, there may be another reservation that overlaps with
            // the incoming range as well. Check the reservation that comes
            // after.
            auto it2 = std::next(it);
            if (it2 != reservations.end()) {
                if (it2->first.first < range.second) {
                    // --?????[=][=???--
                    //      ^-----^
                    return Result::MULTIPLE;
                }
            }

            // Check before as well.
            if (it != reservations.begin()) {
                it2 = std::prev(it);
                if (it2->first.second > range.first) {
                    // --???=][=]-------
                    //     ^------^
                    return Result::MULTIPLE;
                }
            }

            // No other overlaps, so we're enveloping the reservation
            // completely.

            // -------[=]-------
            //      ^-----^
            if (iter) *iter = it;
            return Result::SUPER;

        }

        // Okay, now all we know is that the reservation ends after the range
        // ends. That doesn't mean much.

        // --???????????=]--
        //      ^-----^

        // Let's first check if the reservation even overlaps with range.
        if (it->first.first >= range.second) {

            // Nope, it starts after range.

            // --????????????--[=]--
            //      ^-----^

            // If nothing precedes it, we're done...
            if (it == reservations.begin()) {
                // |---------------[=]--
                //      ^-----^
                return Result::NONE;
            }

            // ... but it's not, so we basically have to check everything again
            // for the reservation that comes before it. All we know about it
            // is that it starts before range.
            it = std::prev(it);
            QL_ASSERT(it->first.first <= range.first);

            // -[=????????????--[=]--
            //      ^-----^

            // Check for complete containment.
            if (it->first.second >= range.second) {
                // --[===========]--[=]--
                //      ^-----^
                if (iter) *iter = it;
                return Result::SUB;
            }

            // If the incoming range is not completely enveloped, there may
            // still be a partial overlap. This is the case when the start of
            // the incoming range is before the end of the reservation.
            if (it->first.second > range.first) {
                // --[=====]---[=]--
                //      ^-----^
                if (iter) *iter = it;
                return Result::PARTIAL;
            }

            // No overlap with this one. We also know nothing that comes after,
            // overlaps, so we're done.

            // --[=]-------[=]--
            //      ^-----^
            return Result::NONE;

        }

        // Okay, so we know it overlaps at the end. We also already know that
        // the reservation must be ordered after range (because of the initial
        // search), so

        // --?????--[====]--
        //      ^-----^

        // We're left with a partial overlap with it, and possibly overlap with
        // something that comes before it. So if nothing comes before it, we're
        // done.
        if (it == reservations.begin()) {
            // |--------[====]--
            //      ^-----^
            if (iter) *iter = it;
            return Result::PARTIAL;
        }

        // Otherwise, check for overlap with the element before. If there's
        // overlap, we're done (for as far as the possible containment results
        // go).
        --it;
        if (it->first.second > range.first) {
            // ??????=]-[====]--
            //      ^-----^
            return Result::MULTIPLE;
        }

        // Otherwise, this was a partial overlap.

        // -[=]-----[====]--
        //      ^-----^
        if (iter) *iter = it;
        return Result::PARTIAL;

    }

    /**
     * Makes the given reservation. Any preexisting reservations that overlap
     * are removed. If replace_all is true, *all* preexisting reservations are
     * first removed.
     */
    void reserve(
        CycleRange range,
        T state = {},
        utils::Bool replace_all = false
    ) {

        // Short-circuit when replace_all is set.
        if (replace_all) {
            reservations.clear();
            reservations.set(range) = state;
            return;
        }

        // Look for the nearest reservation.
        auto to = reservations.lower_bound(range);

        // Short-circuit if it's the exact requested range.
        if (to != reservations.end() && to->first == range) {
            to->second = state;
            return;
        }

        // Otherwise, get rid of all the overlapping reservations first.
        auto from = to;
        while (from != reservations.begin()) {
            --from;
            if (from->first.second <= range.first) {
                ++from;
                break;
            }
        }
        while (to != reservations.end()) {
            if (to->first.first >= range.second) {
                break;
            }
            ++to;
        }
        if (from != to) {
            from = reservations.erase(from, to);
        }

        // Insert the new reservation, using the current position as a hint.
        reservations.emplace_hint(from, utils::Pair<CycleRange, T>(range, state));

    }

    /**
     * Removes all current reservations.
     */
    void reset() {
        reservations.clear();
    }

    /**
     * Dumps the state of this reservation tracker.
     */
    void dump_state(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = "",
        std::function<void(std::ostream&, const T&)> printer
            = [](std::ostream &os, const T &val){ os << utils::try_to_string(val); }
    ) const {
        if (reservations.empty()) {
            os << line_prefix << "no reservations" << std::endl;
            return;
        }
        for (const auto &it : reservations) {
            os << line_prefix << "[" << it.first.first << "," << it.first.second << ") = ";
            printer(os, it.second);
            os << "\n";
        }
    }

};

} // namespace reservations
} // namespace com
} // namespace ql
