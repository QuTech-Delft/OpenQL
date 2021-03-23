/** \file
 * Provides a wrapper for std::map that's safer to use and provides more context
 * when something goes wrong at runtime.
 */

#pragma once

#include <map>
#include "ql_config.h"
#include "utils/str.h"
#include "utils/container_base.h"

namespace ql {
namespace utils {

/**
 * Wrapper for `std::map` which replaces operator[] with safer variants, but
 * does no additional error checking.
 *
 * Instead of operator[], you have to pick one of a few different element
 * accessors based on what you actually want to do:
 *
 *  - If you want to insert a key or modify an existing key, use `map.set(key)`
 *    in place of `map[key]`, usually in front of an assignment statement. This
 *    is equivalent to what `map[key]` normally does. There is no `const`
 *    version of this method.
 *  - If you want to access an existing key, use `map.at(key)` in place of
 *    `map[key]`. This will throw a Exception with context information if
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
 */
template <class Key, class T, class Compare, class Allocator>
class UncheckedMap : public std::map<Key, T, Compare, Allocator> {
public:

    /**
     * Typedef for the wrapped STL container.
     */
    using Stl = std::map<Key, T, Compare, Allocator>;

    /**
     * Default constructor. Constructs an empty container with a
     * default-constructed allocator.
     */
    UncheckedMap() : Stl() {}

    /**
     * Constructor arguments are forwarded to the STL container constructor, so
     * all constructors of the STL container can be used.
     */
    template <class... Args>
    explicit UncheckedMap(Args&&... args) : Stl(std::forward<Args>(args)...) {
    }

    /**
     * Implicit conversion for initializer lists.
     */
    UncheckedMap(std::initializer_list<typename Stl::value_type> init) : Stl(init) {
    }

    /**
     * Default copy constructor.
     */
    UncheckedMap(const UncheckedMap &map) = default;

    /**
     * Default move constructor.
     */
    UncheckedMap(UncheckedMap &&map) noexcept = default;

    /**
     * Default copy assignment.
     */
    UncheckedMap &operator=(const UncheckedMap &other) = default;

    /**
     * Default move assignment.
     */
    UncheckedMap &operator=(UncheckedMap &&other) noexcept = default;

    /**
     * Returns mutable access to the value stored for the given key. If the key
     * does not exist, an Exception is thrown.
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
     * does not exist, an Exception is thrown.
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
    Str dbg(const Key &key) const {
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
    Str to_string(
        const Str &prefix = "{",
        const Str &key_value_separator = ": ",
        const Str &element_separator = ", ",
        const Str &suffix = "}"
    ) const {
        StrStrm ss{};
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

/**
 * Stream << overload for UncheckedMap<>.
 */
template <class Key, class T, class Compare>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::UncheckedMap<Key, T, Compare> &map) {
    os << map.to_string();
    return os;
}

/**
 * Wrapper for `std::map` that guards against undefined behavior and replaces
 * operator[] with safer variants.
 *
 * Instead of operator[], you have to pick one of a few different element
 * accessors based on what you actually want to do:
 *
 *  - If you want to insert a key or modify an existing key, use `map.set(key)`
 *    in place of `map[key]`, usually in front of an assignment statement. This
 *    is equivalent to what `map[key]` normally does. There is no `const`
 *    version of this method.
 *  - If you want to access an existing key, use `map.at(key)` in place of
 *    `map[key]`. This will throw a Exception with context information if
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
 */
template <typename Key, typename T, typename Compare, typename Allocator>
class CheckedMap {
public:

    /**
     * Shorthand for the STL component being wrapped.
     */
    using Stl = std::map<Key, T, Compare, Allocator>;

    /**
     * Shorthand for the data block type.
     */
    using Data = ContainerData<Stl>;

    /**
     * Forward iterator with mutable access to the values.
     */
    using Iter = WrappedIterator<Data, typename Stl::iterator, RegularEndpointAdapter>;

    /**
     * Forward iterator with const access to the values.
     */
    using ConstIter = WrappedIterator<const Data, typename Stl::const_iterator, ConstEndpointAdapter>;

    /**
     * Backward iterator with mutable access to the values.
     */
    using ReverseIter = std::reverse_iterator<Iter>;

    /**
     * Backward iterator with const access to the values.
     */
    using ConstReverseIter = std::reverse_iterator<ConstIter>;

