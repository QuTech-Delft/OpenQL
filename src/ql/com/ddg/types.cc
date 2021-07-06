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
 * String conversion for Reference.
 */
std::ostream &operator<<(std::ostream &os, const Reference &reference) {
    if (reference.is_global_state()) {
        return os << "<global>";
    }
    if (reference.data_type != reference.target->data_type) {
        os << reference.data_type->name << "(" << reference.target->name << ")";
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
    return os;
}

/**
 * Returns whether two references refer to statically provable distinct objects.
 */
utils::Bool is_provably_distinct(const Reference &a, const Reference &b) {

    // If either reference is null, an all-encompassing global state is implied.
    // This is never provably distinct with anything else.
    if (a.is_global_state() || b.is_global_state()) {
        return false;
    }

    // If the target objects are non-null and distinct, the referred objects are
    // obviously distinct.
    if (a.target != b.target) {
        return true;
    }

    // Same for the data type, which is currently only used to refer to the
    // implicit measurement bit of a qubit, which is thus distinct. If this
    // ends up being used for typecasts, this will become more complicated.
    if (a.data_type != b.data_type) {
        return true;
    }

    // Okay, both are referring to the same object. But if the object is
    // non-scalar, they may still be referring to provably different elements of
    // that object. You can do all sorts of fancy aliasing stuff here, but for
    // now we'll only worry about static indices for as far as they are known.
    // If they differ, the targets are distinct.
    utils::UInt known_dims = utils::max(a.indices.size(), b.indices.size());
    for (utils::UInt dim = 0; dim < known_dims; dim++) {
        if (a.indices[dim] != b.indices[dim]) {
            return true;
        }
    }

    // We can't prove that the two references aren't aliases.
    return false;
}

/**
 * Combines two references into the most specific reference that encompasses
 * both a and b.
 */
Reference combine_references(const Reference &a, const Reference &b) {

    // If we're dealing with two different objects, or either reference is
    // already generalized to the global state, the global state is the most
    // specific thing we can represent with a single reference.
    if (
        a.is_global_state() ||
        b.is_global_state() ||
        a.target != b.target ||
        a.data_type != b.data_type
    ) {
        return {};
    }

    // The objects referred to are the same, so look at the indices instead.
    Reference result;
    result.target = a.target;
    result.data_type = a.data_type;
    utils::UInt known_dims = utils::max(a.indices.size(), b.indices.size());
    for (utils::UInt dim = 0; dim < known_dims; dim++) {
        if (a.indices[dim] != b.indices[dim]) break;
        result.indices.push_back(a.indices[dim]);
    }
    return result;

}

/**
 * String conversion for AccessMode. Returns its word form.
 */
std::ostream &operator<<(std::ostream &os, AccessMode mode) {
    switch (mode) {
        case AccessMode::WRITE: return os << "write";
        case AccessMode::READ: return os << "read";
        case AccessMode::COMMUTE_X: return os << "commute-x";
        case AccessMode::COMMUTE_Y: return os << "commute-y";
        case AccessMode::COMMUTE_Z: return os << "commute-z";
    }
    return os << "<UNKNOWN>";
}

/**
 * Represents the given access mode as a single character.
 */
utils::Char get_access_mode_letter(AccessMode mode) {
    switch (mode) {
        case AccessMode::WRITE: return 'W';
        case AccessMode::READ: return 'R';
        case AccessMode::COMMUTE_X: return 'X';
        case AccessMode::COMMUTE_Y: return 'Y';
        case AccessMode::COMMUTE_Z: return 'Z';
    }
    return '?';
}

/**
 * Returns whether the given two access modes commute.
 */
utils::Bool do_modes_commute(AccessMode a, AccessMode b) {
    if (a == AccessMode::WRITE) return false;
    if (b == AccessMode::WRITE) return false;
    if (a != b) return false;
    return true;
}

/**
 * Combines two modes into one, for use when a single object is accessed in
 * multiple ways. If the modes commute, either a or b is returned. If they
 * don't, WRITE is returned.
 */
AccessMode combine_modes(AccessMode a, AccessMode b) {
    if (do_modes_commute(a, b)) {
        return utils::min(a, b);
    } else {
        return AccessMode::WRITE;
    }
}

/**
 * String conversion for Event.
 */
std::ostream &operator<<(std::ostream &os, const Event &event) {
    return os << get_access_mode_letter(event.mode) << ":" << event.reference;
}

/**
 * Returns whether the given two events commute. This is true if the references
 * belonging to the events are statically known to refer to different objects,
 * or if the access modes don't commute.
 */
utils::Bool do_events_commute(const Event &a, const Event &b) {
    if (do_modes_commute(a.mode, b.mode)) return true;
    if (is_provably_distinct(a.reference, b.reference)) return true;
    return false;
}

/**
 * String conversion for DependencyType.
 */
std::ostream &operator<<(std::ostream &os, const DependencyType &dependency_type) {
    os << get_access_mode_letter(dependency_type.second_mode);
    os << "A";
    os << get_access_mode_letter(dependency_type.first_mode);
    return os;
}


} // namespace ddg
} // namespace com
} // namespace ql
