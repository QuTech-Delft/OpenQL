/** \file
 * A map (and set) mapping from non-overlapping *ranges* of keys to values.
 */

#pragma once

#include <functional>
#include "ql/utils/pair.h"
#include "ql/utils/map.h"
#include "ql/utils/exception.h"
#include "ql/utils/ptr.h"

namespace ql {
namespace utils {

/**
 * Dummy type for Reservations with no mapped value.
 */
struct Nothing {};

/**
 * String conversion for Empty.
 */
std::ostream &operator<<(std::ostream &os, const Nothing &e);

/**
 * Result enumeration for find().
 */
enum class RangeMatchType {

    /**
     * No overlap between the incoming range and any preexisting ranges.
     * Zero ranges are returned.
     */
    NONE,

    /**
     * The incoming range overlaps partially with a single preexisting
     * range. This range is returned.
     */
    PARTIAL,

    /**
     * The incoming range overlaps partially with multiple preexisting
     * ranges. These ranges are returned.
     */
    MULTIPLE,

    /**
     * The incoming range completely envelops a single preexisting range.
     * This range is returned.
     */
    SUPER,

    /**
     * The incoming range is completely contained by a single preexisting
     * range. This range is returned.
     */
    SUB,

    /**
     * The incoming range matches an existing range exactly. This range is
     * returned.
     */
    EXACT

};

/**
 * String conversion for RangeMatchType.
 */
std::ostream &operator<<(std::ostream &os, RangeMatchType rmt);

/**
 * A map (and set) mapping from non-overlapping *ranges* of keys to values.
 *
 * The map can either keep all ranges as inserted, or it can optimize by merging
 * consecutive ranges upon insertion when a given value comparator function
 * indicates that the values for the consecutive ranges are equal. By default
 * ranges are kept as-is; alternative comparators can be presented to the
 * constructor or to set().
 */
template <typename K, typename V, typename C = std::less<K>>
class RangeMap {
public:

    /**
     * The key type.
     */
    using Key = K;

    /**
     * Comparator for keys. operator() returns whether the left-hand side
     * sorts before the right-hand side.
     */
    using KeyCompare = C;

    /**
     * The type for a range of keys.
     */
    using Range = utils::Pair<Key, Key>;

    /**
     * Comparator for ranges. operator() returns whether the left-hand side
     * sorts before the right-hand side.
     */
    struct RangeCompare {
        KeyCompare key_compare;
        utils::Bool operator()(const Range& lhs, const Range& rhs) const {
            if (key_compare(lhs.first, rhs.first)) {
                return true;
            } else if (key_compare(rhs.first, lhs.first)) {
                return false;
            } else {
                return key_compare(rhs.second, lhs.second);
            }
        }
    };

    /**
     * The value type.
     */
    using Value = V;

    /**
     * Value comparator type.
     */
    using ValueCompare = std::function<utils::Bool(const V &a, const V &b)>;

    /**
     * The map type this is built upon.
     */
    using Map = utils::Map<Range, Value, RangeCompare>;

    /**
     * Mutable iterator.
     */
    using Iter = typename Map::Iter;

    /**
     * Constant iterator.
     */
    using ConstIter = typename Map::ConstIter;

    /**
     * Mutable iterator.
     */
    using ReverseIter = typename Map::ReverseIter;

    /**
     * Constant iterator.
     */
    using ConstReverseIter = typename Map::ConstReverseIter;

    /**
     * Mutable result for the find operation.
     */
    struct FindResult {

        /**
         * The result of the find operation.
         */
        RangeMatchType type;

        /**
         * Iterator pointing to the first range returned, or to the insertion
         * position for the given range if no ranges are returned
         * (begin == end).
         */
        Iter begin;

        /**
         * Iterator pointing to the range after the last range returned, or to
         * the insertion position for the given range if no ranges are returned
         * (begin == end).
         */
        Iter end;

    };

    /**
     * Immutable result for the find operation.
     */
    struct ConstFindResult {

        /**
         * The result of the find operation.
         */
        RangeMatchType type;

        /**
         * Iterator pointing to the first range returned, or to the insertion
         * position for the given range if no ranges are returned
         * (begin == end).
         */
        ConstIter begin;

