/** \file
 * Provides a wrapper for std::vector that's safer to use and provides more
 * context when something goes wrong at runtime.
 */

#pragma once

#include <vector>
#include "utils/strings.h"
#include "utils/exception.h"

namespace ql {
namespace utils {

/**
 * Wrapper for `std::vector` with additional error detection and handling.
 *
 * Unlike the STL variant, operator[] is range-checked; it basically functions
 * like at(). If ever you really want the unchecked version for performance
 * reasons (you really shouldn't, though) you can use unchecked_at().
 */
template <class T>
class Vec : public std::vector<T> {
public:

    /**
     * Typedef for ourselves, allowing us to avoid repeating the template args
     * everywhere.
     */
    using Self = Vec<T>;

    /**
     * Typedef for the wrapped STL container.
     */
    using Stl = std::vector<T>;

    /**
     * Constructor arguments are forwarded to the STL container constructor, so
     * all constructors of the STL container can be used.
     */
    template <class... Args>
    explicit Vec(Args... args) : Stl(std::forward<Args>(args)...) {
    }

    /**
     * Default copy constructor.
     */
    Vec(const Self &map) = default;

    /**
     * Default move constructor.
     */
    Vec(Self &&map) noexcept = default;

    /**
     * Default copy assignment.
     */
    Self &operator=(const Self &other) = default;

    /**
     * Default move assignment.
     */
    Self &operator=(Self &&other) noexcept = default;

    /**
     * Returns mutable access to the value stored at the given index. If the
     * index is out of range, an Exception is thrown.
     */
    T &at(size_t index) {
        if (index >= this->size()) {
            throw Exception("index " + try_to_string(index) + " is out of range");
        }
        return Stl::operator[](index);
    }

    /**
     * Returns const access to the value stored for the given key. If the
     * index is out of range, an Exception is thrown.
     */
    const T &at(size_t index) const {
        if (index >= this->size()) {
            throw Exception("index " + try_to_string(index) + " is out of range");
        }
        return Stl::operator[](index);
    }

    /**
     * operator[] redirects to at().
     */
    T &operator[](size_t index) {
        return this->at(index);
    }

    /**
     * operator[] redirects to at().
     */
    const T &operator[](size_t index) const {
        return this->at(index);
    }

    /**
     * Returns UNCHECKED mutable access to the value stored at the given index.
     * Unless you've exhausted all other possibilities for optimization and
     * things still run unacceptably slow, and you find out that at() is somehow
     * the culprit, you really should be using at().
     */
    T &unchecked_at(size_t index) {
        return Stl::operator[](index);
    }

    /**
     * Returns UNCHECKED const access to the value stored at the given index.
     * Unless you've exhausted all other possibilities for optimization and
     * things still run unacceptably slow, and you find out that at() is somehow
     * the culprit, you really should be using at().
     */
    const T &unchecked_at(size_t index) const {
        return Stl::operator[](index);
    }

    /**
     * Returns a const reference to the value at the given index, or to a dummy
     * default-constructed value if the index is out of range.
     */
    const T &get(size_t index) const {
        if (index >= this->size()) {
            static const T DEFAULT{};
            return DEFAULT;
        }
        return Stl::operator[](index);
    }

    /**
     * Returns a string representation of the value at the given index, or
     * `"<OUT-OF-RANGE>"` if the index is out of range. A stream << overload
     * must exist for the value type.
     */
    std::string dbg(size_t index) const {
        if (index >= this->size()) {
            return "<OUT-OF-RANGE>";
        }
        return utils::to_string(Stl::operator[](index));
    }

    /**
     * Returns a string representation of the entire contents of the vector.
     * A stream << overload must exist for the value type.
     */
    std::string to_string(
        const std::string &prefix = "[",
        const std::string &separator = ", ",
        const std::string &suffix = "]"
    ) const {
        std::ostringstream ss{};
        ss << prefix;
        bool first = true;
        for (const auto &val : *this) {
            if (first) {
                first = false;
            } else {
                ss << separator;
            }
            ss << val;
        }
        ss << suffix;
        return ss.str();
    }

};

} // namespace utils
} // namespace ql

/**
 * Stream << overload for Vec<>.
 */
template <class T>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::Vec<T> &vec) {
    os << vec.to_string();
    return os;
}
