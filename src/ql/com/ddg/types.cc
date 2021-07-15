/** \file
 * Defines types for representing the data dependency graph.
 */

#include "ql/com/ddg/types.h"

#include "ql/ir/ops.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Converts an IR reference to a static reference object.
 */
Reference::Reference(const utils::One<ir::Reference> &ref) {
    if (!ref.empty()) {
        target = ref->target;
        data_type = ref->data_type;
        for (const auto &index : ref->indices) {
            if (auto ilit = index->as_int_literal()) {
                indices.push_back(ilit->value);
            } else {
                break;
            }
        }
    }
}

/**
 * Converts a static reference back to a normal reference.
 */
utils::One<ir::Reference> Reference::make_reference(const ir::Ref &ir) const {
    auto ref = utils::make<ir::Reference>(target, data_type);
    for (const auto &index : indices) {
        ref->indices.add(ir::make_int_lit(ir, index));
    }
    return ref;
}


/**
 * String conversion for Reference.
 */
std::ostream &operator<<(std::ostream &os, const Reference &reference) {
    if (reference.is_global_state()) {
        return os << "<global>";
    }
    if (reference.data_type != reference.target->data_type) {
        os << reference.data_type->name << "(";
    }
    if (reference.target->name.empty()) {
        os << "<anonymous>";
    } else {
        os << reference.target->name;
    }
    if (!reference.target->shape.empty()) {
        os << "[";
        for (utils::UInt dim = 0; dim < reference.target->shape.size(); dim++) {
            if (dim) os << ", ";
            if (dim < reference.indices.size()) {
                os << reference.indices[dim];
            } else {
                os << "*";
            }
        }
        os << "]";
    }
    if (reference.data_type != reference.target->data_type) {
        os << ")";
    }
    return os;
}

/**
 * Less-than operator to allow this to be used as a key to a map.
 */
utils::Bool Reference::operator<(const Reference &rhs) const {
    if (target > rhs.target) return false;
    if (target < rhs.target) return true;
    if (data_type > rhs.data_type) return false;
    if (data_type < rhs.data_type) return true;
    utils::UInt i = 0;
    while (true) {
        if (i >= rhs.indices.size()) return false;
        if (i >= indices.size()) return true;
        if (indices[i] > rhs.indices[i]) return false;
        if (indices[i] < rhs.indices[i]) return true;
        i++;
    }
}

/**
 * Value-based equality operator.
 */
utils::Bool Reference::operator==(const Reference &rhs) const {
    return (
        target == rhs.target &&
        data_type == rhs.data_type &&
        indices == rhs.indices
    );
}

/**
 * Returns whether this is a null reference, i.e. it refers to unknown or
 * global state of the system.
 */
utils::Bool Reference::is_global_state() const {
    return target.empty();
}

/**
 * Returns whether two references refer to statically provable distinct objects.
 */
utils::Bool Reference::is_provably_distinct_from(const Reference &reference) const {

    // If either reference is null, an all-encompassing global state is implied.
    // This is never provably distinct with anything else.
    if (is_global_state() || reference.is_global_state()) {
        return false;
    }

    // If the target objects are non-null and distinct, the referred objects are
    // obviously distinct.
    if (target != reference.target) {
        return true;
    }

    // Same for the data type, which is currently only used to refer to the
    // implicit measurement bit of a qubit, which is thus distinct. If this
    // ends up being used for typecasts, this will become more complicated.
    if (data_type != reference.data_type) {
        return true;
    }

    // Okay, both are referring to the same object. But if the object is
    // non-scalar, they may still be referring to provably different elements of
    // that object. You can do all sorts of fancy aliasing stuff here, but for
    // now we'll only worry about static indices for as far as they are known.
    // If they differ, the targets are distinct.
    utils::UInt known_dims = utils::max(indices.size(), reference.indices.size());
    for (utils::UInt dim = 0; dim < known_dims; dim++) {
        if (indices[dim] != reference.indices[dim]) {
            return true;
        }
    }

    // We can't prove that the two references aren't aliases.
    return false;
}

