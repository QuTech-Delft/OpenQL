/** \file
 * Provides a class for representing shared pointers/references to possibly
 * polymorphic objects. So, like std::shared_ptr (but with added safety) or
 * tree::One/Maybe (but without requiring tree methods on the object type).
 */

#pragma once

#include <memory>
#include <type_traits>
#include <functional>
#include "ql/utils/logger.h"
#include "ql/utils/exception.h"

namespace ql {
namespace utils {

template<typename T, template<typename> class TT>
struct is_instantiation_of : std::false_type { };

template<typename T, template<typename> class TT>
struct is_instantiation_of<TT<T>, TT> : std::true_type { };

/**
 * Represents shared pointers/references to possibly polymorphic objects. So,
 * like std::shared_ptr (but with added safety) or tree::One/Maybe (but without
 * requiring tree methods on the object type).
 */
template <class T>
class Ptr {
public:
    using Data = T;

private:

    /**
     * The contained value, wrapped in a shared_ptr.
     */
    std::shared_ptr<T> v{};

public:

    /**
     * Initialization method, used to fill an empty container after
     * construction. Will override any previous value.
     */
    template<typename S = T, class... Args>
    void emplace(Args&&... args) {
        v = std::static_pointer_cast<T>(std::make_shared<S>(std::forward<Args>(args)...));
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
    template<
        class Arg1, class... Args,
        typename = typename std::enable_if<!std::is_same<
            typename std::remove_reference<Arg1>::type,
            Ptr<T>
        >::value>::type>
    Ptr(Arg1 &&arg1, Args&&... args) {
        emplace(std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    /**
     * Builder for Ptr objects.
     */
    template<class... Args>
    static Ptr make(Args&&... args) {
        Ptr ret;
        ret.emplace(std::forward<Args>(args)...);
        return ret;
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
    Ptr(Ptr &&o) noexcept {
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
     * Dereference operator.
     */
    T &operator*() const {
        if (!v) {
            throw Exception("attempt to dereference empty Ptr");
        }
        return *v;
    }

    /**
     * Dereference operator.
     */
    T *operator->() const {
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

    /**
     * Pointer-based equality operator.
     */
    utils::Bool operator==(const Ptr &rhs) const {
        return v == rhs.v;
    }

    /**
     * Pointer-based inequality operator.
     */
    utils::Bool operator!=(const Ptr &rhs) const {
        return v != rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator<(const Ptr &rhs) const {
        return v < rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator<=(const Ptr &rhs) const {
        return v <= rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator>(const Ptr &rhs) const {
        return v > rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator>=(const Ptr &rhs) const {
        return v >= rhs.v;
    }

};

/**
 * Specialization of Ptr that adds a clone() function. The contained type must
 * be copy-constructable and complete for this to work.
 */
template <class T>
class CloneablePtr {
public:
    using Data = T;
    using MutData = typename std::remove_const<T>::type;
    using ConstData = typename std::add_const<T>::type;
    using Copier = std::function<std::shared_ptr<MutData>(ConstData&)>;

private:

    /**
     * The contained value, wrapped in a shared_ptr.
     */
    std::shared_ptr<T> v{};

    /**
     * The copy function associated with the pointer, used to implement clone().
     */
    Copier copier;

public:

    /**
     * Returns a copy of the pointer to the copier function.
     */
    Copier get_copier() const {
        return copier;
    }

    /**
     * Initialization method, used to fill an empty container after
     * construction. Will override any previous value.
     */
    template<typename S = T, class... Args>
    void emplace(Args&&... args) {
        v = std::static_pointer_cast<T>(std::make_shared<S>(std::forward<Args>(args)...));
        copier = [](ConstData &src){
            return std::static_pointer_cast<MutData>(std::make_shared<S>(dynamic_cast<const S&>(src)));
        };
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
    CloneablePtr() = default;

    /**
     * Constructor for a filled container.
     *
     * This only works when there is at least one argument for the constructor
     * of the contained object, and the object is not polymorphic. You'll have
     * to use emplace() otherwise.
     */
    template<
        class Arg1, class... Args,
        typename = typename std::enable_if<!std::is_same<
            typename std::remove_reference<Arg1>::type,
            CloneablePtr<T>
        >::value>::type>
    CloneablePtr(Arg1 &&arg1, Args&&... args) {
        emplace(std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    /**
     * Builder for Ptr objects.
     */
    template<class... Args>
    static CloneablePtr make(Args&&... args) {
        CloneablePtr ret;
        ret.emplace(std::forward<Args>(args)...);
        return ret;
    }

    /**
     * Copy constructor. Only the pointer is moved, so the two Ptrs will
     * refer to the same object.
     */
    CloneablePtr(const CloneablePtr &o) : v(o.v), copier(o.copier) {
    }

    /**
     * Move constructor. Just moves the pointer to the contained object.
     */
    CloneablePtr(CloneablePtr &&o) noexcept {
        v.swap(o.v);
        copier.swap(o.copier);
    }

    /**
     * Copy or move assignment from a contained object. Defers to the copy or
     * move constructor of the contained object type.
     */
    template<typename S = T, typename = typename std::enable_if<std::is_base_of<T, S>::value>::type>
    CloneablePtr &operator=(S &&rhs) {
        emplace<S>(std::forward<S>(rhs));
        return *this;
    }

    /**
     * Copy assignment. Copies the pointer, so both Ptrs will point to the same
     * object.
     */
    template<typename S = T, typename = typename std::enable_if<std::is_base_of<T, S>::value>::type>
    CloneablePtr &operator=(const CloneablePtr<S> &rhs) {
        v = rhs.unwrap();
        auto rhs_copier = rhs.get_copier();
        copier = [rhs_copier](ConstData &src){
            return std::static_pointer_cast<MutData>(rhs_copier(src));
        };
        return *this;
    }

    /**
     * Copy assignment. Copies the pointer, so both Ptrs will point to the same
     * object.
     */
    CloneablePtr &operator=(const CloneablePtr &rhs) {
        v = rhs.v;
        copier = rhs.copier;
        return *this;
    }

    /**
     * Move assignment.
     */
    CloneablePtr &operator=(CloneablePtr &&rhs) noexcept {
        v.swap(rhs.v);
        copier.swap(rhs.copier);
        return *this;
    }

    /**
     * Clone function. Returns a new pointer that points to a copy of the
     * contained object. If no object is contained, a new empty pointer is
     * returned.
     */
    CloneablePtr<MutData> clone() const {
        CloneablePtr<MutData> retval;
        if (v) {
            QL_ASSERT(copier);
            retval.v = copier(*v);
            retval.copier = copier;
        }
        return retval;
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
     * Returns the raw shared_ptr. Note that the pointer cannot be assigned,
     * because this might break the copier function. Instead, set_raw may be
     * used to do this.
     */
    const std::shared_ptr<T> &unwrap() const {
        return v;
    }

    /**
     * Updates the contained shared_ptr and associated copier. Things will break
     * if these don't match up. In general you shouldn't be using it directly
     * (but some other methods that convert between template variants need it,
     * so it must be public).
     */
    void set_raw(const std::shared_ptr<T> new_v, Copier new_copier) {
        v = new_v;
        copier = new_copier;
    }

    /**
     * Returns whether this ClonablePtr points to a value of the given type.
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
    CloneablePtr<S> try_as() const noexcept {
        CloneablePtr<S> result;
        auto c = copier;
        result.set_raw(
            std::dynamic_pointer_cast<S>(v),
            [c](typename CloneablePtr<S>::ConstData &src) {
                return std::dynamic_pointer_cast<CloneablePtr<S>::ConstData>(c(src));
            }
        );
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
     * Casts to a const ClonablePtr.
     */
    CloneablePtr<ConstData> as_const() const noexcept {
        auto result = CloneablePtr<ConstData>();
        result.set_raw(
            std::const_pointer_cast<ConstData>(v),
            copier
        );
        return result;
    }

    /**
     * Dereference operator.
     */
    T &operator*() const {
        if (!v) {
            throw Exception("attempt to dereference empty Ptr");
        }
        return *v;
    }

    /**
     * Dereference operator.
     */
    T *operator->() const {
        if (!v) {
            throw Exception("attempt to dereference empty Ptr");
        }
        return v.get();
    }

    /**
     * Stream overload for pointers.
     */
    friend std::ostream &operator<<(std::ostream &os, const CloneablePtr &ptr) {
        if (ptr.v) {
            os << *ptr.v;
        } else {
            os << "<NULL>";
        }
        return os;
    }

    /**
     * Pointer-based equality operator.
     */
    utils::Bool operator==(const CloneablePtr &rhs) const {
        return v == rhs.v;
    }

    /**
     * Pointer-based inequality operator.
     */
    utils::Bool operator!=(const CloneablePtr &rhs) const {
        return v != rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator<(const CloneablePtr &rhs) const {
        return v < rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator<=(const CloneablePtr &rhs) const {
        return v <= rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator>(const CloneablePtr &rhs) const {
        return v > rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator>=(const CloneablePtr &rhs) const {
        return v >= rhs.v;
    }

};

/**
 * Represents raw pointers/references to possibly polymorphic objects. It
 * emulates Ptr's interface (though it lacks anything that would construct a
 * copy of the contained value), but internally it's a raw pointer with no
 * ownership. That means it can do and does a null check whenever it's
 * dereferenced, and always initializes to nullptr (unlike an actual raw
 * pointer, which gets an undefined value), but cannot guard against dangling
 * pointers if someone else deletes it before or after the wrapper is given this
 * pointer. Therefore, use Ptr unless this really isn't possible.
 */
template <class T>
class RawPtr {
private:

    /**
     * The contained value.
     */
    T *v = nullptr;

public:

    /**
     * Drops the contained object, if any.
     */
    void reset() {
        v = nullptr;
    }

    /**
     * Constructor for an empty container.
     */
    RawPtr() = default;

    /**
     * Constructor from a raw pointer.
     */
    RawPtr(T *v) : v(v) {
    }

    /**
     * Copy constructor. Only the pointer is moved, so the two Ptrs will
     * refer to the same object.
     */
    RawPtr(const RawPtr &o) : v(o.v) {
    }

    /**
     * Move constructor. Just moves the pointer to the contained object.
     */
    RawPtr(RawPtr &&o) noexcept : v(o.v) {
    }

    /**
     * Assignment from a raw pointer.
     */
    RawPtr &operator=(T *rhs) {
        v = rhs;
        return *this;
    }

    /**
     * Copy assignment. Copies the pointer, so both RawPtrs will point to the
     * same object.
     */
    template<typename S = T, typename = typename std::enable_if<std::is_base_of<T, S>::value>::type>
    RawPtr &operator=(const RawPtr<S> &rhs) {
        v = rhs.unwrap();
        return *this;
    }

    /**
     * Copy assignment. Copies the pointer, so both RawPtrs will point to the
     * same object.
     */
    RawPtr &operator=(const RawPtr &rhs) {
        v = rhs.v;
        return *this;
    }

    /**
     * Move assignment.
     */
    RawPtr &operator=(RawPtr &&rhs) noexcept {
        v = rhs.v;
        return *this;
    }

    /**
     * Returns whether this container is filled.
     */
    bool has_value() const noexcept {
        return v != nullptr;
    }

    /**
     * Returns whether this container is filled.
     */
    explicit operator bool() const noexcept {
        return v != nullptr;
    }

    /**
     * Returns the raw pointer.
     */
    const T *unwrap() const {
        return v;
    }

    /**
     * Returns the raw pointer.
     */
    T *unwrap() {
        return v;
    }

    /**
     * Returns whether this RawPtr points to a value of the given type.
     */
    template<typename S>
    bool is() const noexcept {
        if (v == nullptr) return false;
        return dynamic_cast<S*>(v) != nullptr;
    }

    /**
     * Casts to a RawPtr of the given type. The resulting RawPtr will be empty
     * if the cast failed.
     */
    template<typename S>
    RawPtr<S> try_as() const noexcept {
        RawPtr<S> result;
        result.unwrap() = dynamic_cast<S*>(v);
        return result;
    }

    /**
     * Casts to a RawPtr of the given type. Throws an exception if the cast
     * failed or this RawPtr is empty.
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
        result.unwrap() = const_cast<const T*>(v);
        return result;
    }

    /**
     * Mutating dereference operator.
     */
    T &operator*() const {
        if (v == nullptr) {
            throw Exception("attempt to dereference empty RawPtr");
        }
        return *v;
    }

    /**
     * Mutating dereference operator.
     */
    T *operator->() const {
        if (v == nullptr) {
            throw Exception("attempt to dereference empty RawPtr");
        }
        return v;
    }

    /**
     * Stream overload for pointers.
     */
    friend std::ostream &operator<<(std::ostream &os, const RawPtr &ptr) {
        if (ptr.v) {
            os << *ptr.v;
        } else {
            os << "<NULL>";
        }
        return os;
    }

    /**
     * Pointer-based equality operator.
     */
    utils::Bool operator==(const RawPtr &rhs) const {
        return v == rhs.v;
    }

    /**
     * Pointer-based inequality operator.
     */
    utils::Bool operator!=(const RawPtr &rhs) const {
        return v != rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator<(const RawPtr &rhs) const {
        return v < rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator<=(const RawPtr &rhs) const {
        return v <= rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator>(const RawPtr &rhs) const {
        return v > rhs.v;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator>=(const RawPtr &rhs) const {
        return v >= rhs.v;
    }

};

} // namespace utils
} // namespace ql
