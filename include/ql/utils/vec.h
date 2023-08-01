/** \file
 * Provides a wrapper for std::vector that's safer to use and provides more
 * context when something goes wrong at runtime.
 */

#pragma once

#include <vector>
#include "ql/config.h"
#include "ql/utils/str.h"
#include "ql/utils/container_base.h"

namespace ql {
namespace utils {

/**
 * Wrapper for `std::vector` with range checks on operator[], but no further
 * check.
 *
 * Unlike the STL variant, operator[] is range-checked; it basically functions
 * like at(). The iterators are also wrapped to detect accidental undefined
 * behavior.
 */
template <class T, class Allocator>
class UncheckedVec : public std::vector<T, Allocator> {
public:

    /**
     * Typedef for the wrapped STL container.
     */
    using Stl = std::vector<T, Allocator>;

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
     * Default constructor. Constructs an empty container with a
     * default-constructed allocator.
     */
    UncheckedVec() : Stl() {}

    /**
     * Constructs an empty container with the given allocator alloc.
     */
    explicit UncheckedVec(const Allocator &alloc) : Stl(alloc) {}

    /**
     * Constructs the container with count copies of elements with value value.
     */
    UncheckedVec(size_type count, const T &value, const Allocator &alloc = Allocator()) : Stl(count, value, alloc) {}

    /**
     * Constructs the container with count default-inserted instances of T. No
     * copies are made.
     */
    explicit UncheckedVec(size_type count) : Stl(count) {}

    /**
     * Constructs the container with the contents of the range [first, last).
     *
     * This constructor has the same effect as
     * vector(static_cast<size_type>(first), static_cast<value_type>(last), a)
     * if InputIt is an integral type.
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
    UncheckedVec(InputIt first, InputIt last, const Allocator &alloc = Allocator()) : Stl(first, last, alloc) {}

    /**
     * Copy constructor. Constructs the container with the copy of the contents
     * of other.
     */
    UncheckedVec(const Stl &other) : Stl(other) {}

    /**
     * Constructs the container with the copy of the contents of other, using
     * alloc as the allocator.
     */
    UncheckedVec(const Stl &other, const Allocator &alloc) : Stl(other, alloc) {}

    /**
     * Move constructor. Constructs the container with the contents of other
     * using move semantics. Allocator is obtained by move-construction from
     * the allocator belonging to other. After the move, other is guaranteed to
     * be empty().
     */
    UncheckedVec(Stl &&other) : Stl(std::forward<Stl>(other)) {}

    /**
     * Allocator-extended move constructor. Using alloc as the allocator for the
     * new container, moving the contents from other; if
     * alloc != other.get_allocator(), this results in an element-wise move.
     * (in that case, other is not guaranteed to be empty after the move)
     */
    UncheckedVec(Stl &&other, const Allocator &alloc) : Stl(std::forward<Stl>(other), alloc) {}

    /**
     * Constructs the container with the contents of the initializer list init.
     */
    UncheckedVec(std::initializer_list<T> init, const Allocator &alloc = Allocator()) : Stl(init, alloc) {};

    /**
     * Returns a reference to the element at specified location pos, with bounds
     * checking. If pos is not within the range of the container, an exception
     * of type ContainerException is thrown.
     */
    reference at(size_type pos) {
        if (pos >= this->size()) {
            QL_CONTAINER_ERROR(
                "index " + std::to_string(pos) + " is out of range, "
                "size is " + std::to_string(this->size())
            );
        }
        return Stl::operator[](pos);
    }

    /**
     * Returns a const reference to the element at specified location pos, with
     * bounds checking. If pos is not within the range of the container, an
     * exception of type ContainerException is thrown.
     */
    const_reference at(size_type pos) const {
        if (pos >= this->size()) {
            QL_CONTAINER_ERROR(
                "index " + std::to_string(pos) + " is out of range, "
                "size is " + std::to_string(this->size())
            );
        }
        return Stl::operator[](pos);
    }

    /**
     * Returns a reference to the element at specified location pos, with bounds
     * checking. If pos is not within the range of the container, an exception
     * of type ContainerException is thrown.
     */
    reference operator[](size_type pos) {
        return this->at(pos);
    }

    /**
     * Returns a const reference to the element at specified location pos, with
     * bounds checking. If pos is not within the range of the container, an
     * exception of type ContainerException is thrown.
     */
    const_reference operator[](size_type pos) const {
        return this->at(pos);
    }