/**
 * Returns whether the given reference refers to a superset of the
 * objects/elements that this reference refers to.
 */
utils::Bool Reference::is_shadowed_by(const Reference &reference) const {
    if (reference.is_global_state()) return true;
    if (is_global_state()) return false;
    if (target != reference.target) return false;
    if (data_type != reference.data_type) return false;
    if (reference.indices.size() > indices.size()) return false;
    for (utils::UInt dim = 0; dim < reference.indices.size(); dim++) {
        if (indices[dim] != reference.indices[dim]) {
            return false;
        }
    }
    return true;
}

/**
 * Combines two references into the most specific reference that encompasses
 * both a and b.
 */
Reference Reference::union_with(const Reference &reference) const {

    // If we're dealing with two different objects, or either reference is
    // already generalized to the global state, the global state is the most
    // specific thing we can represent with a single reference.
    if (
        is_global_state() ||
        reference.is_global_state() ||
        target != reference.target ||
        data_type != reference.data_type
    ) {
        return {};
    }

    // The objects referred to are the same, so look at the indices instead.
    // Starting from the major dimension, all dimensions for which the indices
    // match for both references are included. As soon as there's a difference,
    // we stop.
    Reference result;
    result.target = target;
    result.data_type = data_type;
    utils::UInt known_dims = utils::min(indices.size(), reference.indices.size());
    for (utils::UInt dim = 0; dim < known_dims; dim++) {
        if (indices[dim] != reference.indices[dim]) break;
        result.indices.push_back(indices[dim]);
    }
    return result;

}

/**
 * Combines two references into the most specific reference that encompasses
 * the intersection between a and b.
 */
Reference Reference::intersect_with(const Reference &reference) const {

    // If either is the global state, return the other.
    if (is_global_state()) return reference;
    if (reference.is_global_state()) return *this;

    // If we're dealing with two different objects, the global state is the most
    // specific thing we can represent with a single reference.
    if (
        target != reference.target ||
        data_type != reference.data_type
    ) {
        return {};
    }

    // The objects referred to are the same, so look at the indices instead.
    // Starting from the major dimension, all dimensions for which the indices
    // match for both references are included. If that includes all statically
    // known indices for one of the two references, one is a subset of the
    // other, so we can return the more specific one.
    Reference result;
    result.target = target;
    result.data_type = data_type;
    utils::UInt known_dims = utils::min(indices.size(), reference.indices.size());
    for (utils::UInt dim = 0; dim < known_dims; dim++) {
        if (indices[dim] != reference.indices[dim]) return result;
        result.indices.push_back(indices[dim]);
    }
    if (indices.size() > reference.indices.size()) {
        return *this;
    } else {
        return reference;
    }

}

/**
 * Returns the classical write access mode, that doesn't commute with
 * anything else.
 */
AccessMode::AccessMode() : value(Enum::WRITE) {
}

/**
 * Constructs an access mode from a (currently hardcoded) operand mode.
 */
AccessMode::AccessMode(ir::prim::OperandMode operand_mode) {
    switch (operand_mode) {
        case ir::prim::OperandMode::WRITE:     value = Enum::WRITE;     break;
        case ir::prim::OperandMode::READ:      value = Enum::READ;      break;
        case ir::prim::OperandMode::COMMUTE_X: value = Enum::COMMUTE_X; break;
        case ir::prim::OperandMode::COMMUTE_Y: value = Enum::COMMUTE_Y; break;
        case ir::prim::OperandMode::COMMUTE_Z: value = Enum::COMMUTE_Z; break;
        default: QL_ICE("cannot use operand mode " << operand_mode << " in DDG");
    }
}

/**
 * Returns the classical write access mode, that doesn't commute with
 * anything else.
 */
AccessMode AccessMode::write() {
    return {};
}

/**
 * Returns the classical read access mode, that commutes with itself but
 * not with write.
 */