        /**
         * Iterator pointing to the range after the last range returned, or to
         * the insertion position for the given range if no ranges are returned
         * (begin == end).
         */
        ConstIter end;

    };

private:

    /**
     * The underlying map.
     */
    Map map = {};

    /**
     * Comparator for keys.
     */
    KeyCompare key_compare = {};

    /**
     * Comparator for ranges.
     */
    RangeCompare range_compare = {};

    /**
     * Value comparator, for merging consecutive ranges.
     */
    ValueCompare value_compare = [](const V &a, const V &b) { return false; };

public:

    /**
     * Creates an empty range map that does not automatically optimize
     * consecutive ranges.
     */
    RangeMap() = default;

    /**
     * Creates an empty range map that automatically optimizes consecutive
     * ranges when the value for the two ranges is equal according to the given
     * equality predicate.
     */
    explicit RangeMap(
        ValueCompare &&value_compare
    ) :
        value_compare(std::forward<ValueCompare>(value_compare))
    {}

    /**
     * Utility for key less-than comparison.
     */
    inline utils::Bool key_lt(const Key &a, const Key &b) const {
        return key_compare(a, b);
    }

    /**
     * Utility for key greater-equal comparison.
     */
    inline utils::Bool key_ge(const Key &a, const Key &b) const {
        return !key_compare(a, b);
    }

    /**
     * Utility for key greater-than comparison.
     */
    inline utils::Bool key_gt(const Key &a, const Key &b) const {
        return key_compare(b, a);
    }

    /**
     * Utility for key less-equal comparison.
     */
    inline utils::Bool key_le(const Key &a, const Key &b) const {
        return !key_compare(b, a);
    }

    /**
     * Utility for key equality comparison.
     */
    inline utils::Bool key_eq(const Key &a, const Key &b) const {
        return key_le(a, b) && !key_lt(a, b);
    }

    /**
     * Utility for key inequality comparison.
     */
    inline utils::Bool key_ne(const Key &a, const Key &b) const {
        return !key_eq(a, b);
    }

    /**
     * Determines whether the given range is valid.
     */
    inline utils::Bool range_valid(const Range &a) const {
        return key_le(a.first, a.second);
    }

    /**
     * Determines whether the given range is empty.
     */
    inline utils::Bool range_empty(const Range &a) const {
        return key_eq(a.first, a.second);
    }

    /**
     * Determines whether range a completely envelops range b.
     */
    inline utils::Bool range_envelop(const Range &a, const Range &b) const {
        return key_le(a.first, b.first) && key_ge(a.second, b.second);
    }

    /**
     * Determines whether two ranges are exactly equal.
     */
    inline utils::Bool range_equal(const Range &a, const Range &b) const {
        return key_eq(a.first, b.first) && key_eq(a.second, b.second);
    }

    /**
     * Determines whether range a starts before range b.
     */
    inline utils::Bool range_starts_before(const Range &a, const Range &b) const {
        return key_lt(a.first, b.first);
    }

    /**
     * Determines whether range a ends after range b.
     */
    inline utils::Bool range_ends_after(const Range &a, const Range &b) const {
        return key_gt(a.second, b.second);
    }

    /**
     * Determines whether range a is entirely before range b.
     */
    inline utils::Bool range_entirely_before(const Range &a, const Range &b) const {
        return key_le(a.second, b.first);
    }

    /**
     * Determines whether range a ends exactly when b starts.
     */
    inline utils::Bool range_consecutive(const Range &a, const Range &b) const {
        return key_eq(a.second, b.first);
    }

    /**
     * Throws an exception if any of the ranges are invalid.
     */
    void check_consistency() const {
        auto prev = map.end();
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (!range_valid(it->first)) {
                throw utils::Exception("RangeMap invariant failed: found null range");
            }
            if (prev != map.end()) {
                if (!range_entirely_before(prev->first, it->first)) {
                    throw utils::Exception("RangeMap invariant failed: found overlapping range");
                }
            }
            prev = it;
        }
    }

private:

    /**
     * Returns the ranges that overlap with the given range. The given range may
     * be a null range.
     */
    utils::Pair<Iter, Iter> find_internal(const Range &range) {
        // NOTE: keep in sync with const variant!
        auto first = map.lower_bound(range);
        auto last = first;
        while (first != map.begin()) {
            auto it = std::prev(first);
            if (range_entirely_before(it->first, range)) break;
            first = it;
        }
        while (last != map.end()) {
            if (range_entirely_before(range, last->first)) break;
            ++last;
        }
        return {first, last};
    }

    /**
     * Returns the ranges that overlap with the given range. The given range may
     * be a null range.
     */
    utils::Pair<ConstIter, ConstIter> find_internal(const Range &range) const {
        // NOTE: keep in sync with mutable variant!
        auto first = map.lower_bound(range);
        auto last = first;
        while (first != map.begin()) {
            auto it = std::prev(first);
            if (range_entirely_before(it->first, range)) break;
            first = it;
        }
        while (last != map.end()) {
            if (range_entirely_before(range, last->first)) break;
            ++last;
        }
        return {first, last};
    }

    /**
     * Given a range and the set of overlapping existing ranges, determines the
     * type of match.
     */
    RangeMatchType get_match_type(
        const Range &range,
        const ConstIter &first,
        const ConstIter &last
    ) const {
        if (first == last) {
            return RangeMatchType::NONE;
        } else if (std::next(first) != last) {
            return RangeMatchType::MULTIPLE;
        } else if (range_equal(first->first, range)) {
            return RangeMatchType::EXACT;
        } else if (range_envelop(range, first->first)) {
            return RangeMatchType::SUPER;
        } else if (range_envelop(first->first, range)) {
            return RangeMatchType::SUB;
        } else {
            return RangeMatchType::PARTIAL;
        }
    }

public:

    /**
     * Finds all ranges in the map that overlap with the given range.
     */
    FindResult find(const Range &range) {
        if (!range_valid(range)) {
            throw utils::Exception(
                "Invalid range presented to find(): " + utils::try_to_string(range)
            );
        }
        auto its = find_internal(range);
        return {
            get_match_type(range, its.first, its.second),
            its.first,
            its.second
        };
    }

    /**
     * Finds all ranges in the map that overlap with the given range.
     */
    ConstFindResult find(const Range &range) const {
        if (!range_valid(range)) {
            throw utils::Exception(
                "Invalid range presented to find(): " + utils::try_to_string(range)
            );
        }
        auto its = find_internal(range);
        return {
            get_match_type(range, its.first, its.second),
            its.first,
            its.second
        };
    }

    /**
     * Returns an iterator to the range that contains the given key, or end() if
     * no such range exists.
     */
    Iter find(const Key &key) {
        auto its = find_internal({key, key});
        if (its.first == its.second) {
            return map.end();
        } else {
            return its.first;
        }
    }

    /**
     * Returns an iterator to the range that contains the given key, or end() if
     * no such range exists.
     */
    ConstIter find(const Key &key) const {
        auto its = find_internal({key, key});
        if (its.first == its.second) {
            return map.end();
        } else {
            return its.first;
        }
    }

    /**
     * Returns the value associated with the given range. If there is no exact
     * map for the range, an exception is thrown.
     */
    Value &at(const Range &range) {
        return map.at(range);
    }

    /**
     * Returns the value associated with the given range. If there is no exact
     * map for the range, an exception is thrown.
     */
    const Value &at(const Range &range) const {
        return map.at(range);
    }