    // Member types expected by the standard library.
    using key_type = Key;
    using mapped_type = T;
    using value_type = typename Stl::value_type;
    using key_compare = Compare;
    using allocator_type = Allocator;
    using size_type = typename Stl::size_type;
    using difference_type = typename Stl::difference_type;
    using reference = typename Stl::reference;
    using const_reference = typename Stl::const_reference;
    using pointer = typename Stl::pointer;
    using const_pointer = typename Stl::const_pointer;
    using iterator = Iter;
    using const_iterator = ConstIter;
    using reverse_iterator = ReverseIter;
    using const_reverse_iterator = ConstReverseIter;

private:

    /**
     * Data block for the map.
     */
    std::shared_ptr<Data> data_ptr;

    /**
     * Safely returns mutable access to the data block.
     */
    Data &get_data() {
        if (!data_ptr) {
            throw ContainerException(
                "container is used after move or otherwise has invalid data block"
            );
        }
        return *data_ptr;
    }

    /**
     * Safely returns const access to the data block.
     */
    const Data &get_data() const {
        if (!data_ptr) {
            throw ContainerException(
                "container is used after move or otherwise has invalid data block"
            );
        }
        return *data_ptr;
    }

public:

    /**
     * Default constructor. Constructs an empty container with a
     * default-constructed allocator.
     */
    CheckedMap() : data_ptr(std::make_shared<Data>()) {}

    /**
     * Constructs an empty container with the given comparator comp and
     * allocator alloc.
     */
    explicit CheckedMap(const Compare &comp, const Allocator &alloc = Allocator()) : data_ptr(std::make_shared<Data>(comp, alloc)) {}

    /**
     * Constructs the container with the contents of the range [first, last). If
     * multiple elements in the range have keys that compare equivalent, it is
     * unspecified which element is inserted.
     */
    template <
        typename InputIt,
        typename = typename std::enable_if<std::is_convertible<
            typename std::iterator_traits<InputIt>::iterator_category,
            std::input_iterator_tag
        >::value>::type
    >
    CheckedMap(
        InputIt first,
        InputIt last,
        const Compare &comp = Compare(),
        const Allocator &alloc = Allocator()
    ) : data_ptr(std::make_shared<Data>(first, last, comp, alloc)) {}

    /**
     * Copy constructor. Constructs the container with the copy of the contents
     * of other.
     */
    CheckedMap(const CheckedMap &other) : data_ptr(std::make_shared<Data>(other.get_data().get_const())) {}

    /**
     * Copy constructor. Constructs the container with the copy of the contents
     * of other.
     */
    CheckedMap(const Stl &other) : data_ptr(std::make_shared<Data>(other)) {}

    /**
     * Constructs the container with the copy of the contents of other, using
     * alloc as the allocator.
     */
    CheckedMap(const Stl &other, const Allocator &alloc) : data_ptr(std::make_shared<Data>(other, alloc)) {}

    /**
     * Move constructor. The data block pointer is moved, so iterators remain
     * valid.
     */
    CheckedMap(CheckedMap &&other) noexcept = default;

    /**
     * Move constructor. Constructs the container with the contents of other
     * using move semantics. Allocator is obtained by move-construction from
     * the allocator belonging to other. After the move, other is guaranteed to
     * be empty().
     */
    CheckedMap(Stl &&other) : data_ptr(std::make_shared<Data>(std::forward<Stl>(other))) {}

    /**
     * Allocator-extended move constructor. Using alloc as the allocator for the
     * new container, moving the contents from other; if
     * alloc != other.get_allocator(), this results in an element-wise move.
     * (in that case, other is not guaranteed to be empty after the move)
     */
    CheckedMap(Stl &&other, const Allocator &alloc) : data_ptr(std::make_shared<Data>(std::forward<Stl>(other), alloc)) {}

    /**
     * Constructs the container with the contents of the initializer list init.
     */
    CheckedMap(
        std::initializer_list<value_type> init,
        const Compare &comp = Compare(),
        const Allocator &alloc = Allocator()
    ) : data_ptr(std::make_shared<Data>(init, comp, alloc)) {};

    /**
     * Copy assignment from an STL map.
     */
    CheckedMap &operator=(const Stl &other) {
        get_data().get_mut().operator=(other);
        return *this;
    }