AccessMode AccessMode::read() {
    AccessMode result;
    result.value = Enum::READ;
    return result;
}

/**
 * String conversion for AccessMode. Returns its word form.
 */
std::ostream &operator<<(std::ostream &os, const AccessMode &access_mode) {
    switch (access_mode.value) {
        case AccessMode::Enum::WRITE: return os << "write";
        case AccessMode::Enum::READ: return os << "read";
        case AccessMode::Enum::COMMUTE_X: return os << "commute-x";
        case AccessMode::Enum::COMMUTE_Y: return os << "commute-y";
        case AccessMode::Enum::COMMUTE_Z: return os << "commute-z";
    }
    return os << "<UNKNOWN>";
}

/**
 * Value-based equality operator.
 */
utils::Bool AccessMode::operator==(const AccessMode &access_mode) const {
    return value == access_mode.value;
}

/**
 * Represents the given access mode as a single character, used to represent
 * the dependency relation between two non-commuting modes (RAW, WAW, WAR,
 * etc.).
 */
utils::Char AccessMode::as_letter() const {
    switch (value) {
        case AccessMode::Enum::WRITE: return 'W';
        case AccessMode::Enum::READ: return 'R';
        case AccessMode::Enum::COMMUTE_X: return 'X';
        case AccessMode::Enum::COMMUTE_Y: return 'Y';
        case AccessMode::Enum::COMMUTE_Z: return 'Z';
    }
    return '?';
}

/**
 * Returns whether the given two access modes commute. Must be symmetric.
 */
utils::Bool AccessMode::commutes_with(const AccessMode &access_mode) const {

    // All modes except write commute with themselves.
    if (value == access_mode.value && value != AccessMode::Enum::WRITE) return true;

    // None of the remaining modes commute.
    return false;

}

/**
 * Combines two modes into one, for example used when a single object is
 * accessed in multiple ways but has to be represented with a single access
 * mode. The requirement on combine_modes(a, b) -> c is that any mode d that
 * does not commute with a OR does not commute with mode b also does not commute
 * with mode c, but the more modes the result commutes with, the less
 * pessimistic the DDG will be.
 */
AccessMode AccessMode::combine_with(const AccessMode &b) const {
    if (commutes_with(b)) {
        return *this;
    } else {
        return AccessMode(ir::prim::OperandMode::WRITE);
    }
}

/**
 * Creates an Event object from a pair as stored in the Events map.
 */
Event::Event(
    const utils::Pair<const Reference, AccessMode> &pair
) :
    reference(pair.first),
    mode(pair.second)
{}


/**
 * String conversion for Event.
 */
std::ostream &operator<<(std::ostream &os, const Event &event) {
    return os << event.mode.as_letter() << ":" << event.reference;
}

/**
 * Returns whether the given event commutes with this event. This is true if
 * the references belonging to the events are statically known to refer to
 * different objects, or if the access modes commute.
 */
utils::Bool Event::commutes_with(const Event &event) const {
    if (mode.commutes_with(event.mode)) return true;
    if (reference.is_provably_distinct_from(event.reference)) return true;
    return false;
}

/**
 * Returns whether the given event completely shadows this event. That is,
 * the access modes don't commute, and the specified reference refers to a
 * superset of the objects referred to by this reference.
 */
utils::Bool Event::is_shadowed_by(const Event &event) const {
    if (mode.commutes_with(event.mode)) return false;
    if (!reference.is_shadowed_by(event.reference)) return false;
    return true;
}

/**
 * String conversion for DependencyType.
 */
std::ostream &operator<<(std::ostream &os, const DependencyType &dependency_type) {
    os << dependency_type.second_mode.as_letter();
    os << "A";
    os << dependency_type.first_mode.as_letter();
    return os;
}

/**
 * String conversion for Cause.
 */
std::ostream &operator<<(std::ostream &os, const Cause &cause) {
    return os << cause.dependency_type << ":" << cause.reference;
}

} // namespace ddg
} // namespace com
} // namespace ql
