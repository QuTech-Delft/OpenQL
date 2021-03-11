/** \file
 * Provides a class for representing shared pointers/references to possibly
 * polymorphic objects. So, like std::shared_ptr (but with added safety) or
 * tree::One/Maybe (but without requiring tree methods on the object type).
 */

#pragma once

#include <memory>
#include "utils/exception.h"

namespace ql {
namespace utils {

/**
 * Represents a shared pointers/references to possibly polymorphic objects. So,
 * like std::shared_ptr (but with added safety) or tree::One/Maybe (but without
 * requiring tree methods on the object type).
 */
template <class T>
class Ptr {
private:

    /**
     * The contained value, wrapped in a unique_ptr.
     */
    std::shared_ptr<T> v{};

public:

    /**
     * Initialization method, used to fill an empty container after
     * construction. The container is expected to be empty initially.
     */
    template<typename S = T, class... Args>
    void emplace(Args&&... args) {
        if (v) {
            throw Exception("Ptr has already been initialized", false);
        }
        v = std::unique_ptr<T>(new S(std::forward<Args>(args)...));
    }

    /**
     * Drops the contained object, if any.
     */
    void reset() {
        v.reset();
    }

    /**
     * Constructor for an empty container.
     */
    Ptr() = default;

    /**
     * Constructor for a filled container.
     *
     * This only works when there is at least one argument for the constructor
     * of the contained object, and the object is not polymorphic. You'll have
     * to use emplace() otherwise.
     */
    template<class Arg1, class... Args>
    Ptr(Arg1 &&arg1, Args&&... args) {
        emplace(std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    /**
     * Copy constructor. Only the pointer is moved, so the two Ptrs will
     * refer to the same object.
     */
    Ptr(const Ptr &o) : v(o.v) {
    }

    /**
     * Move constructor. Just moves the pointer to the contained object.
     */
    Ptr(Ptr &&o) {
        v.swap(o.v);
    }

    /**
     * Copy or move assignment from a contained object. Defers to the copy or
     * move constructor of the contained object type.
     */
    template<typename S = T, typename = typename std::enable_if<std::is_base_of<T, S>::value>::type>
    Ptr &operator=(S &&rhs) {
        v = std::shared_ptr<T>(new T(std::forward<S>(rhs)));
        return *this;
    }

    /**
     * Copy assignment. Copies the pointer, so both Ptrs will point to the same
     * object.
     */
    template<typename S = T, typename = typename std::enable_if<std::is_base_of<T, S>::value>::type>
    Ptr &operator=(const Ptr<S> &rhs) {
        v = rhs.unwrap();
        return *this;
    }

    /**
     * Copy assignment. Copies the pointer, so both Ptrs will point to the same
     * object.
     */
    Ptr &operator=(const Ptr &rhs) {
        v = rhs.v;
        return *this;
    }

    /**
     * Move assignment.
     */
    Ptr &operator=(Ptr &&rhs) noexcept {
        v.swap(rhs.v);
        return *this;
    }

    /**
     * Returns whether this container is filled.
     */
    bool has_value() const noexcept {
        return (bool)v;
    }

    /**
     * Returns whether this container is filled.
     */
    explicit operator bool() const noexcept {
        return (bool)v;
    }

    /**
     * Returns the raw shared_ptr.
     */
    const std::shared_ptr<T> &unwrap() const {
        return v;
    }

    /**
     * Returns the raw shared_ptr.
     */
    std::shared_ptr<T> &unwrap() {
        return v;
    }

    /**
     * Returns whether this Ptr points to a value of the given type.
     */
    template<typename S>
    bool is() const noexcept {
        if (v == nullptr) return false;
        return std::dynamic_pointer_cast<S>(v) != nullptr;
    }

    /**
     * Casts to a Ptr of the given type. The resulting Ptr will be empty if the
     * cast failed.
     */
    template<typename S>
    Ptr<S> try_as() const noexcept {
        Ptr<S> result;
        result.unwrap() = std::dynamic_pointer_cast<S>(v);
        return result;
    }

    /**
     * Casts to a Ptr of the given type. Throws an exception if the cast failed
     * or this Ptr is empty.
     */
    template<typename S>
    Ptr<S> as() const noexcept {
        if (!v) {
            throw Exception("attempt to cast empty Ptr");
        }
        auto result = try_as<S>();
        if (!result) {
            throw Exception("attempt to cast Ptr to unsupported type");
        }
        return result;
    }

    /**
     * Casts to a const Ptr.
     */
    Ptr<const T> as_const() const noexcept {
        auto result = Ptr<const T>();
        result.unwrap() = std::const_pointer_cast<const T>(v);
        return result;
    }

    /**
     * Mutating dereference operator.
     */
    T &operator*() {
        if (!v) {
            throw Exception("attempt to dereference empty Ptr");
        }
        return *v;
    }

    /**
     * Const dereference operator.
     */
    const T &operator*() const {
        if (!v) {
            throw Exception("attempt to dereference empty Ptr");
        }
        return *v;
    }

    /**
     * Mutating dereference operator.
     */
    T *operator->() {
        if (!v) {
            throw Exception("attempt to dereference empty Ptr");
        }
        return v.get();
    }

    /**
     * Const dereference operator.
     */
    const T *operator->() const {
        if (!v) {
            throw Exception("attempt to dereference empty Ptr");
        }
        return v.get();
    }

    /**
     * Stream overload for pointers.
     */
    friend std::ostream &operator<<(std::ostream &os, const Ptr &ptr) {
        if (ptr.v) {
            os << *ptr.v;
        } else {
            os << "<NULL>";
        }
        return os;
    }

};

} // namespace utils
} // namespace ql
