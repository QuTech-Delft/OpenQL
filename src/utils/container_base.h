/** \file
 * Provides utilities and base classes for checked containers and iterators.
 */

#pragma once

#include <memory>
#include <iterator>
#include "utils/exception.h"

namespace ql {
namespace utils {

/**
 * The data block for a protected container. This is wrapped in a shared_ptr
 * that's copied for all returned iterators, so the container's data block is
 * not deleted until all its iterators are deleted, too.
 */
template <typename S>
class ContainerData {
public:

    using Stl = S;

private:

    /**
     * The contained, protected container.
     */
    Stl vec;

    /**
     * Current "version" of the container. Whenever the container's
     * structure changes (insertions & deletions), this is incremented.
     * Iterators use this to guard against mutation they weren't aware
     * about.
     */
    size_t version = 0;

public:

    /**
     * Forward all constructors to the container.
     */
    template<typename... Args>
    explicit ContainerData(
        Args&&... args
    ) :
        vec(std::forward<Args>(args)...),
        version(0)
    {}

    /**
     * Initializer lists behave weirdly and need their own overload.
     */
    ContainerData(
        std::initializer_list<typename Stl::value_type> init,
        const typename Stl::allocator_type &alloc = typename Stl::allocator_type()
    ) :
        vec(init, alloc),
        version(0)
    {}

    /**
     * Returns mutable access to the container. Iterators that are not
     * explicitly updated after this lose their validity.
     */
    Stl &get_mut() {
        version++;
        return vec;
    }

    /**
     * Returns mutable access to the container without invalidating
     * iterators. This is just for accessing elements in a mutable way.
     */
    Stl &get_mut_element_only() {
        return vec;
    }

    /**
     * Returns const access to the container.
     */
    const Stl &get_const() const {
        return vec;
    }

    /**
     * Returns the current iterator version.
     */
    size_t get_version() const {
        return version;
    }

};

/**
 * Helper class used to return the begin() and end() corresponding to regular
 * iterators.
 */
class RegularEndpointAdapter {
public:
    template <typename T>
    static typename T::Stl::iterator begin(T &container) {
        return container.get_mut_element_only().begin();
    }
    template <typename T>
    static typename T::Stl::iterator end(T &container) {
        return container.get_mut_element_only().end();
    }
};

/**
 * Helper class used to return the begin() and end() corresponding to const
 * iterators.
 */
class ConstEndpointAdapter {
public:
    template <typename T>
    static typename T::Stl::const_iterator begin(const T &container) {
        return container.get_const().cbegin();
    }
    template <typename T>
    static typename T::Stl::const_iterator end(const T &container) {
        return container.get_const().cend();
    }
};

template <typename T, typename Alloc = std::allocator<T>>
class CheckedVec;
template <typename T, typename Alloc = std::allocator<T>>
class UncheckedVec;
template <typename T, typename Alloc = std::allocator<T>>
class CheckedList;
template <typename T, typename Alloc = std::allocator<T>>
class UncheckedList;
template <
    typename Key,
    typename T,
    typename Compare = std::less<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>
>
class CheckedMap;
template <
    typename Key,
    typename T,
    typename Compare = std::less<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>
>
class UncheckedMap;

/**
 * Wrapper for standard iterators to detect undefined behavior and throw an
 * exception.
 */
template <typename D, typename I, typename EP>
class WrappedIterator {
public:
    using Data = D;
    using Iter = I;
    using EndpointAdapter = EP;
    using iterator_category = typename std::iterator_traits<Iter>::iterator_category;
    using value_type = typename std::iterator_traits<Iter>::value_type;
    using difference_type = typename std::iterator_traits<Iter>::difference_type;
    using pointer = typename std::iterator_traits<Iter>::pointer;
    using reference = typename std::iterator_traits<Iter>::reference;
    using size_type = typename Data::Stl::size_type;

private:

    // All types of WrappedIterators are friends.
    template <typename D2, typename I2, typename EP2>
    friend class WrappedIterator;

    // Containers must be able to use the private methods of this class.
    template <typename T, typename Alloc>
    friend class CheckedVec;
    template <typename T, typename Alloc>
    friend class CheckedList;
    template <typename Key, typename T, typename Compare, typename Allocator>
    friend class CheckedMap;

