/** \file
 * Provides a wrapper for std::list that's safer to use and provides more
 * context when something goes wrong at runtime.
 */

#pragma once

#include <list>
#include "ql/config.h"
#include "ql/utils/str.h"
#include "ql/utils/container_base.h"

namespace ql {
namespace utils {

/**
 * Wrapper for `std::list` with additional error detection and handling.
 *
 * Unlike the STL variant, iterators detect accidental undefined behavior and
 * throw an exception instead.
 */
template <typename T, typename Allocator>
class UncheckedList : public std::list<T, Allocator> {
public:

    /**
     * Shorthand for the STL component being wrapped.
     */
    using Stl = std::list<T, Allocator>;

    /**
     * Forward iterator with mutable access to the values.
     */
    using Iter = typename Stl::iterator;

    /**
     * Forward iterator with const access to the values.
     */
    using ConstIter = typename Stl::const_iterator;

    /**
     * Backward iterator with mutable access to the values.
     */
    using ReverseIter = typename Stl::reverse_iterator;

    /**
     * Backward iterator with const access to the values.
     */
    using ConstReverseIter = typename Stl::const_reverse_iterator;

    // Member types expected by the standard library.
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = typename Stl::size_type;
    using difference_type = typename Stl::difference_type;
    using reference = typename Stl::reference;
    using const_reference = typename Stl::const_reference;
    using pointer = typename Stl::pointer;
    using const_pointer = typename Stl::const_pointer;
    using iterator = typename Stl::iterator;
    using const_iterator = typename Stl::const_iterator;
    using reverse_iterator = typename Stl::reverse_iterator;
    using const_reverse_iterator = typename Stl::const_reverse_iterator;

    /**
     *  Default constructor. Constructs an empty container with a
     *  default-constructed allocator.
     */
    UncheckedList() : Stl() {}

    /**
     * Constructs an empty container with the given allocator alloc.
     */
    explicit UncheckedList(const Allocator &alloc) : Stl(alloc) {}

    /**
     * Constructs the container with count copies of elements with value value.
     */
    UncheckedList(size_type count, const T &value, const Allocator &alloc = Allocator()) : Stl(count, value, alloc) {}

    /**
     * Constructs the container with count default-inserted instances of T. No
     * copies are made.
     */
    explicit UncheckedList(size_type count) : Stl(count) {};

    /**
     * Constructs the container with the contents of the range [first, last).
     *
     * This constructor has the same effect as
     * list(static_cast<size_type>(first), static_cast<value_type>(last), a) if
     * InputIt is an integral type.
     *
     * This overload only participates in overload resolution if InputIt
     * satisfies LegacyInputIterator, to avoid ambiguity with other overloads.
     */
    template <
        typename InputIt,
        typename = typename std::enable_if<std::is_convertible<
            typename std::iterator_traits<InputIt>::iterator_category,
            std::input_iterator_tag
        >::value>::type
    >
    UncheckedList(InputIt first, InputIt last, const Allocator &alloc = Allocator()) : Stl(first, last, alloc) {}

    /**
     * Copy constructor. Constructs the container with the copy of the contents
     * of other.
     */
    UncheckedList(const Stl &other) : Stl(other) {}

    /**
     * Constructs the container with the copy of the contents of other, using
     * alloc as the allocator.
     */
    UncheckedList(const Stl &other, const Allocator &alloc) : Stl(other, alloc) {}

    /**
     * Move constructor. Constructs the container with the contents of other
     * using move semantics. Allocator is obtained by move-construction from
     * the allocator belonging to other. After the move, other is guaranteed to
     * be empty().
     */
    UncheckedList(Stl &&other) : Stl(std::forward<Stl>(other)) {}

    /**
     * Allocator-extended move constructor. Using alloc as the allocator for the
     * new container, moving the contents from other; if
     * alloc != other.get_allocator(), this results in an element-wise move.
     * (in that case, other is not guaranteed to be empty after the move)
     */
    UncheckedList(Stl &&other, const Allocator &alloc) : Stl(std::forward<Stl>(other), alloc) {}

