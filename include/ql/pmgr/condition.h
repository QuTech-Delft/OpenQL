/** \file
 * Contains the condition classes that the pass management logic uses for
 * conditional pass execution.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"

namespace ql {
namespace pmgr {
namespace condition {

/**
 * Base class for the conditions used by GROUP_IF, GROUP_WHILE, and
 * GROUP_REPEAT_UNTIL_NOT pass nodes.
 */
class Base {
public:

    /**
     * Default virtual destructor.
     */
    virtual ~Base() = default;

    /**
     * Evaluates the condition, given the pass return code.
     */
    virtual bool evaluate(utils::Int pass_return_value) const = 0;

    /**
     * Returns a string representation of the condition for debugging.
     */
    virtual utils::Str to_string() const = 0;

};

/**
 * Reference to a pass condition.
 */
using Ref = utils::Ptr<Base>;

/**
 * Immutable reference to a pass condition.
 */
using CRef = utils::Ptr<const Base>;

/**
 * Comparison relation for the Compare class.
 */
enum class Relation {
    EQ, NE, GT, GE, LT, LE
};

/**
 * Class for conditions based on how the value in question compares to a
 * reference value.
 */
class Compare : public Base {
private:

    /**
     * Reference value to compare to.
     */
    utils::Int value;

    /**
     * The relation to use.
     */
    Relation relation;

public:

    /**
     * Constructs a condition that bases its result upon how the the pass return
     * code compares to the given value. If invert is false or unspecified, true is
     * returned when the value is in range. If invert is true, false is returned
     * when the value is in range.
     */
    Compare(utils::Int value, Relation relation = Relation::EQ);

    /**
     * Evaluates the condition, given the pass return code.
     */
    bool evaluate(utils::Int pass_return_value) const override;

    /**
     * Returns a string representation of the condition for debugging.
     */
    utils::Str to_string() const override;

};

/**
 * Class for conditions based on a range of values.
 */
class Range : public Base {
private:

    /**
     * Minimum value in the range.
     */
    utils::Int min;

    /**
     * Maximum value in the range.
     */
    utils::Int max;

    /**
     * Whether to invert the result.
     */
    utils::Bool invert;

public:

    /**
     * Constructs a condition that bases its result upon whether the pass return
     * code is within a certain range. min and max represent the inclusive
     * range. If invert is false or unspecified, true is returned when the
     * value is in range. If invert is true, false is returned when the value is
     * in range.
     */
    Range(utils::Int min, utils::Int max, utils::Bool invert = false);

    /**
     * Evaluates the condition, given the pass return code.
     */
    bool evaluate(utils::Int pass_return_value) const override;

    /**
     * Returns a string representation of the condition for debugging.
     */
    utils::Str to_string() const override;

};

} // namespace condition
} // namespace pmgr
} // namespace ql
