/** \file
 * Provides a wrapper for std::unordered_map that's safer to use and provides
 * more context when something goes wrong at runtime.
 */

#pragma once

#include <unordered_map>
#include "utils/strings.h"
#include "utils/exception.h"

namespace ql {
namespace utils {

/**
 * Wrapper for `std::unordered_map` with additional error detection and
 * handling.
 *
 * operator[] is intentionally deleted altogether. Instead, you have to pick one
 * of a few different element accessors based on what you actually want to do:
 *
 *  - If you want to insert a key or modify an existing key, use `map.set(key)`
 *    in place of `map[key]`, usually in front of an assignment statement. This
 *    is equivalent to what `map[key]` normally does. There is no `const`
 *    version of this method.
 *  - If you want to access an existing key, use `map.at(key)` in place of
 *    `map[key]`. This will throw an Exception with context information if
 *    the key does not exist yet. This is equivalent to what `map[at]` normally
 *    does. There is both a `const` and non-`const` version, the latter giving
 *    you a mutable reference to the value, the former being immutable.
 *  - If you want to read a key if it exists but get some default value instead
 *    if it doesn't, for instance when your value type is another container and
 *    empty containers may or may not actually be in the map, use `map.get(key)`
 *    or `map.get(key, default)` in place of `map[key]`. Unlike the usual
 *    `map[key]`, the default-constructed value for `map.get(key)` is *not*
 *    inserted into the map, making it `const`.
 *  - If you want to print the value of a key for debugging purposes, use
 *    `map.dbg(key)`. This will return a string representation of the value, or
 *    gracefully return "<EMPTY>" if there is no value in the map for the given
 *    key. The map is not modified, making it `const`.
 *
 * Key-value pairs are stored by means of a hash of the key type. That means
 * element lookup and insertion are (usually, depending on the quality of the
 * hash) constant complexity and therefore might be faster than Map. However,
 * iteration order is undefined and possibly even nondeterministic. If there is
 * an intrinsic and simple ordering for the key type and you need to iterate in
 * this order, use Map instead.
 */
template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class UMap : public std::unordered_map<Key, T, Hash, KeyEqual> {
public:

    /**
     * Typedef for ourselves, allowing us to avoid repeating the template args
     * everywhere.
     */
    using Self = UMap<Key, T, Hash, KeyEqual>;

    /**
     * Typedef for the wrapped STL container.
     */
    using Stl = std::unordered_map<Key, T, Hash, KeyEqual>;

    /**
     * Constructor arguments are forwarded to the STL container constructor, so
     * all constructors of the STL container can be used.
     */
    template <class... Args>
    explicit UMap(Args... args) : Stl(std::forward<Args>(args)...) {
    }

    /**
     * Implicit conversion for initializer lists.
     */
    UMap(std::initializer_list<T> init) : Stl(init) {
    }

    /**
     * Default copy constructor.
     */
    UMap(const Self &map) = default;

    /**
     * Default move constructor.
     */
    UMap(Self &&map) noexcept = default;

    /**
     * Default copy assignment.
     */
    Self &operator=(const Self &other) = default;

    /**
     * Default move assignment.
     */
    Self &operator=(Self &&other) noexcept = default;

    /**
     * Returns mutable access to the value stored for the given key. If the key
     * does not exist, an Exception is thrown.
     */
    T &at(const Key &key) {
        auto it = this->find(key);
        if (it != this->end()) {
            return it->second;
        } else {
            throw exception("key " + try_to_string(key) + " does not exist in map");
        }
    }

    /**
     * Returns const access to the value stored for the given key. If the key
     * does not exist, an Exception is thrown.
     */
    const T &at(const Key &key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            return it->second;
        } else {
            throw exception("key " + try_to_string(key) + " does not exist in map");
        }
    }

    /**
     * Use this to set values in the map, rather than operator[].
     *
     * Specifically, instead of
     *
     *     map[key] = value;
     *
     * use
     *
     *     map.set(key) = value;
     *
     * Just calling set(key) without an assignment statement inserts a
     * default-constructed value for the given key.
     */
    T &set(const Key &key) {
        return Stl::operator[](key);
    }

    /**
     * Returns a const reference to the value at the given key, or to a dummy
     * default-constructed value if the key does not exist.
     */
    const T &get(const Key &key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            return it->second;
        } else {
            static const T DEFAULT{};
            return DEFAULT;
        }
    }

    /**
     * Returns a const reference to the value at the given key, or to the given
     * default value if the key does not exist.
     */
    const T &get(const Key &key, const T &dflt) const {
        auto it = this->find(key);
        if (it != this->end()) {
            return it->second;
        } else {
            return dflt;
        }
    }

    /**
     * Returns a string representation of the value at the given key, or
     * `"<EMPTY>"` if there is no value for the given key. A stream << overload
     * must exist for the value type.
     */
    std::string dbg(const Key &key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            return utils::to_string(it->second);
        } else {
            return "<EMPTY>";
        }
    }

    /**
     * Returns a string representation of the entire contents of the map. Stream
     * << overloads must exist for both the key and value type.
     */
    std::string to_string(
        const std::string &prefix = "{",
        const std::string &key_value_separator = ": ",
        const std::string &element_separator = ", ",
        const std::string &suffix = "}"
    ) const {
        std::ostringstream ss{};
        ss << prefix;
        bool first = true;
        for (const auto &kv : *this) {
            if (first) {
                first = false;
            } else {
                ss << element_separator;
            }
            ss << kv.first << key_value_separator << kv.second;
        }
        ss << suffix;
        return ss.str();
    }

    /**
     * operator[] is unsafe in maps: it can insert keys when you don't expect it
     * to. Therefore it is disabled.
     */
    T &operator[](const Key &key) = delete;

};

} // namespace utils
} // namespace ql

/**
 * Stream << overload for UMap<>.
 */
template <class Key, class T, class Compare>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::UMap<Key, T, Compare> &map) {
    os << map.to_string();
    return os;
}