    /**
     * Move assignment from an STL map.
     */
    CheckedMap &operator=(Stl &&other) {
        get_data().get_mut().operator=(std::move(other));
        return *this;
    }

    /**
     * Copy assignment.
     */
    CheckedMap &operator=(const CheckedMap &rhs) {
        get_data().get_mut().operator=(rhs.get_data().get_const());
        return *this;
    }

    /**
     * Move assignment. The data block pointer is moved, so iterators remain
     * valid.
     */
    CheckedMap &operator=(CheckedMap &&rhs) noexcept = default;

    /**
     * Replaces the contents with those identified by initializer list ilist.
     */
    CheckedMap &operator=(std::initializer_list<value_type> ilist) {
        get_data().get_mut().operator=(ilist);
        return *this;
    }

    /**
     * Returns the allocator associated with the container.
     */
    allocator_type get_allocator() const {
        return get_data().get_const().get_allocator();
    }

    /**
     * Returns mutable access to the value stored for the given key. If the key
     * does not exist, an Exception is thrown.
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
     * does not exist, an Exception is thrown.
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
        return get_data().get_mut_element_only().operator[](key);
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
    Str dbg(const Key &key) const {
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
    Str to_string(
        const Str &prefix = "{",
        const Str &key_value_separator = ": ",
        const Str &element_separator = ", ",
        const Str &suffix = "}"
    ) const {
        StrStrm ss{};
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
     * Returns an iterator to the first element of the map. If the map is empty,
     * the returned iterator will be equal to end().
     */
    Iter begin() {
        return Iter(get_data().get_mut_element_only().begin(), data_ptr);
    }

    /**
     * Returns an iterator to the first element of the map. If the map is empty,
     * the returned iterator will be equal to end().
     */
    ConstIter begin() const {
        return ConstIter(get_data().get_const().cbegin(), data_ptr);
    }

