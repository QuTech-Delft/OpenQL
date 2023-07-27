/** \file
 * Contains the condition classes that the pass management logic uses for
 * conditional pass execution.
 */

#include "ql/pmgr/condition.h"

namespace ql {
namespace pmgr {
namespace condition {

/**
 * Constructs a condition that bases its result upon how the the pass return
 * code compares to the given value. If invert is false or unspecified, true is
 * returned when the value is in range. If invert is true, false is returned
 * when the value is in range.
 */
Compare::Compare(
    utils::Int value,
    Relation relation
) :
    value(value),
    relation(relation)
{}

/**
 * Evaluates the condition, given the pass return code.
 */
bool Compare::evaluate(utils::Int pass_return_value) const {
    switch (relation) {
        case Relation::EQ: return pass_return_value == value;
        case Relation::NE: return pass_return_value != value;
        case Relation::GT: return pass_return_value <  value;
        case Relation::GE: return pass_return_value <= value;
        case Relation::LT: return pass_return_value >  value;
        case Relation::LE: return pass_return_value >= value;
        default: return false;
    }
}

/**
 * Returns a string representation of the condition for debugging.
 */
utils::Str Compare::to_string() const {
    switch (relation) {
        case Relation::EQ: return "value == " + utils::to_string(value);
        case Relation::NE: return "value != " + utils::to_string(value);
        case Relation::GT: return "value > "  + utils::to_string(value);
        case Relation::GE: return "value >= " + utils::to_string(value);
        case Relation::LT: return "value < "  + utils::to_string(value);
        case Relation::LE: return "value <= " + utils::to_string(value);
        default: return "UNKNOWN";
    }
}

/**
 * Constructs a condition that bases its result upon whether the pass return
 * code is within a certain range. min and max represent the inclusive
 * range. If invert is false or unspecified, true is returned when the
 * value is in range. If invert is true, false is returned when the value is
 * in range.
 */
Range::Range(
    utils::Int min,
    utils::Int max,
    utils::Bool invert
) :
    min(min),
    max(max),
    invert(invert)
{}

/**
 * Evaluates the condition, given the pass return code.
 */
bool Range::evaluate(utils::Int pass_return_value) const {
    if (pass_return_value >= min && pass_return_value <= max) {
        return !invert;
    } else {
        return invert;
    }
}

/**
 * Returns a string representation of the condition for debugging.
 */
utils::Str Range::to_string() const {
    utils::StrStrm ss{};
    if (invert) {
        ss << "false when ";
    } else {
        ss << "true when ";
    }
    ss << min << " <= value <= " << max;
    return ss.str();
}

} // namespace condition
} // namespace pmgr
} // namespace ql