    /**
     * Constructs the container with the contents of the initializer list init.
     */
    UncheckedList(std::initializer_list<T> init, const Allocator &alloc = Allocator()) : Stl(init, alloc) {};

    /**
     * Returns a string representation of the entire contents of the list. A
     * stream << overload must exist for the value type.
     */
    std::string to_string(
        const std::string &prefix = "[",
        const std::string &separator = ", ",
        const std::string &suffix = "]",
        const std::string &last_separator = "",
        const std::string &only_separator = ""
    ) const {
        std::ostringstream ss{};
        ss << prefix;
        bool first = true;
        for (auto it = this->cbegin(); it != this->cend(); ++it) {
            if (first) {
                first = false;
            } else {
                if (it == std::prev(this->cend())) {
                    if (it == std::next(this->cbegin())) {
                        ss << (only_separator.empty() ? separator : only_separator);
                    } else {
                        ss << (last_separator.empty() ? separator : last_separator);
                    }
                } else {
                    ss << separator;
                }
            }
            ss << *it;
        }
        ss << suffix;
        return ss.str();
    }

};

/**
 * Stream << overload for UncheckedList<>.
 */
template <class T>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::UncheckedList<T> &list) {
    os << list.to_string();
    return os;
}

/**
 * Wrapper for `std::list` with additional error detection and handling.
 *
 * Unlike the STL variant, iterators detect accidental undefined behavior and
 * throw an exception instead.
 */
template <typename T, typename Allocator>
class CheckedList {
public:

    /**
     * Shorthand for the STL component being wrapped.
     */
    using Stl = std::list<T, Allocator>;

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
    using value_type = T;
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
     * Data block for the list.
     */
    std::shared_ptr<Data> data_ptr;