    /**
     * The wrapped iterator.
     */
    Iter iter;

    /**
     * Pointer to the data block. Prevents the data block from being deleted
     * while the iterator is live, and is also used to check whether the
     * iterator actually belongs to the container when it's used on one.
     */
    std::shared_ptr<Data> data_ptr;

    /**
     * Data block version that this iterator is valid for.
     */
    size_t version;

    /**
     * Checks the validity of this iterator.
     */
    Data &check() const {
        if (!this->data_ptr) {
            throw ContainerException("iterator does not belong to any container");
        }
        if (version != this->data_ptr->get_version()) {
            throw ContainerException("using outdated iterator");
        }
        return *data_ptr;
    }

    /**
     * Checks the validity of this iterator in the context of the given
     * container data block.
     */
    Data &check(const std::shared_ptr<Data> &data) const {
        if (data != this->data_ptr) {
            throw ContainerException("iterator belongs to different container");
        }
        return check();
    }

    /**
     * Checks whether this iterator and the provided one make sense within the
     * same context.
     */
    Data &check(const WrappedIterator &other) const {
        auto &res = check();
        other.check();
        if (other.data_ptr != this->data_ptr) {
            throw ContainerException("using unrelated iterators in a single context");
        }
        return res;
    }

    /**
     * Updates this iterator's version number. Does not check data block
     * validity; must only be used when the data block is known to exist.
     */
    void update() {
        version = data_ptr->get_version();
    }

    /**
     * Wraps a raw iterator.
     */
    explicit WrappedIterator(
        Iter &&iter,
        const std::shared_ptr<Data> &data
    ) :
        iter(std::forward<Iter>(iter)),
        data_ptr(data)
    {
        if (!data) {
            throw ContainerException("container is used after move");
        }
        update();
    }

public:

    /**
     * Default constructor, because iterators must be default-constructable;
     * nevertheless, such an iterator is not usable in any context.
     */
    WrappedIterator() : iter(), data_ptr(), version(0) {}

    /**
     * Conversion copy constructor from a compatible iterator.
     */
    template <
        typename D2, typename I2, typename EP2,
        typename = typename std::enable_if<
            std::is_convertible<D2, Data>::value
            && std::is_convertible<I2, Iter>::value
        >::type
    >
    WrappedIterator(
        const WrappedIterator<D2, I2, EP2> &src
    ) :
        iter(src.iter),
        data_ptr(src.data_ptr),
        version(src.version)
    {}

    /**
     * Conversion move constructor from a compatible iterator.
     */
    template <
        typename D2, typename I2, typename EP2,
        typename = typename std::enable_if<
            std::is_convertible<D2, Data>::value
            && std::is_convertible<I2, Iter>::value
        >::type
    >
    WrappedIterator(
        WrappedIterator<D2, I2, EP2> &&src
    ) :
        iter(src.iter),
        data_ptr(src.data_ptr),
        version(src.version)
    {}

    /**
     * Copy constructor.
     */
    WrappedIterator(const WrappedIterator &src) = default;

    /**
     * Move constructor.
     */
    WrappedIterator(WrappedIterator &&src) noexcept = default;

    /**
     * Copy assignment.
     */
    WrappedIterator &operator=(const WrappedIterator &src) = default;

    /**
     * Move assignment.
     */
    WrappedIterator &operator=(WrappedIterator &&src) noexcept = default;

    /**
     * Prefix increment operator.
     */
    WrappedIterator &operator++() {
        auto &d = check();
        if (iter == EndpointAdapter::end(d)) {
            throw ContainerException("moving iterator past end of container");
        }
        ++iter;
        return *this;
    }

    /**
     * Postfix increment operator.
     */
    WrappedIterator operator++(int dummy) {
        (void)dummy;
        WrappedIterator copy = *this;
        this->operator++();
        return copy;
    }

    /**
     * Prefix increment operator.
     */
    WrappedIterator &operator--() {
        auto &d = check();
        if (iter == EndpointAdapter::begin(d)) {
            throw ContainerException("moving iterator past beginning of container");
        }
        --iter;
        return *this;
    }