    /**
     * Replaces the given range with a mapping to the given value. If the new
     * range is adjacent to existing ranges that are equal according to
     * compare(value, existing_value), the ranges will be merged. Returns an
     * iterator to the inserted range.
     */
    Iter set(Range range, const Value &value, const ValueCompare &compare) {
        if (!range_valid(range)) {
            throw utils::Exception(
                "Invalid range presented to set(): " + utils::try_to_string(range)
            );
        }

        // Look for existing overlapping ranges and update them if necessary.
        auto its = find_internal(range);
        auto before_it = its.first;
        auto after_it = its.second;
        utils::Ptr<Range> before_range;
        utils::Ptr<Value> before_value;
        utils::Ptr<Range> after_range;
        utils::Ptr<Value> after_value;
        if (before_it != map.end() && range_starts_before(before_it->first, range)) {
            if (compare(value, before_it->second)) {

                // Overlapping preexisting range before new range compares
                // equal, so we extend the to-be-added range.
                range.first = before_it->first.first;

            } else {

                // Overlapping preexisting range before new range compares
                // unequal, so we have to re-add it.
                before_range.emplace(before_it->first.first, range.first);
                before_value.emplace(before_it->second);

            }
        } else if (before_it != map.begin()) {
            auto it = std::prev(before_it);
            if (range_consecutive(it->first, range) && compare(value, it->second)) {

                // The range immediately before the inserted range has a
                // value that compares equal, so we have to merge with it.
                before_it = it;
                range.first = before_it->first.first;

            }
        }
        if (after_it != map.begin() && range_ends_after(std::prev(after_it)->first, range)) {
            if (compare(value, std::prev(after_it)->second)) {

                // Overlapping preexisting range after new range compares
                // equal, so we extend the to-be-added range.
                range.second = std::prev(after_it)->first.second;

            } else {

                // Overlapping preexisting range after new range compares
                // unequal, so we have to re-add it.
                after_range.emplace(range.second, std::prev(after_it)->first.second);
                after_value.emplace(std::prev(after_it)->second);

            }
        } else if (after_it != map.end()) {
            if (range_consecutive(range, after_it->first) && compare(value, after_it->second)) {

                // The range immediately after the inserted range has a
                // value that compares equal, so we have to merge with it.
                range.second = after_it->first.second;
                ++after_it;

            }
        }

        // Erase the overlapping ranges.
        map.erase(before_it, after_it);

        // Add the updated ranges before and after, if any.
        if (before_range.has_value()) {
            map.insert({std::move(*before_range), std::move(*before_value)});
        }
        if (after_range.has_value()) {
            map.insert({std::move(*after_range), std::move(*after_value)});
        }

        // Add the new range.
        return map.insert({range, value}).first;

    }

    /**
     * Replaces the given range with a mapping to the given or default value.
     * If the new range is adjacent to existing ranges that are equal according
     * to the value comparator specified at construction (if any), the ranges
     * will be merged. Returns an iterator to the inserted range.
     */
    Iter set(const Range &range, const Value &value = {}) {
        return set(range, value, value_compare);
    }

    /**
     * Replaces the given range with a mapping to the default value. If the new
     * range is adjacent to existing ranges that are equal according to
     * compare(value, existing_value), the ranges will be merged. Returns an
     * iterator to the inserted range.
     */
    Iter set(const Range &range, const ValueCompare &compare) {
        return set(range, {}, value_compare);
    }

    /**
     * Erases the given range.
     */
    void erase(Range range) {
        if (!range_valid(range)) {
            throw utils::Exception(
                "Invalid range presented to set(): " + utils::try_to_string(range)
            );
        }

        auto its = find_internal(range);
        auto before_it = its.first;
        auto after_it = its.second;
        utils::Ptr<Range> before_range;
        utils::Ptr<Value> before_value;
        utils::Ptr<Range> after_range;
        utils::Ptr<Value> after_value;
        if (before_it != map.end() && range_starts_before(before_it->first, range)) {

            // Must re-add partially-overlapping preexisting range at the start
            // of the to-be-erased range.
            before_range.emplace(before_it->first.first, range.first);
            before_value.emplace(before_it->second);

        }
        if (after_it != map.begin() && range_ends_after(std::prev(after_it)->first, range)) {

            // Must re-add partially-overlapping preexisting range at the end
            // of the to-be-erased range.
            after_range.emplace(range.second, std::prev(after_it)->first.second);
            after_value.emplace(std::prev(after_it)->second);

        }

        // Erase the overlapping ranges.
        map.erase(before_it, after_it);

        // Add the updated ranges before and after, if any.
        if (before_range.has_value()) {
            map.insert({std::move(*before_range), std::move(*before_value)});
        }
        if (after_range.has_value()) {
            map.insert({std::move(*after_range), std::move(*after_value)});
        }

    }

    /**
     * Returns an iterator to the first element of the map. If the map is empty,
     * the returned iterator will be equal to end().
     */
    Iter begin() {
        return map.begin();
    }

