#pragma once

#include <memory>
#include "utils/exception.h"

namespace ql {
namespace utils {

/**
 * Represents an optional value. You can use this when you need to own some
 * object but can't immediately initialize it, or whenever else you need an
 * optional object. It's sort of like std::optional, but unfortunately that
 * doesn't exist yet in our target version of C++.
 */
template <class T>
class Opt {
private:

    /**
     * The contained value, wrapped in a unique_ptr.
     */
    std::unique_ptr<T> v{};

public:

    /**
     * Initialization method, used to fill an empty container after
     * construction. The container is expected to be empty initially.
     */
    template<class... Args>
    void emplace(Args... args) {
        if (v) {
            throw exception("Opt has already been initialized", false);
        }
        v = std::unique_ptr<T>(new T(args...));
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
    Opt() = default;

    /**
     * Constructor for a filled container.
     *
     * This only works when there is at least one argument for the constructor
     * of the contained object. You'll have to use emplace() otherwise. The
     * trick std::optional uses to avoid this is not implemented here.
     */
    template<class Arg1, class... Args>
    Opt(Arg1 arg1, Args... args) {
        emplace(arg1, args...);
    }

    /**
     * Copy constructor. Defers to the copy constructor of the contained object
     * (if any).
     */
    Opt(const Opt<T> &o) {
        if (o) {
            emplace(*o);
        }
    }

    /**
     * Move constructor. Just moves the pointer to the contained object.
     */
    Opt(Opt<T> &&o) {
        v.swap(o.v);
    }

    /**
     * Copy or move assignment from a contained object. Defers to the copy or
     * move constructor of the contained object type.
     */
    Opt &operator=(T &&rhs) {
        v = std::unique_ptr<T>(new T(std::forward<T>(rhs)));
        return *this;
    }

    /**
     * Copy assignment. Defers to the copy constructor of the contained object
     * type.
     */
    Opt &operator=(const Opt<T> &rhs) {
        reset();
        if (rhs) {
            emplace(*rhs);
        }
        return *this;
    }

    /**
     * Move assignment.
     */
    Opt &operator=(Opt<T> &&rhs) noexcept {
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
     * Mutating dereference operator.
     */
    T &operator*() {
        if (!v) {
            throw exception("attempt to dereference empty Opt");
        }
        return *v;
    }

    /**
     * Const dereference operator.
     */
    const T &operator*() const {
        if (!v) {
            throw exception("attempt to dereference empty Opt");
        }
        return *v;
    }

    /**
     * Mutating dereference operator.
     */
    T *operator->() {
        if (!v) {
            throw exception("attempt to dereference empty Opt");
        }
        return v.get();
    }

    /**
     * Const dereference operator.
     */
    const T *operator->() const {
        if (!v) {
            throw exception("attempt to dereference empty Opt");
        }
        return v.get();
    }

};

} // namespace utils
} // namespace ql