    /**
     * Safely returns mutable access to the data block.
     */
    Data &get_data() {
        if (!data_ptr) {
            QL_CONTAINER_ERROR(
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
            QL_CONTAINER_ERROR(
                "container is used after move or otherwise has invalid data block"
            );
        }
        return *data_ptr;
    }

public:

    /**
     *  Default constructor. Constructs an empty container with a
     *  default-constructed allocator.
     */
    CheckedList() : data_ptr(std::make_shared<Data>()) {}

    /**
     * Constructs an empty container with the given allocator alloc.
     */
    explicit CheckedList(const Allocator &alloc) : data_ptr(std::make_shared<Data>(alloc)) {}

    /**
     * Constructs the container with count copies of elements with value value.
     */
    CheckedList(size_type count, const T &value, const Allocator &alloc = Allocator()) : data_ptr(std::make_shared<Data>(count, value, alloc)) {}

    /**
     * Constructs the container with count default-inserted instances of T. No
     * copies are made.
     */
    explicit CheckedList(size_type count) : data_ptr(std::make_shared<Data>(count)) {};

    /**
     * Constructs the container with the contents of the range [first, last).
     *
     * This constructor has the same effect as
     * list(static_cast<size_type>(first), static_cast<value_type>(last), a) if
     * InputIt is an integral type.
     *
     * This overload only participates in overload resolution if InputIt
     * satisfies LegacyInputIterator, to avoid ambiguity with other overloads.
     */
    template <
        typename InputIt,
        typename = typename std::enable_if<std::is_convertible<
            typename std::iterator_traits<InputIt>::iterator_category,
            std::input_iterator_tag
        >::value>::type
    >
    CheckedList(InputIt first, InputIt last, const Allocator &alloc = Allocator()) : data_ptr(std::make_shared<Data>(first, last, alloc)) {}

    /**
     * Copy constructor. Constructs the container with the copy of the contents
     * of other.
     */
    CheckedList(const CheckedList &other) : data_ptr(std::make_shared<Data>(other.get_data().get_const())) {}

    /**
     * Copy constructor. Constructs the container with the copy of the contents
     * of other.
     */
    CheckedList(const Stl &other) : data_ptr(std::make_shared<Data>(other)) {}

    /**
     * Constructs the container with the copy of the contents of other, using
     * alloc as the allocator.
     */
    CheckedList(const Stl &other, const Allocator &alloc) : data_ptr(std::make_shared<Data>(other, alloc)) {}

    /**
     * Move constructor. The data block pointer is moved, so iterators remain
     * valid.
     */
    CheckedList(CheckedList &&other) noexcept = default;

    /**
     * Move constructor. Constructs the container with the contents of other
     * using move semantics. Allocator is obtained by move-construction from
     * the allocator belonging to other. After the move, other is guaranteed to
     * be empty().
     */
    CheckedList(Stl &&other) : data_ptr(std::make_shared<Data>(std::forward<Stl>(other))) {}

    /**
     * Allocator-extended move constructor. Using alloc as the allocator for the
     * new container, moving the contents from other; if
     * alloc != other.get_allocator(), this results in an element-wise move.
     * (in that case, other is not guaranteed to be empty after the move)
     */
    CheckedList(Stl &&other, const Allocator &alloc) : data_ptr(std::make_shared<Data>(std::forward<Stl>(other), alloc)) {}

    /**
     * Constructs the container with the contents of the initializer list init.
     */
    CheckedList(std::initializer_list<T> init, const Allocator &alloc = Allocator()) : data_ptr(std::make_shared<Data>(init, alloc)) {};

    /**
     * Copy assignment from an STL list.
     */
    CheckedList &operator=(const Stl &other) {
        get_data().get_mut().operator=(other);
        return *this;
    }

    /**
     * Move assignment from an STL list.
     */
    CheckedList &operator=(Stl &&other) {
        get_data().get_mut().operator=(std::move(other));
        return *this;
    }

    /**
     * Copy assignment.
     */
    CheckedList &operator=(const CheckedList &rhs) {
        get_data().get_mut().operator=(rhs.get_data().get_const());
        return *this;
    }

    /**
     * Move assignment. The data block pointer is moved, so iterators remain
     * valid.
     */
    CheckedList &operator=(CheckedList &&rhs) noexcept = default;

    /**
     * Replaces the contents with those identified by initializer list ilist.
     */
    CheckedList &operator=(std::initializer_list<T> ilist) {
        get_data().get_mut().operator=(ilist);
        return *this;
    }

    /**
     * Replaces the contents with count copies of value value.
     */
    void assign(typename Stl::size_type count, const T &value) {
        get_data().get_mut().assign(count, value);
    }

    /**
     * Replaces the contents with copies of those in the range [first, last).
     */
    template <
        typename InputIt,
        typename = typename std::enable_if<std::is_convertible<
            typename std::iterator_traits<InputIt>::iterator_category,
            std::input_iterator_tag
        >::value>::type
    >
    void assign(InputIt first, InputIt last) {
        get_data().get_mut().assign(first, last);
    }

    /**
     * Replaces the contents with copies of those in the range [first, last).
     */
    template <typename A, typename B, typename C>
    void assign(WrappedIterator<A, B, C> first, WrappedIterator<A, B, C> last) {
        first.check(last);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR(
                "cannot assign using iterators from the same container"
            );
        }
        get_data().get_mut().assign(first, last);
    }

    /**
     * Replaces the contents with the elements from the initializer list ilist.
     */
    void assign(std::initializer_list<T> ilist) {
        get_data().get_mut().assign(ilist);
    }

    /**
     * Returns the allocator associated with the container.
     */
    typename Stl::allocator_type get_allocator() const {
        return get_data().get_const().get_allocator();
    }

    /**
     * Returns a string representation of the entire contents of the list. A
     * stream << overload must exist for the value type.
     */
    std::string to_string(
        const std::string &prefix = "[",
        const std::string &separator = ", ",
        const std::string &suffix = "]",
        const std::string &last_separator = "",
        const std::string &only_separator = ""
    ) const {
        std::ostringstream ss{};
        ss << prefix;
        bool first = true;
        for (auto it = this->cbegin(); it != this->cend(); ++it) {
            if (first) {
                first = false;
            } else {
                if (it == std::prev(this->cend())) {
                    if (it == std::next(this->cbegin())) {
                        ss << (only_separator.empty() ? separator : only_separator);
                    } else {
                        ss << (last_separator.empty() ? separator : last_separator);
                    }
                } else {
                    ss << separator;
                }
            }
            ss << *it;
        }
        ss << suffix;
        return ss.str();
    }