    /**
     * Postfix increment operator.
     */
    WrappedIterator operator--(int dummy) {
        (void)dummy;
        WrappedIterator copy = *this;
        this->operator--();
        return copy;
    }

    /**
     * Advances the iterator by the given delta.
     */
    WrappedIterator &operator+=(difference_type rhs) {
        auto &d = check();
        if (rhs > EndpointAdapter::end(d) - iter) {
            throw ContainerException("moving iterator past end of container");
        }
        if (rhs < EndpointAdapter::begin(d) - iter) {
            throw ContainerException("moving iterator past begin of container");
        }
        iter += rhs;
        return *this;
    }

    /**
     * Advances the iterator by the given delta.
     */
    friend WrappedIterator operator+(const WrappedIterator &lhs, difference_type rhs) {
        WrappedIterator retval{lhs};
        retval += rhs;
        return retval;
    }

    /**
     * Advances the iterator by the given delta.
     */
    friend WrappedIterator operator+(difference_type lhs, const WrappedIterator &rhs) {
        return rhs + lhs;
    }

    /**
     * Rewinds the iterator by the given delta.
     */
    WrappedIterator &operator-=(difference_type rhs) {
        return this->operator+=(-rhs);
    }

    /**
     * Rewinds the iterator by the given delta.
     */
    friend WrappedIterator operator-(const WrappedIterator &lhs, difference_type rhs) {
        WrappedIterator retval = lhs;
        retval -= rhs;
        return retval;
    }

    /**
     * Returns the distance between two iterators.
     */
    friend typename Iter::difference_type operator-(WrappedIterator &&lhs, WrappedIterator &&rhs) {
        lhs.check(rhs);
        return lhs.iter - rhs.iter;
    }

    /**
     * Returns the distance between two iterators.
     */
    friend typename Iter::difference_type operator-(const WrappedIterator &lhs, const WrappedIterator &rhs) {
        lhs.check(rhs);
        return lhs.iter - rhs.iter;
    }

    /**
     * Dereference operator.
     */
    reference operator*() const {
        auto &d = check();
        if (iter == EndpointAdapter::end(d)) {
            throw ContainerException("dereferencing past-the-end iterator");
        }
        return *iter;
    }

    /**
     * Dereference operator.
     */
    pointer operator->() const {
        auto &d = check();
        if (iter == EndpointAdapter::end(d)) {
            throw ContainerException("dereferencing past-the-end iterator");
        }
        return &(*iter);
    }

    /**
     * Array dereference operator.
     */
    reference operator[](difference_type offset) const {
        auto &d = check();
        if (offset >= EndpointAdapter::end(d) - iter) {
            throw ContainerException("iterator indexing after end of container");
        }
        if (offset < EndpointAdapter::begin(d) - iter) {
            throw ContainerException("iterator indexing before begin of container");
        }
        return iter[offset];
    }

    /**
     * Equality operator.
     */
    friend bool operator==(const WrappedIterator &lhs, const WrappedIterator &rhs) {
        lhs.check(rhs);
        return lhs.iter == rhs.iter;
    }

    /**
     * Inequality operator.
     */
    friend bool operator!=(const WrappedIterator &lhs, const WrappedIterator &rhs) {
        lhs.check(rhs);
        return lhs.iter != rhs.iter;
    }

    /**
     * Greater-than operator.
     */
    friend bool operator>(const WrappedIterator &lhs, const WrappedIterator &rhs) {
        lhs.check(rhs);
        return lhs.iter > rhs.iter;
    }

    /**
     * Greater-than-or-equals operator.
     */
    friend bool operator>=(const WrappedIterator &lhs, const WrappedIterator &rhs) {
        lhs.check(rhs);
        return lhs.iter >= rhs.iter;
    }

    /**
     * Less-than operator.
     */
    friend bool operator<(const WrappedIterator &lhs, const WrappedIterator &rhs) {
        lhs.check(rhs);
        return lhs.iter < rhs.iter;
    }

    /**
     * Less-than-or-equals operator.
     */
    friend bool operator<=(const WrappedIterator &lhs, const WrappedIterator &rhs) {
        lhs.check(rhs);
        return lhs.iter <= rhs.iter;
    }

};

} // namespace utils
} // namespace ql