    /**
     * Returns UNCHECKED mutable access to the value stored at the given index.
     * Unless you've exhausted all other possibilities for optimization and
     * things still run unacceptably slow, and you find out that at() is somehow
     * the culprit, you really should be using at().
     */
    reference unchecked_at(size_type index) {
        return Stl::operator[](index);
    }

    /**
     * Returns UNCHECKED const access to the value stored at the given index.
     * Unless you've exhausted all other possibilities for optimization and
     * things still run unacceptably slow, and you find out that at() is somehow
     * the culprit, you really should be using at().
     */
    const_reference unchecked_at(size_type index) const {
        return Stl::operator[](index);
    }

    /**
     * Returns a const reference to the value at the given index, or to a dummy
     * default-constructed value if the index is out of range.
     */
    const_reference get(size_type index) const {
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
 * Stream << overload for UncheckedVec<>.
 */
template <class T>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::UncheckedVec<T> &vec) {
    os << vec.to_string();
    return os;
}

/**
 * Wrapper for `std::vector` with additional error detection and handling.
 *
 * Unlike the STL variant, operator[] is range-checked; it basically functions
 * like at(). The iterators are also wrapped to detect accidental undefined
 * behavior.
 */
template <typename T, typename Allocator>
class CheckedVec {
public:

    /**
     * Shorthand for the STL component being wrapped.
     */
    using Stl = std::vector<T, Allocator>;

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
     * Data block for the vector.
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
     * Default constructor. Constructs an empty container with a
     * default-constructed allocator.
     */
    CheckedVec() : data_ptr(std::make_shared<Data>()) {}

    /**
     * Constructs an empty container with the given allocator alloc.
     */
    explicit CheckedVec(const Allocator &alloc) : data_ptr(std::make_shared<Data>(alloc)) {}

    /**
     * Constructs the container with count copies of elements with value value.
     */
    CheckedVec(size_type count, const T &value, const Allocator &alloc = Allocator()) : data_ptr(std::make_shared<Data>(count, value, alloc)) {}

    /**
     * Constructs the container with count default-inserted instances of T. No
     * copies are made.
     */
    explicit CheckedVec(size_type count) : data_ptr(std::make_shared<Data>(count)) {}

    /**
     * Constructs the container with the contents of the range [first, last).
     *
     * This constructor has the same effect as
     * vector(static_cast<size_type>(first), static_cast<value_type>(last), a)
     * if InputIt is an integral type.
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
    CheckedVec(InputIt first, InputIt last, const Allocator &alloc = Allocator()) : data_ptr(std::make_shared<Data>(first, last, alloc)) {}

    /**
     * Copy constructor. Constructs the container with the copy of the contents
     * of other.
     */
    CheckedVec(const CheckedVec &other) : data_ptr(std::make_shared<Data>(other.get_data().get_const())) {}

    /**
     * Copy constructor. Constructs the container with the copy of the contents
     * of other.
     */
    CheckedVec(const Stl &other) : data_ptr(std::make_shared<Data>(other)) {}

    /**
     * Constructs the container with the copy of the contents of other, using
     * alloc as the allocator.
     */
    CheckedVec(const Stl &other, const Allocator &alloc) : data_ptr(std::make_shared<Data>(other, alloc)) {}

    /**
     * Move constructor. The data block pointer is moved, so iterators remain
     * valid.
     */
    CheckedVec(CheckedVec &&other) noexcept = default;

    /**
     * Move constructor. Constructs the container with the contents of other
     * using move semantics. Allocator is obtained by move-construction from
     * the allocator belonging to other. After the move, other is guaranteed to
     * be empty().
     */
    CheckedVec(Stl &&other) : data_ptr(std::make_shared<Data>(std::forward<Stl>(other))) {}

    /**
     * Allocator-extended move constructor. Using alloc as the allocator for the
     * new container, moving the contents from other; if
     * alloc != other.get_allocator(), this results in an element-wise move.
     * (in that case, other is not guaranteed to be empty after the move)
     */
    CheckedVec(Stl &&other, const Allocator &alloc) : data_ptr(std::make_shared<Data>(std::forward<Stl>(other), alloc)) {}