    /**
     * Returns a reference to the first element in the container. If the list
     * is empty, an exception of type ContainerException is thrown.
     */
    reference front() {
        auto &v = get_data().get_mut_element_only();
        if (v.empty()) {
            QL_CONTAINER_ERROR("front() called on empty list");
        }
        return v.front();
    }

    /**
     * Returns a reference to the first element in the container. If the list
     * is empty, an exception of type ContainerException is thrown.
     */
    const_reference front() const {
        auto &v = get_data().get_const();
        if (v.empty()) {
            QL_CONTAINER_ERROR("front() called on empty list");
        }
        return v.front();
    }

    /**
     * Returns a reference to the last element in the container. If the list
     * is empty, an exception of type ContainerException is thrown.
     */
    reference back() {
        auto &v = get_data().get_mut_element_only();
        if (v.empty()) {
            QL_CONTAINER_ERROR("back() called on empty list");
        }
        return v.back();
    }

    /**
     * Returns a reference to the last element in the container. If the list
     * is empty, an exception of type ContainerException is thrown.
     */
    const_reference back() const {
        auto &v = get_data().get_const();
        if (v.empty()) {
            QL_CONTAINER_ERROR("back() called on empty list");
        }
        return v.back();
    }

    /**
     * Returns an iterator to the first element of the list. If the list is
     * empty, the returned iterator will be equal to end().
     */
    Iter begin() {
        return Iter(get_data().get_mut_element_only().begin(), data_ptr);
    }

    /**
     * Returns an iterator to the first element of the list. If the list is
     * empty, the returned iterator will be equal to end().
     */
    ConstIter begin() const {
        return ConstIter(get_data().get_const().cbegin(), data_ptr);
    }