    /**
     * Returns an iterator to the first element of the map. If the map is empty,
     * the returned iterator will be equal to end().
     */
    ConstIter cbegin() const {
        return ConstIter(get_data().get_const().cbegin(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the map.
     * This element acts as a placeholder; attempting to access it results in an
     * exception.
     */
    Iter end() {
        return Iter(get_data().get_mut_element_only().end(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the map.
     * This element acts as a placeholder; attempting to access it results in an
     * exception.
     */
    ConstIter end() const {
        return ConstIter(get_data().get_const().cend(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the map.
     * This element acts as a placeholder; attempting to access it results in an
     * exception.
     */
    ConstIter cend() const {
        return ConstIter(get_data().get_const().cend(), data_ptr);
    }

    /**
     * Returns a reverse iterator to the first element of the reversed map. It
     * corresponds to the last element of the non-reversed map. If the map is
     * empty, the returned iterator is equal to rend().
     */
    ReverseIter rbegin() {
        return ReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the first element of the reversed map. It
     * corresponds to the last element of the non-reversed map. If the map is
     * empty, the returned iterator is equal to rend().
     */
    ConstReverseIter rbegin() const {
        return ConstReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the first element of the reversed map. It
     * corresponds to the last element of the non-reversed map. If the map is
     * empty, the returned iterator is equal to rend().
     */
    ConstReverseIter crbegin() const {
        return ConstReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed map. It corresponds to the element preceding the first
     * element of the non-reversed map. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ReverseIter rend() {
        return ReverseIter(begin());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed map. It corresponds to the element preceding the first
     * element of the non-reversed map. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ConstReverseIter rend() const {
        return ConstReverseIter(begin());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed map. It corresponds to the element preceding the first
     * element of the non-reversed map. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ConstReverseIter crend() const {
        return ConstReverseIter(begin());
    }

    /**
     * Checks if the container has no elements, i.e. whether begin() == end().
     */
    bool empty() const {
        return get_data().get_const().empty();
    }

    /**
     * Returns the number of elements in the container, i.e.
     * std::distance(begin(), end()).
     */
    size_type size() const {
        return get_data().get_const().size();
    }

    /**
     * Returns the maximum number of elements the container is able to hold due
     * to system or library implementation limitations, i.e.
     * std::distance(begin(), end()) for the largest container.
     */
    size_type max_size() const {
        return get_data().get_const().max_size();
    }

    /**
     * Erases all elements from the container. After this call, size() returns
     * zero.
     *
     * Invalidates any references, pointers, or iterators referring to contained
     * elements. Any past-the-end iterators are also invalidated.
     */
    void clear() {
        get_data().get_mut().clear();
    }

    /**
     * Inserts element(s) into the container, if the container doesn't already
     * contain an element with an equivalent key.
     *
     * No iterators or references are invalidated.
     */
    std::pair<iterator, bool> insert(const value_type &value) {
        auto p = get_data().get_mut_element_only().insert(value);
        return std::make_pair<iterator, bool>(iterator(std::move(p.first), data_ptr), std::move(p.second));
    }

    /**
     * Inserts element(s) into the container, if the container doesn't already
     * contain an element with an equivalent key. Equivalent to
     * emplace(std::forward<P>(value)) and only participates in overload
     * resolution if std::is_constructible<value_type, P&&>::value == true.
     *
     * No iterators or references are invalidated.
     */
    template <
        typename P,
        typename = typename std::enable_if<
            std::is_constructible<value_type, P&&>::value
        >::type
    >
    std::pair<iterator, bool> insert(P &&value) {
        auto p = get_data().get_mut_element_only().insert(value);
        return std::make_pair<iterator, bool>(iterator(std::move(p.first), data_ptr), std::move(p.second));
    }

    /**
     * Inserts value in the position just prior to hint, if the container
     * doesn't already contain an element with an equivalent key.
     *
     * No iterators or references are invalidated.
     */
    iterator insert(const_iterator hint, const value_type &value) {
        return iterator(get_data().get_mut_element_only().insert(hint, value), data_ptr);
    }

    /**
     * Inserts value in the position just prior to hint, if the container
     * doesn't already contain an element with an equivalent key. Equivalent to
     * emplace_hint(std::forward<P>(value)) and only participates in overload
     * resolution if std::is_constructible<value_type, P&&>::value == true.
     *
     * No iterators or references are invalidated.
     */
    template <
        typename P,
        typename = typename std::enable_if<
            std::is_constructible<value_type, P&&>::value
        >::type
    >
    iterator insert(const_iterator hint, P &&value) {
        return iterator(get_data().get_mut_element_only().insert(hint, value), data_ptr);
    }

    /**
     * Inserts elements from range [first, last). If multiple elements in the
     * range have keys that compare equivalent, it is unspecified which element
     * is inserted. Only inserts values when the corresponding key does not yet
     * exist in the map.
     *
     * No iterators or references are invalidated.
     */
    template <
        typename InputIt,
        typename = typename std::enable_if<std::is_convertible<
            typename std::iterator_traits<InputIt>::iterator_category,
            std::input_iterator_tag
        >::value>::type
    >
    iterator insert(const InputIt &first, const InputIt &last) {
        return iterator(get_data().get_mut_element_only().insert(first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last). If multiple elements in the
     * range have keys that compare equivalent, it is unspecified which element
     * is inserted. Only inserts values when the corresponding key does not yet
     * exist in the map.
     *
     * No iterators or references are invalidated.
     */
    iterator insert(const Iter &first, const Iter &last) {
        if (first.data_ptr == data_ptr) {
            throw ContainerException("inserting from same map");
        }
        return iterator(get_data().get_mut_element_only().insert(first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last). If multiple elements in the
     * range have keys that compare equivalent, it is unspecified which element
     * is inserted. Only inserts values when the corresponding key does not yet
     * exist in the map.
     *
     * No iterators or references are invalidated.
     */
    iterator insert(const ConstIter &first, const ConstIter &last) {
        if (first.data_ptr == data_ptr) {
            throw ContainerException("inserting from same map");
        }
        return iterator(get_data().get_mut_element_only().insert(first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last). If multiple elements in the
     * range have keys that compare equivalent, it is unspecified which element
     * is inserted. Only inserts values when the corresponding key does not yet
     * exist in the map.
     *
     * No iterators or references are invalidated.
     */
    iterator insert(const ReverseIter &first, const ReverseIter &last) {
        if (first.data_ptr == data_ptr) {
            throw ContainerException("inserting from same map");
        }
        return iterator(get_data().get_mut_element_only().insert(first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last). If multiple elements in the
     * range have keys that compare equivalent, it is unspecified which element
     * is inserted. Only inserts values when the corresponding key does not yet
     * exist in the map.
     *
     * No iterators or references are invalidated.
     */
    iterator insert(const ConstReverseIter &first, const ConstReverseIter &last) {
        if (first.data_ptr == data_ptr) {
            throw ContainerException("inserting from same map");
        }
        return iterator(get_data().get_mut_element_only().insert(first, last), data_ptr);
    }

    /**
     * Inserts elements from initializer list ilist. If multiple elements in the
     * range have keys that compare equivalent, it is unspecified which element
     * is inserted. Only inserts values when the corresponding key does not yet
     * exist in the map.
     *
     * No iterators or references are invalidated.
     */
    void insert(std::initializer_list<value_type> ilist) {
        get_data().get_mut_element_only().insert(ilist);
    }

    /**
     * Inserts a new element into the container constructed in-place with the
     * given args if there is no element with the key in the container.
     *
     * Careful use of emplace allows the new element to be constructed while
     * avoiding unnecessary copy or move operations. The constructor of the new
     * element (i.e. std::pair<const Key, T>) is called with exactly the same
     * arguments as supplied to emplace, forwarded via
     * std::forward<Args>(args).... The element may be constructed even if there
     * already is an element with the key in the container, in which case the
     * newly constructed element will be destroyed immediately.
     *
     * No iterators or references are invalidated.
     */
    template <class... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        auto p = get_data().get_mut_element_only().emplace(std::forward<Args>(args)...);
        return std::make_pair<iterator, bool>(iterator(std::move(p.first), data_ptr), std::move(p.second));
    }

    /**
     * Inserts a new element to the container as close as possible to the
     * position just before hint. The element is constructed in-place, i.e. no
     * copy or move operations are performed.
     *
     * The constructor of the element type (value_type, that is,
     * std::pair<const Key, T>) is called with exactly the same arguments as
     * supplied to the function, forwarded with std::forward<Args>(args)....
     *
     * No iterators or references are invalidated.
     */
    template <class... Args>
    std::pair<iterator, bool> emplace_hint(const ConstIter &pos, Args&&... args) {
        pos.check(data_ptr);
        auto p = get_data().get_mut_element_only().emplace_hint(std::forward<Args>(args)...);
        return std::make_pair<iterator, bool>(iterator(std::move(p.first), data_ptr), std::move(p.second));
    }

    /**
     * Removes the element at pos.
     *
     * The iterator pos must be valid and dereferenceable. Thus the end()
     * iterator (which is valid, but is not dereferenceable) cannot be used as a
     * value for pos.
     *
     * All iterators are invalidated.
     */
    Iter erase(const ConstIter &pos) {
        pos.check(data_ptr);
        *pos;
        return Iter(get_data().get_mut().erase(pos.iter), data_ptr);
    }

    /**
     * Removes the elements in the range [first, last).
     *
     * The iterator pos must be valid and dereferenceable. Thus the end()
     * iterator (which is valid, but is not dereferenceable) cannot be used as a
     * value for pos.
     *
     * The iterator first does not need to be dereferenceable if first==last:
     * erasing an empty range is a no-op.
     *
     * All iterators are invalidated.
     */
    Iter erase(const ConstIter &first, const Iter &last) {
        first.check(last);
        first.check(data_ptr);
        if (first != last) {
            *first;
        }
        return Iter(get_data().get_mut().erase(first.iter, last.iter), data_ptr);
    }

    /**
     * Removes the element (if one exists) with the key equivalent to key.
     *
     * All iterators are invalidated. Returns the number of elements removed.
     */
    size_type erase(const key_type &key) {
        return get_data().get_mut().erase(key);
    }

    /**
     * Swaps the data block of two containers.
     */
    void swap(CheckedMap &other) {
        std::swap(data_ptr, other.data_ptr);
    }

    /**
     * Returns the number of elements with key that compares equivalent to the
     * specified argument, which is either 1 or 0 since this container does not
     * allow duplicates.
     */
    size_type count(const Key &key) const {
        return get_data().get_const().count(key);
    }

    /**
     * Finds an element with key equivalent to key.
     */
    iterator find(const Key &key) {
        return Iter(get_data().get_mut_element_only().find(key), data_ptr);
    }

    /**
     * Finds an element with key equivalent to key.
     */
    const_iterator find(const Key &key) const {
        return ConstIter(get_data().get_const().find(key), data_ptr);
    }

    /**
     * Returns a range containing all elements with the given key in the
     * container. The range is defined by two iterators, one pointing to the
     * first element that is not less than key and another pointing to the first
     * element greater than key. Alternatively, the first iterator may be
     * obtained with lower_bound(), and the second with upper_bound().
     */
    std::pair<iterator, iterator> equal_range(const Key &key) {
        auto p = get_data().get_mut_element_only().equal_range(key);
        return std::make_pair<Iter, Iter>(Iter(p.first, data_ptr), Iter(p.second, data_ptr));
    }

    /**
     * Returns a range containing all elements with the given key in the
     * container. The range is defined by two iterators, one pointing to the
     * first element that is not less than key and another pointing to the first
     * element greater than key. Alternatively, the first iterator may be
     * obtained with lower_bound(), and the second with upper_bound().
     */
    std::pair<const_iterator, const_iterator> equal_range(const Key &key) const {
        auto p = get_data().get_const().equal_range(key);
        return std::make_pair<ConstIter, ConstIter>(ConstIter(p.first, data_ptr), ConstIter(p.second, data_ptr));
    }

    /**
     * Returns an iterator pointing to the first element that is not less than
     * (i.e. greater or equal to) key.
     */
    iterator lower_bound(const Key &key) {
        return Iter(get_data().get_mut_element_only().lower_bound(key), data_ptr);
    }

    /**
     * Returns an iterator pointing to the first element that is not less than
     * (i.e. greater or equal to) key.
     */
    const_iterator lower_bound(const Key &key) const {
        return ConstIter(get_data().get_const().lower_bound(key), data_ptr);
    }

    /**
     * Returns an iterator pointing to the first element that is greater than
     * key.
     */
    iterator upper_bound(const Key &key) {
        return Iter(get_data().get_mut_element_only().upper_bound(key), data_ptr);
    }

    /**
     * Returns an iterator pointing to the first element that is greater than
     * key.
     */
    const_iterator upper_bound(const Key &key) const {
        return ConstIter(get_data().get_const().upper_bound(key), data_ptr);
    }

    /**
     * Returns the function object that compares the keys, which is a copy of
     * this container's constructor argument comp.
     */
    key_compare key_comp() const {
        return get_data().get_const().key_comp();
    }

    /**
     * Returns a function object that compares objects of type
     * std::map::value_type (key-value pairs) by using key_comp to compare the
     * first components of the pairs.
     */
    typename Stl::value_compare value_comp() const {
        return get_data().get_const().value_comp();
    }

    /**
     * Checks if the contents of lhs and rhs are equal, that is, they have the
     * same number of elements and each element in lhs compares equal with the
     * element in rhs at the same position.
     */
    friend bool operator==(const CheckedMap &lhs, const CheckedMap &rhs) {
        return lhs.get_data().get_const() == rhs.get_data().get_const();
    }

    /**
     * Checks if the contents of lhs and rhs are equal, that is, they have the
     * same number of elements and each element in lhs compares equal with the
     * element in rhs at the same position.
     */
    friend bool operator!=(const CheckedMap &lhs, const CheckedMap &rhs) {
        return lhs.get_data().get_const() != rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator>(const CheckedMap &lhs, const CheckedMap &rhs) {
        return lhs.get_data().get_const() > rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator>=(const CheckedMap &lhs, const CheckedMap &rhs) {
        return lhs.get_data().get_const() >= rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator<(const CheckedMap &lhs, const CheckedMap &rhs) {
        return lhs.get_data().get_const() < rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator<=(const CheckedMap &lhs, const CheckedMap &rhs) {
        return lhs.get_data().get_const() <= rhs.get_data().get_const();
    }

};

/**
 * Stream << overload for CheckedMap<>.
 */
template <class Key, class T, class Compare>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::CheckedMap<Key, T, Compare> &map) {
    os << map.to_string();
    return os;
}

template <class Key, class T, class Compare = std::less<Key>>
#ifdef QL_CHECKED_MAP
using Map = CheckedMap<Key, T, Compare>;
#else
using Map = UncheckedMap<Key, T, Compare>;
#endif

} // namespace utils
} // namespace ql
