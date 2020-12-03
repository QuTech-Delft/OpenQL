/** \file
 * Provides aliases for the base classes of tree-gen's support library, which
 * wrap shared_ptr, vectors of shared_ptrs, and weak_ptrs in a safe way.
 */

#pragma once

#include "tree-base.hpp"

namespace ql {
namespace utils {

/**
 * Base type for tree nodes.
 */
using Node = tree::base::Base;

/**
 * Wrapper for an optional, possibly polymorphic object with shared ownership.
 *
 * In a nutshell, this behaves like a pointer. However, memory management is
 * handled for you through RTTI and reference counting (so use-after-free and
 * the likes should never happen), and all dereference operations are
 * null-checked. Basically, it's a lot safer and more convenient.
 *
 * To create a new object, use make<Type>(...) instead of new Type(...).
 */
template <class T>
using Maybe = tree::base::Maybe<T>;

/**
 * Wrapper for a mandatory, possibly polymorphic object with shared ownership.
 *
 * Same as Maybe, but not "supposed" to be null: when you receive one of these,
 * you may assume it isn't null, and let the internal null-check throw an
 * exception when that assertion fails. So in a way it acts more like a
 * reference than a pointer.
 */
template <class T>
using One = tree::base::One<T>;

/**
 * Wrapper for zero or more (a vector) of possibly polymorphic objects with
 * shared ownership.
 *
 * This is basically just a shorthand for std::vector<One>.
 */
template <class T>
using Any = tree::base::Any<T>;

/**
 * Wrapper for one or more (a vector) of possibly polymorphic objects with
 * shared ownership.
 *
 * Same as Any, but not "supposed" to be empty: when you receive one of these,
 * you may assume it isn't empty, and let the internal range checks throw an
 * exception when that assertion fails.
 */
template <class T>
using Many = tree::base::Many<T>;

/**
 * Like Maybe, but doesn't own the referenced object.
 *
 * That is, if all Maybe/One owners go away, this becomes a "dangling pointer".
 * Nevertheless, this is checked when the link is dereferenced. Links are useful
 * to break cycles in recursive structures that would prevent the structure from
 * being cleaned up.
 *
 * In the context of a tree structure, the tree is only considered complete when
 * the linked object is also reachable in the tree through a Maybe or One node.
 */
template <class T>
using OptLink = tree::base::OptLink<T>;

/**
 * Like One, but doesn't own the referenced object.
 *
 * That is, if all Maybe/One owners go away, this becomes a "dangling pointer".
 * Nevertheless, this is checked when the link is dereferenced. Links are useful
 * to break cycles in recursive structures that would prevent the structure from
 * being cleaned up.
 *
 * In the context of a tree structure, the tree is only considered complete when
 * the linked object is also reachable in the tree through a Maybe or One node.
 */
template <class T>
using Link = tree::base::Link<T>;

/**
 * Constructs a One or Maybe object, analogous to std::make_shared.
 */
template <class T, typename... Args>
One<T> make(Args&&... args) {
    return One<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

} // namespace utils
} // namespace ql