    /**
     * Returns an iterator to the first element of the list. If the list is
     * empty, the returned iterator will be equal to end().
     */
    ConstIter cbegin() const {
        return ConstIter(get_data().get_const().cbegin(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the
     * list. This element acts as a placeholder; attempting to access it
     * results in an exception.
     */
    Iter end() {
        return Iter(get_data().get_mut_element_only().end(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the
     * list. This element acts as a placeholder; attempting to access it
     * results in an exception.
     */
    ConstIter end() const {
        return ConstIter(get_data().get_const().cend(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the
     * list. This element acts as a placeholder; attempting to access it
     * results in an exception.
     */
    ConstIter cend() const {
        return ConstIter(get_data().get_const().cend(), data_ptr);
    }

    /**
     * Returns a reverse iterator to the first element of the reversed list.
     * It corresponds to the last element of the non-reversed list. If the
     * list is empty, the returned iterator is equal to rend().
     */
    ReverseIter rbegin() {
        return ReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the first element of the reversed list.
     * It corresponds to the last element of the non-reversed list. If the
     * list is empty, the returned iterator is equal to rend().
     */
    ConstReverseIter rbegin() const {
        return ConstReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the first element of the reversed list.
     * It corresponds to the last element of the non-reversed list. If the
     * list is empty, the returned iterator is equal to rend().
     */
    ConstReverseIter crbegin() const {
        return ConstReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed list. It corresponds to the element preceding the first
     * element of the non-reversed list. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ReverseIter rend() {
        return ReverseIter(begin());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed list. It corresponds to the element preceding the first
     * element of the non-reversed list. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ConstReverseIter rend() const {
        return ConstReverseIter(begin());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed list. It corresponds to the element preceding the first
     * element of the non-reversed list. This element acts as a placeholder;
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
     * Inserts value before pos.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const T &value) {
        pos.check(data_ptr);
        return Iter(get_data().get_mut().insert(pos.iter, value), data_ptr);
    }

    /**
     * Inserts value before pos.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, T &&value) {
        pos.check(data_ptr);
        return Iter(get_data().get_mut().insert(pos.iter, value), data_ptr);
    }

    /**
     * Inserts count copies of the value before pos.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, size_type count, const T &value) {
        pos.check(data_ptr);
        return Iter(get_data().get_mut().insert(pos.iter, count, value), data_ptr);
    }

    /**
     * Inserts elements from range [first, last) before pos.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    template <
        typename InputIt,
        typename = typename std::enable_if<std::is_convertible<
            typename std::iterator_traits<InputIt>::iterator_category,
            std::input_iterator_tag
        >::value>::type
    >
    Iter insert(const ConstIter &pos, const InputIt &first, const InputIt &last) {
        pos.check(data_ptr);
        return Iter(get_data().get_mut().insert(pos.iter, first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last) before pos. A
     * ContainerException is thrown if first and last belong to this list.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const Iter &first, const Iter &last) {
        pos.check(data_ptr);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR("inserting from same list");
        }
        return Iter(get_data().get_mut().insert(pos.iter, first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last) before pos. A
     * ContainerException is thrown if first and last belong to this list.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const ConstIter &first, const ConstIter &last) {
        pos.check(data_ptr);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR("inserting from same list");
        }
        return Iter(get_data().get_mut().insert(pos.iter, first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last) before pos. A
     * ContainerException is thrown if first and last belong to this list.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const ReverseIter &first, const ReverseIter &last) {
        pos.check(data_ptr);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR("inserting from same list");
        }
        return Iter(get_data().get_mut().insert(pos.iter, first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last) before pos. A
     * ContainerException is thrown if first and last belong to this list.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const ConstReverseIter &first, const ConstReverseIter &last) {
        pos.check(data_ptr);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR("inserting from same list");
        }
        return Iter(get_data().get_mut().insert(pos.iter, first, last), data_ptr);
    }

    /**
     * Inserts elements from initializer list ilist before pos.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, std::initializer_list<T> ilist) {
        pos.check(data_ptr);
        return Iter(get_data().get_mut().insert(pos.iter, ilist), data_ptr);
    }

    /**
     * Inserts a new element into the container directly before pos.
     *
     * The element is constructed through std::allocator_traits::construct,
     * which typically uses placement-new to construct the element in-place at a
     * location provided by the container. However, if the required location has
     * been occupied by an existing element, the inserted element is constructed
     * at another location at first, and then move assigned into the required
     * location.
     *
     * The arguments args... are forwarded to the constructor as
     * std::forward<Args>(args).... args... may directly or indirectly refer to
     * a value in the container.
     *
     * All iterators and references are invalidated.
     */
    template <class... Args>
    Iter emplace(const ConstIter &pos, Args&&... args) {
        pos.check(data_ptr);
        return Iter(get_data().get_mut().emplace(pos.iter, std::forward<Args>(args)...), data_ptr);
    }

    /**
     * Removes the element at pos.
     *
     * The iterator pos must be valid and dereferenceable. Thus the end()
     * iterator (which is valid, but is not dereferenceable) cannot be used as a
     * value for pos.
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
     * Appends the given element value to the end of the container. The new
     * element is initialized as a copy of value.
     *
     * Iterators and references remain valid.
     */
    void push_back(const T &value) {
        return get_data().get_mut_element_only().push_back(value);
    }

    /**
     * Appends the given element value to the end of the container. value is
     * moved into the new element.
     *
     * Iterators and references remain valid.
     */
    void push_back(T &&value) {
        return get_data().get_mut_element_only().push_back(std::move(value));
    }

    /**
     * Appends a new element to the end of the container. The element is
     * constructed through std::allocator_traits::construct, which typically
     * uses placement-new to construct the element in-place at the location
     * provided by the container. The arguments args... are forwarded to the
     * constructor as std::forward<Args>(args)....
     *
     * Iterators and references remain valid.
     */
    template <class... Args>
    void emplace_back(Args&&... args) {
        get_data().get_mut_element_only().emplace_back(std::forward<Args>(args)...);
    }

    /**
     * Removes the last element of the container.
     *
     * Calling pop_back on an empty container results in a ContainerException.
     *
     * All iterators and references are invalidated.
     */
    void pop_back() {
        auto &v = get_data().get_mut();
        if (v.empty()) {
            QL_CONTAINER_ERROR("pop_back() called on empty list");
        }
        v.pop_back();
    }

    /**
     * Prepends the given element value to the beginning of the container. The
     * new element is initialized as a copy of value.
     *
     * Iterators and references remain valid.
     */
    void push_front(const T &value) {
        get_data().get_mut_element_only().push_front(value);
    }

    /**
     * Prepends the given element value to the beginning of the container. value
     * is moved into the new element.
     *
     * Iterators and references remain valid.
     */
    void push_front(T &&value) {
        get_data().get_mut_element_only().push_front(std::move(value));
    }

    /**
     * Inserts a new element to the beginning of the container. The element is
     * constructed through std::allocator_traits::construct, which typically
     * uses placement-new to construct the element in-place at the location
     * provided by the container. The arguments args... are forwarded to the
     * constructor as std::forward<Args>(args)...
     *
     * Iterators and references remain valid.
     */
    template <class... Args>
    void emplace_front(Args&&... args) {
        get_data().get_mut_element_only().emplace_front(std::forward<Args>(args)...);
    }

    /**
     * Removes the last element of the container.
     *
     * Removes the first element of the container. If there are no elements in
     * the container, the behavior is undefined.
     *
     * All iterators and references are invalidated.
     */
    void pop_front() {
        auto &v = get_data().get_mut();
        if (v.empty()) {
            QL_CONTAINER_ERROR("pop_back() called on empty list");
        }
        v.pop_front();
    }

    /**
     * Resizes the container to contain count elements.
     *
     * If the current size is greater than count, the container is reduced to
     * its first count elements.
     *
     * If the current size is less than count, additional default-inserted
     * elements are appended.
     */
    void resize(size_type count) {
        get_data().get_mut().resize(count);
    }

    /**
     * Resizes the container to contain count elements.
     *
     * If the current size is greater than count, the container is reduced to
     * its first count elements.
     *
     * If the current size is less than count, additional copies of value are
     * appended.
     */
    void resize(size_type count, const T &value) {
        get_data().get_mut().resize(count, value);
    }

    /**
     * Swaps the data block of two containers.
     */
    void swap(CheckedList &other) {
        std::swap(data_ptr, other.data_ptr);
    }

    /**
     * Merges two sorted lists into one. The lists should be sorted into
     * ascending order.
     *
     * No elements are copied. The container other becomes empty after the
     * operation. The function does nothing if other refers to the same object
     * as *this. If get_allocator() != other.get_allocator(), the behavior is
     * undefined. All iterators are invalidated. The first version uses
     * operator< to compare the elements, the second version uses the given
     * comparison function comp.
     *
     * This operation is stable: for equivalent elements in the two lists, the
     * elements from *this shall always precede the elements from other, and the
     * order of equivalent elements of *this and other does not change.
     */
    void merge(CheckedList &other) {
        get_data().get_mut().merge(other.get_data().get_mut());
    }

    /**
     * Merges two sorted lists into one. The lists should be sorted into
     * ascending order.
     *
     * No elements are copied. The container other becomes empty after the
     * operation. The function does nothing if other refers to the same object
     * as *this. If get_allocator() != other.get_allocator(), the behavior is
     * undefined. All iterators are invalidated. The first version uses
     * operator< to compare the elements, the second version uses the given
     * comparison function comp.
     *
     * This operation is stable: for equivalent elements in the two lists, the
     * elements from *this shall always precede the elements from other, and the
     * order of equivalent elements of *this and other does not change.
     */
    template <class Compare>
    void merge(CheckedList &other, Compare comp) {
        get_data().get_mut().merge(other.get_data().get_mut(), comp);
    }

    /**
     * Transfers all elements from other into *this. The elements are inserted
     * before the element pointed to by pos. The container other becomes empty
     * after the operation. The behavior is undefined if other refers to the
     * same object as *this.
     *
     * No elements are copied or moved, only the internal pointers of the list
     * nodes are re-pointed. The behavior is undefined if
     * get_allocator() != other.get_allocator(). All iterators are invalidated.
     */
    void splice(const ConstIter &pos, CheckedList &other) {
        pos.check(data_ptr);
        get_data().get_mut().splice(pos.iter, other.get_data().get_mut());
    }

    /**
     * Transfers the element pointed to by it from other into *this. The element
     * is inserted before the element pointed to by pos.
     *
     * No elements are copied or moved, only the internal pointers of the list
     * nodes are re-pointed. The behavior is undefined if
     * get_allocator() != other.get_allocator(). All iterators are invalidated.
     */
    void splice(const ConstIter &pos, CheckedList &other, const ConstIter &it) {
        pos.check(data_ptr);
        it.check(other.data_ptr);
        get_data().get_mut().splice(pos.iter, other.get_data().get_mut(), it.iter);
    }

    /**
     * Transfers the elements in the range [first, last) from other into *this.
     * The elements are inserted before the element pointed to by pos.
     *
     * \warn The behavior is undefined if pos is an iterator in the range
     * [first,last). Iterator safety does not check for this case.
     *
     * No elements are copied or moved, only the internal pointers of the list
     * nodes are re-pointed. The behavior is undefined if
     * get_allocator() != other.get_allocator(). All iterators are invalidated.
     */
    void splice(const ConstIter &pos, CheckedList &other, const ConstIter &first, const ConstIter &last) {
        pos.check(data_ptr);
        first.check(other.data_ptr);
        first.check(other);
        get_data().get_mut().splice(pos.iter, other.get_data().get_mut(), first.iter, last.iter);
    }

    /**
     * Removes all elements that are equal to value.
     */
    void remove(const T &value) {
        get_data().get_mut().remove(value);
    }

    /**
     * Removes all elements for which predicate p returns true.
     */
    template <class UnaryPredicate>
    void remove_if(UnaryPredicate p) {
        get_data().get_mut().remove_if(p);
    }

    /**
     * Reverses the order of the elements in the container. No references or
     * iterators become invalidated.
     */
    void reverse() {
        get_data().get_mut_element_only().reverse();
    }

    /**
     * Removes all consecutive duplicate elements from the container. Only the
     * first element in each group of equal elements is left. operator== is used
     * to compare the elements.
     */
    void unique() {
        get_data().get_mut().unique();
    }

    /**
     * Removes all consecutive duplicate elements from the container. Only the
     * first element in each group of equal elements is left. The predicate is
     * used to compare the elements.
     */
    template <class BinaryPredicate>
    void unique(BinaryPredicate p) {
        get_data().get_mut().unique(p);
    }

    /**
     * Sorts the elements in ascending order. The order of equal elements is
     * preserved. operator< is used to compare the elements.
     */
    void sort() {
        get_data().get_mut().sort();
    }

    /**
     * Sorts the elements in ascending order. The order of equal elements is
     * preserved. the given comparison function is used to compare the elements.
     */
    template <class Compare>
    void sort(Compare comp) {
        get_data().get_mut().sort(comp);
    }

    /**
     * Checks if the contents of lhs and rhs are equal, that is, they have the
     * same number of elements and each element in lhs compares equal with the
     * element in rhs at the same position.
     */
    friend bool operator==(const CheckedList &lhs, const CheckedList &rhs) {
        return lhs.get_data().get_const() == rhs.get_data().get_const();
    }

    /**
     * Checks if the contents of lhs and rhs are equal, that is, they have the
     * same number of elements and each element in lhs compares equal with the
     * element in rhs at the same position.
     */
    friend bool operator!=(const CheckedList &lhs, const CheckedList &rhs) {
        return lhs.get_data().get_const() != rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator>(const CheckedList &lhs, const CheckedList &rhs) {
        return lhs.get_data().get_const() > rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator>=(const CheckedList &lhs, const CheckedList &rhs) {
        return lhs.get_data().get_const() >= rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator<(const CheckedList &lhs, const CheckedList &rhs) {
        return lhs.get_data().get_const() < rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator<=(const CheckedList &lhs, const CheckedList &rhs) {
        return lhs.get_data().get_const() <= rhs.get_data().get_const();
    }

};

/**
 * Stream << overload for CheckedList<>.
 */
template <class T>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::CheckedList<T> &list) {
    os << list.to_string();
    return os;
}

template <typename T, typename Allocator = std::allocator<T>>
#ifdef QL_CHECKED_LIST
using List = CheckedList<T, Allocator>;
#else
using List = UncheckedList<T, Allocator>;
#endif

} // namespace utils
} // namespace ql
