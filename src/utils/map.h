/** \file
 * Provides a wrapper for std::map that's safer to use and provides more context
 * when something goes wrong at runtime.
 */

#pragma once

#include <map>
#include "utils/strings.h"
#include "utils/exception.h"

namespace ql {
namespace utils {

/**
 * Wrapper for `std::map` with additional error detection and handling.
 *
 * operator[] is intentionally deleted altogether. Instead, you have to pick one
 * of a few different element accessors based on what you actually want to do:
 *
 *  - If you want to insert a key or modify an existing key, use `map.set(key)`
 *    in place of `map[key]`, usually in front of an assignment statement. This
 *    is equivalent to what `map[key]` normally does. There is no `const`
 *    version of this method.
 *  - If you want to access an existing key, use `map.at(key)` in place of
 *    `map[key]`. This will throw a ql::exception with context information if
 *    the key does not exist yet. This is equivalent to what `map[at]` normally
 *    does. There is both a `const` and non-`const` version, the latter giving
 *    you a mutable reference to the value, the former being immutable.
 *  - If you want to read a key if it exists but get access to a
 *    default-constructed value if it doesn't, for instance when your value type
 *    is another container and empty containers may or may not actually be in
 *    the map, use `map.get(key)` in place of `map[key]`. Unlike the usual
 *    `map[key]`, the default-constructed value is *not* inserted into the map
 *    for the given key, making it `const`.
 *  - If you want to print the value of a key for debugging purposes, use
 *    `map.dbg(key)`. This will return a string representation of the value, or
 *    gracefully return "<EMPTY>" if there is no value in the map for the given
 *    key. The map is not modified, making it `const`.
 *
 * This is an ordered map. That means that a strict ordering must be defined for
 * the key type; the key-value pairs are stored by this ordering in a form of
 * tree, allowing logarithmic complexity insertion and access. Iteration order
 * is deterministic. If you don't insert often or there is no logical ordering
 * for the keys, consider using UMap instead.
 */
template <class Key, class T, class Compare = std::less<Key>>
class Map : public std::map<Key, T, Compare> {
public:

    /**
     * Typedef for ourselves, allowing us to avoid repeating the template args
     * everywhere.
     */
    using Self = Map<Key, T, Compare>;

    /**
     * Typedef for the wrapped STL container.
     */
    using Stl = std::map<Key, T, Compare>;

    /**
     * Constructor arguments are forwarded to the STL container constructor, so
     * all constructors of the STL container can be used.
     */
    template <class... Args>
    explicit Map(Args... args) : Stl(std::forward<Args>(args)...) {
    }

    /**
     * Default copy constructor.
     */
    Map(const Self &map) = default;

    /**
     * Default move constructor.
     */
    Map(Self &&map) noexcept = default;

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
     * does not exist, ql::exception is thrown.
     */
    T &at(const Key &key) {
        auto it = this->find(key);
        if (it != this->end()) {
            return it->second;
        } else {
            throw Exception("key " + try_to_string(key) + " does not exist in map");
        }
    }

    /**
     * Returns const access to the value stored for the given key. If the key
     * does not exist, ql::exception is thrown.
     */
    const T &at(const Key &key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            return it->second;
        } else {
            throw Exception("key " + try_to_string(key) + " does not exist in map");
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
     * Returns a string representation of the value at the given key, or
     * `"<EMPTY>"` if there is no value for the given key.
     */
    std::string dbg(const Key &key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            return try_to_string(it->second);
        } else {
            return "<EMPTY>";
        }
    }

    /**
     * operator[] is unsafe in maps: it can insert keys when you don't expect it
     * to. Therefore it is disabled.
     */
    T &operator[](const Key &key) = delete;

};

} // namespace utils
} // namespace ql