    /**
     * Returns an iterator to the first element of the map. If the map is empty,
     * the returned iterator will be equal to end().
     */
    ConstIter begin() const {
        return map.begin();
    }

    /**
     * Returns an iterator to the first element of the map. If the map is empty,
     * the returned iterator will be equal to end().
     */
    ConstIter cbegin() const {
        return map.cbegin();
    }

    /**
     * Returns an iterator to the element following the last element of the map.
     * This element acts as a placeholder; attempting to access it results in an
     * exception.
     */
    Iter end() {
        return map.end();
    }

    /**
     * Returns an iterator to the element following the last element of the map.
     * This element acts as a placeholder; attempting to access it results in an
     * exception.
     */
    ConstIter end() const {
        return map.end();
    }

    /**
     * Returns an iterator to the element following the last element of the map.
     * This element acts as a placeholder; attempting to access it results in an
     * exception.
     */
    ConstIter cend() const {
        return map.cend();
    }

    /**
     * Returns a reverse iterator to the first element of the reversed map. It
     * corresponds to the last element of the non-reversed map. If the map is
     * empty, the returned iterator is equal to rend().
     */
    ReverseIter rbegin() {
        return map.rbegin();
    }

    /**
     * Returns a reverse iterator to the first element of the reversed map. It
     * corresponds to the last element of the non-reversed map. If the map is
     * empty, the returned iterator is equal to rend().
     */
    ConstReverseIter rbegin() const {
        return map.rbegin();
    }

    /**
     * Returns a reverse iterator to the first element of the reversed map. It
     * corresponds to the last element of the non-reversed map. If the map is
     * empty, the returned iterator is equal to rend().
     */
    ConstReverseIter crbegin() const {
        return map.crbegin();
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed map. It corresponds to the element preceding the first
     * element of the non-reversed map. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ReverseIter rend() {
        return map.rend();
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed map. It corresponds to the element preceding the first
     * element of the non-reversed map. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ConstReverseIter rend() const {
        return map.rend();
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed map. It corresponds to the element preceding the first
     * element of the non-reversed map. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ConstReverseIter crend() const {
        return map.crend();
    }

    /**
     * Returns whether the map is empty.
     */
    bool empty() const {
        return map.empty();
    }

    /**
     * Returns the number of ranges.
     */
    typename Map::size_type size() const {
        return map.size();
    }

    /**
     * Erases all ranges.
     */
    void clear() {
        map.clear();
    }

    /**
     * Dumps the state as a multiline string. When Value is not Nothing, the
     * optional printer callback may be used to change the way the value is
     * printed.
     */
    void dump_state(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = "",
        std::function<void(std::ostream&, const V&)> printer
            = [](std::ostream &os, const V &val){ os << utils::try_to_string(val); }
    ) const {
        if (map.empty()) {
            os << line_prefix << "empty" << std::endl;
            return;
        }
        for (const auto &it : map) {
            os << line_prefix << "[" << it.first.first << ".." << it.first.second << ")";
            if (!std::is_same<Value, Nothing>::value) {
                os << " => ";
                printer(os, it.second);
            }
            os << std::endl;
        }
    }

    /**
     * Converts the state to a string for debugging.
     */
    utils::Str to_string() const {
        if (map.empty()) {
            return "empty";
        }
        utils::StrStrm ss;
        ss << "{";
        utils::Bool first = true;
        for (const auto &it : map) {
            if (first) {
                first = false;
            } else {
                ss << ", ";
            }
            ss << "[" << it.first.first << ".." << it.first.second << ")";
            if (!std::is_same<Value, Nothing>::value) {
                ss << ": " << utils::try_to_string(it.second);
            }
        }
        ss << "}";
        return ss.str();
    }

    /**
     * String conversion for RangeMap.
     */
    friend std::ostream &operator<<(std::ostream &os, const RangeMap &rm) {
        return os << rm.to_string();
    }

};

/**
 * Convenience typedef for RangeMaps that don't map to anything significant and
 * thus behave like a set instead.
 */
template <typename K, typename C = std::less<K>>
using RangeSet = RangeMap<K, Nothing, C>;

} // namespace utils
} // namespace ql