    /**
     * Constructs the container with the contents of the initializer list init.
     */
    CheckedVec(std::initializer_list<T> init, const Allocator &alloc = Allocator()) : data_ptr(std::make_shared<Data>(init, alloc)) {};

    /**
     * Copy assignment from an STL vector.
     */
    CheckedVec &operator=(const Stl &other) {
        get_data().get_mut().operator=(other);
        return *this;
    }

    /**
     * Move assignment from an STL vector.
     */
    CheckedVec &operator=(Stl &&other) {
        get_data().get_mut().operator=(std::move(other));
        return *this;
    }

    /**
     * Copy assignment.
     */
    CheckedVec &operator=(const CheckedVec &rhs) {
        get_data().get_mut().operator=(rhs.get_data().get_const());
        return *this;
    }

    /**
     * Move assignment. The data block pointer is moved, so iterators remain
     * valid.
     */
    CheckedVec &operator=(CheckedVec &&rhs) noexcept = default;

    /**
     * Replaces the contents with those identified by initializer list ilist.
     */
    CheckedVec &operator=(std::initializer_list<T> ilist) {
        get_data().get_mut().operator=(ilist);
        return *this;
    }

    /**
     * Replaces the contents with count copies of value value.
     */
    void assign(size_type count, const T &value) {
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
    allocator_type get_allocator() const {
        return get_data().get_const().get_allocator();
    }

    /**
     * Returns a reference to the element at specified location pos, with bounds
     * checking. If pos is not within the range of the container, an exception
     * of type ContainerException is thrown.
     */
    reference at(size_type pos) {
        auto &v = get_data().get_mut_element_only();
        if (pos >= v.size()) {
            QL_CONTAINER_ERROR(
                "index " + std::to_string(pos) + " is out of range, "
                "size is " + std::to_string(v.size())
            );
        }
        return v[pos];
    }

    /**
     * Returns a const reference to the element at specified location pos, with
     * bounds checking. If pos is not within the range of the container, an
     * exception of type ContainerException is thrown.
     */
    const_reference at(size_type pos) const {
        auto &v = get_data().get_const();
        if (pos >= v.size()) {
            QL_CONTAINER_ERROR(
                "index " + std::to_string(pos) + " is out of range, "
                "size is " + std::to_string(v.size())
            );
        }
        return v[pos];
    }

    /**
     * Returns a reference to the element at specified location pos, with bounds
     * checking. If pos is not within the range of the container, an exception
     * of type ContainerException is thrown.
     */
    reference operator[](size_type pos) {
        return this->at(pos);
    }

    /**
     * Returns a const reference to the element at specified location pos, with
     * bounds checking. If pos is not within the range of the container, an
     * exception of type ContainerException is thrown.
     */
    const_reference operator[](size_type pos) const {
        return this->at(pos);
    }

    /**
     * Returns UNCHECKED mutable access to the value stored at the given index.
     * Unless you've exhausted all other possibilities for optimization and
     * things still run unacceptably slow, and you find out that at() is somehow
     * the culprit, you really should be using at().
     */
    reference unchecked_at(size_type index) {
        return Stl::operator[](index);
    }

    /**
     * Returns UNCHECKED const access to the value stored at the given index.
     * Unless you've exhausted all other possibilities for optimization and
     * things still run unacceptably slow, and you find out that at() is somehow
     * the culprit, you really should be using at().
     */
    const_reference unchecked_at(size_type index) const {
        return Stl::operator[](index);
    }

    /**
     * Returns a const reference to the value at the given index, or to a dummy
     * default-constructed value if the index is out of range.
     */
    const_reference get(size_type index) const {
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
     * Returns a reference to the first element in the container. If the vector
     * is empty, an exception of type ContainerException is thrown.
     */
    reference front() {
        auto &v = get_data().get_mut_element_only();
        if (v.empty()) {
            QL_CONTAINER_ERROR("front() called on empty vector");
        }
        return v.front();
    }

    /**
     * Returns a reference to the first element in the container. If the vector
     * is empty, an exception of type ContainerException is thrown.
     */
    const_reference front() const {
        auto &v = get_data().get_const();
        if (v.empty()) {
            QL_CONTAINER_ERROR("front() called on empty vector");
        }
        return v.front();
    }

    /**
     * Returns a reference to the last element in the container. If the vector
     * is empty, an exception of type ContainerException is thrown.
     */
    reference back() {
        auto &v = get_data().get_mut_element_only();
        if (v.empty()) {
            QL_CONTAINER_ERROR("back() called on empty vector");
        }
        return v.back();
    }

    /**
     * Returns a reference to the last element in the container. If the vector
     * is empty, an exception of type ContainerException is thrown.
     */
    const_reference back() const {
        auto &v = get_data().get_const();
        if (v.empty()) {
            QL_CONTAINER_ERROR("back() called on empty vector");
        }
        return v.back();
    }

    /**
     * Returns pointer to the underlying array serving as element storage. The
     * pointer is such that range [data(); data() + size()) is always a valid
     * range, even if the container is empty (data() is not dereferenceable in
     * that case).
     */
    T *data() {
        return get_data().get_mut_element_only().data();
    }

    /**
     * Returns pointer to the underlying array serving as element storage. The
     * pointer is such that range [data(); data() + size()) is always a valid
     * range, even if the container is empty (data() is not dereferenceable in
     * that case).
     */
    const T *data() const {
        return get_data().get_const().data();
    }

    /**
     * Returns an iterator to the first element of the vector. If the vector is
     * empty, the returned iterator will be equal to end().
     */
    Iter begin() {
        return Iter(get_data().get_mut_element_only().begin(), data_ptr);
    }

    /**
     * Returns an iterator to the first element of the vector. If the vector is
     * empty, the returned iterator will be equal to end().
     */
    ConstIter begin() const {
        return ConstIter(get_data().get_const().cbegin(), data_ptr);
    }

    /**
     * Returns an iterator to the first element of the vector. If the vector is
     * empty, the returned iterator will be equal to end().
     */
    ConstIter cbegin() const {
        return ConstIter(get_data().get_const().cbegin(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the
     * vector. This element acts as a placeholder; attempting to access it
     * results in an exception.
     */
    Iter end() {
        return Iter(get_data().get_mut_element_only().end(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the
     * vector. This element acts as a placeholder; attempting to access it
     * results in an exception.
     */
    ConstIter end() const {
        return ConstIter(get_data().get_const().cend(), data_ptr);
    }

    /**
     * Returns an iterator to the element following the last element of the
     * vector. This element acts as a placeholder; attempting to access it
     * results in an exception.
     */
    ConstIter cend() const {
        return ConstIter(get_data().get_const().cend(), data_ptr);
    }

    /**
     * Returns a reverse iterator to the first element of the reversed vector.
     * It corresponds to the last element of the non-reversed vector. If the
     * vector is empty, the returned iterator is equal to rend().
     */
    ReverseIter rbegin() {
        return ReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the first element of the reversed vector.
     * It corresponds to the last element of the non-reversed vector. If the
     * vector is empty, the returned iterator is equal to rend().
     */
    ConstReverseIter rbegin() const {
        return ConstReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the first element of the reversed vector.
     * It corresponds to the last element of the non-reversed vector. If the
     * vector is empty, the returned iterator is equal to rend().
     */
    ConstReverseIter crbegin() const {
        return ConstReverseIter(end());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed vector. It corresponds to the element preceding the first
     * element of the non-reversed vector. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ReverseIter rend() {
        return ReverseIter(begin());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed vector. It corresponds to the element preceding the first
     * element of the non-reversed vector. This element acts as a placeholder;
     * attempting to access it results in an exception.
     */
    ConstReverseIter rend() const {
        return ConstReverseIter(begin());
    }

    /**
     * Returns a reverse iterator to the element following the last element of
     * the reversed vector. It corresponds to the element preceding the first
     * element of the non-reversed vector. This element acts as a placeholder;
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
     * Increase the capacity of the vector to a value that's greater or equal to
     * new_cap. If new_cap is greater than the current capacity(), new storage
     * is allocated, otherwise the method does nothing.
     *
     * reserve() does not change the size of the vector.
     *
     * All iterators and references are invalidated.
     */
    void reserve(size_type new_cap) {
        get_data().get_mut().reserve(new_cap);
    }

    /**
     * Returns the number of elements that the container has currently allocated
     * space for.
     */
    size_type capacity() const {
        return get_data().get_const().capacity();
    }

    /**
     * Requests the removal of unused capacity.
     *
     * It is a non-binding request to reduce capacity() to size(). It depends
     * on the implementation whether the request is fulfilled.
     *
     * All iterators and references are invalidated.
     */
    void shrink_to_fit() {
        get_data().get_mut().shrink_to_fit();
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
     * ContainerException is thrown if first and last belong to this vector.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const Iter &first, const Iter &last) {
        pos.check(data_ptr);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR("inserting from same vector");
        }
        return Iter(get_data().get_mut().insert(pos.iter, first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last) before pos. A
     * ContainerException is thrown if first and last belong to this vector.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const ConstIter &first, const ConstIter &last) {
        pos.check(data_ptr);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR("inserting from same vector");
        }
        return Iter(get_data().get_mut().insert(pos.iter, first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last) before pos. A
     * ContainerException is thrown if first and last belong to this vector.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const ReverseIter &first, const ReverseIter &last) {
        pos.check(data_ptr);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR("inserting from same vector");
        }
        return Iter(get_data().get_mut().insert(pos.iter, first, last), data_ptr);
    }

    /**
     * Inserts elements from range [first, last) before pos. A
     * ContainerException is thrown if first and last belong to this vector.
     *
     * Causes reallocation if the new size() is greater than the old capacity().
     * All iterators and references are invalidated.
     */
    Iter insert(const ConstIter &pos, const ConstReverseIter &first, const ConstReverseIter &last) {
        pos.check(data_ptr);
        if (first.data_ptr == data_ptr) {
            QL_CONTAINER_ERROR("inserting from same vector");
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
     * All iterators and references are invalidated.
     */
    void push_back(const T &value) {
        get_data().get_mut().push_back(value);
    }

    /**
     * Appends the given element value to the end of the container. value is
     * moved into the new element.
     *
     * All iterators and references are invalidated.
     */
    void push_back(T &&value) {
        get_data().get_mut().push_back(std::move(value));
    }

    /**
     * Appends a new element to the end of the container. The element is
     * constructed through std::allocator_traits::construct, which typically
     * uses placement-new to construct the element in-place at the location
     * provided by the container. The arguments args... are forwarded to the
     * constructor as std::forward<Args>(args)....
     *
     * All iterators and references are invalidated.
     */
    template <class... Args>
    void emplace_back(Args&&... args) {
        get_data().get_mut().emplace_back(std::forward<Args>(args)...);
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
            QL_CONTAINER_ERROR("pop_back() called on empty vector");
        }
        v.pop_back();
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
    void swap(CheckedVec &other) {
        std::swap(data_ptr, other.data_ptr);
    }

    /**
     * Checks if the contents of lhs and rhs are equal, that is, they have the
     * same number of elements and each element in lhs compares equal with the
     * element in rhs at the same position.
     */
    friend bool operator==(const CheckedVec &lhs, const CheckedVec &rhs) {
        return lhs.get_data().get_const() == rhs.get_data().get_const();
    }

    /**
     * Checks if the contents of lhs and rhs are equal, that is, they have the
     * same number of elements and each element in lhs compares equal with the
     * element in rhs at the same position.
     */
    friend bool operator!=(const CheckedVec &lhs, const CheckedVec &rhs) {
        return lhs.get_data().get_const() != rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator>(const CheckedVec &lhs, const CheckedVec &rhs) {
        return lhs.get_data().get_const() > rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator>=(const CheckedVec &lhs, const CheckedVec &rhs) {
        return lhs.get_data().get_const() >= rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator<(const CheckedVec &lhs, const CheckedVec &rhs) {
        return lhs.get_data().get_const() < rhs.get_data().get_const();
    }

    /**
     * Compares the contents of lhs and rhs lexicographically. The comparison is
     * performed by a function equivalent to std::lexicographical_compare.
     */
    friend bool operator<=(const CheckedVec &lhs, const CheckedVec &rhs) {
        return lhs.get_data().get_const() <= rhs.get_data().get_const();
    }

};

/**
 * Stream << overload for CheckedVec<>.
 */
template <class T>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::CheckedVec<T> &vec) {
    os << vec.to_string();
    return os;
}

template <typename T, typename Allocator = std::allocator<T>>
#ifdef QL_CHECKED_VEC
using Vec = CheckedVec<T, Allocator>;
#else
using Vec = UncheckedVec<T, Allocator>;
#endif

} // namespace utils
} // namespace ql
