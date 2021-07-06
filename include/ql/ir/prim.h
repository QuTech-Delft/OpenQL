/** \file
 * Defines basic primitive types used within the IR.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/json.h"
#include "ql/utils/exception.h"
#include "ql/utils/tree.h"
#include "ql/com/topology.h"
#include "ql/arch/declarations.h"
#include "ql/rmgr/declarations.h"

namespace ql {
namespace ir {
namespace prim {

/**
 * Generates a default value for the given primitive type. This is specialized
 * for the primitives mapping to builtin types (int, bool, etc, for which the
 * "constructor" doesn't initialize the value at all) such that they actually
 * initialize with a sane default. Used in the default constructors of the
 * generated tree nodes to ensure that there's no garbage in the nodes.
 */
template <class T>
T initialize() { return T(); };

/**
 * Serializes the given primitive object to CBOR.
 */
template <typename T>
void serialize(const T &obj, utils::tree::cbor::MapWriter &map);

/**
 * Deserializes the given primitive object from CBOR.
 */
template <typename T>
T deserialize(const utils::tree::cbor::MapReader &map);

/**
 * String primitive used within the trees. Defaults to "".
 */
using Str = utils::Str;
template <>
Str initialize<Str>();
template <>
void serialize(const Str &obj, utils::tree::cbor::MapWriter &map);
template <>
Str deserialize(const utils::tree::cbor::MapReader &map);

/**
 * JSON primitive used within the trees. Defaults to {}. The normal Json class
 * from nlohmann is wrapped so we can use a nicer-formatted string
 * representation for debug dumps.
 */
class Json {
public:

    /**
     * The wrapped JSON data. You can use this or the dereference operator to
     * access it.
     */
    utils::Json data;

    /**
     * Builds a JSON data structure.
     */
    Json(const utils::Json &data);

    /**
     * Dereference operator.
     */
    const utils::Json &operator*() const;

    /**
     * Dereference operator.
     */
    utils::Json &operator*();

    /**
     * Dereference operator.
     */
    const utils::Json *operator->() const;

    /**
     * Dereference operator.
     */
    utils::Json *operator->();

    /**
     * Equality operator.
     */
    utils::Bool operator==(const Json &rhs) const;

    /**
     * Inequality operator.
     */
    utils::Bool operator!=(const Json &rhs) const;

};
template <>
Json initialize<Json>();
template <>
void serialize(const Json &obj, utils::tree::cbor::MapWriter &map);
template <>
Json deserialize(const utils::tree::cbor::MapReader &map);
std::ostream &operator<<(std::ostream &os, const Json &json);

/**
 * Boolean primitive used within the trees. Defaults to false.
 */
using Bool = utils::Bool;
template <>
Bool initialize<Bool>();
template <>
void serialize(const Bool &obj, utils::tree::cbor::MapWriter &map);
template <>
Bool deserialize(const utils::tree::cbor::MapReader &map);

/**
 * Integer primitive used within the trees. Defaults to 0.
 */
using Int = utils::Int;
template <>
Int initialize<Int>();
template <>
void serialize(const Int &obj, utils::tree::cbor::MapWriter &map);
template <>
Int deserialize(const utils::tree::cbor::MapReader &map);

/**
 * Unsigned integer primitive used within the trees. Defaults to 0.
 */
using UInt = utils::UInt;
template <>
UInt initialize<UInt>();
template <>
void serialize(const UInt &obj, utils::tree::cbor::MapWriter &map);
template <>
UInt deserialize(const utils::tree::cbor::MapReader &map);

/**
 * A vector of unsigned integers used within the trees. Defaults to [].
 */
using UIntVec = utils::Vec<utils::UInt>;
template <>
void serialize(const UIntVec &obj, utils::tree::cbor::MapWriter &map);
template <>
UIntVec deserialize(const utils::tree::cbor::MapReader &map);

/**
 * Real number primitive used within the trees. Defaults to 0.0.
 */
using Real = utils::Real;
template <>
Real initialize<Real>();
template <>
void serialize(const Real &obj, utils::tree::cbor::MapWriter &map);
template <>
Real deserialize(const utils::tree::cbor::MapReader &map);

/**
 * Complex number primitive used within the trees. Defaults to 0.0.
 */
using Complex = utils::Complex;

/**
 * Two-dimensional matrix of some kind of type.
 */
template <typename T>
class Matrix {
private:

    /**
     * The contained data, stored row-major.
     */
    utils::Vec<T> data;

    /**
     * The number of rows in the matrix.
     */
    utils::UInt nrows;

    /**
     * The number of columns in the matrix.
     */
    utils::UInt ncols;

public:

    /**
     * Creates an empty matrix.
     */
    Matrix()
        : data(ncols), nrows(1), ncols(0)
    {}

    /**
     * Creates a vector.
     */
    Matrix(utils::UInt ncols)
        : data(ncols), nrows(1), ncols(ncols)
    {}

    /**
     * Creates a zero-initialized matrix of the given size.
     */
    Matrix(utils::UInt nrows, utils::UInt ncols)
        : data(nrows*ncols), nrows(nrows), ncols(ncols)
    {}

    /**
     * Creates a column vector with the given data.
     */
    Matrix(const utils::Vec<T> &data)
        : data(data), nrows(data.size()), ncols(1)
    {}

    /**
     * Creates a matrix with the given data. The number of rows is inferred. If
     * the number of data elements is not divisible by the number of columns, a
     * range error is thrown.
     */
    Matrix(const utils::Vec<T> &data, utils::UInt ncols)
        : data(data), nrows(data.size() / ncols), ncols(ncols)
    {
        if (data.size() % ncols != 0) {
            throw utils::Exception("invalid matrix shape");
        }
    }

    /**
     * Returns the number of rows.
     */
    utils::UInt size_rows() const {
        return nrows;
    }

    /**
     * Returns the number of columns.
     */
    utils::UInt size_cols() const {
        return ncols;
    }

    /**
     * Returns access to the raw data vector.
     */
    const utils::Vec<T> &get_data() const {
        return data;
    }

    /**
     * Returns the value at the given position. row and col start at 1. Throws
     * an Exception when either or both indices are out of range.
     */
    T at(utils::UInt row, utils::UInt col) const {
        if (row < 1 || row > nrows || col < 1 || col > ncols) {
            throw utils::Exception("matrix index out of range");
        }
        return data[(row - 1) * ncols + col - 1];
    }

    /**
     * Returns a mutable reference to the value at the given position. row and
     * col start at 1. Throws an Exception when either or both indices are out
     * of range.
     */
    T &at(utils::UInt row, utils::UInt col) {
        if (row < 1 || row > nrows || col < 1 || col > ncols) {
            throw utils::Exception("matrix index out of range");
        }
        return data[(row - 1) * ncols + col - 1];
    }

    /**
     * Equality operator for matrices.
     */
    utils::Bool operator==(const Matrix<T> &rhs) const {
        return data == rhs.data && nrows == rhs.nrows && ncols == rhs.ncols;
    }

    /**
     * Inequality operator for matrices.
     */
    utils::Bool operator!=(const Matrix<T> &rhs) const {
        return !(*this == rhs);
    }

};

/**
 * Matrix of real numbers.
 */
using RMatrix = Matrix<Real>;
template <>
void serialize(const RMatrix &obj, utils::tree::cbor::MapWriter &map);
template <>
RMatrix deserialize(const utils::tree::cbor::MapReader &map);

/**
 * Matrix of complex numbers.
 */
using CMatrix = Matrix<Complex>;
template <>
void serialize(const CMatrix &obj, utils::tree::cbor::MapWriter &map);
template <>
CMatrix deserialize(const utils::tree::cbor::MapReader &map);

/**
 * Stream << overload for matrix nodes.
 */
template <typename T>
std::ostream &operator<<(std::ostream &os, const Matrix<T> &mat) {
    os << "[";
    for (utils::UInt row = 1; row <= mat.size_rows(); row++) {
        if (row > 1) {
            os << "; ";
        }
        for (size_t col = 1; col <= mat.size_cols(); col++) {
            if (col > 1) {
                os << ", ";
            }
            os << mat.at(row, col);
        }
    }
    os << "]";
    return os;
}

/**
 * Value access mode for an operand.
 */
enum class OperandMode {

    /**
     * Used for classical write or non-commuting qubit access. The corresponding
     * operand must be a reference.
     */
    WRITE,

    /**
     * Used for classical read-only access. Other instructions accessing the
     * same operand with mode READ may commute.
     */
    READ,

    /**
     * Used for classical operands of which the value must be known at
     * compile-time. Only accepts literal values.
     */
    LITERAL,

    /**
     * Used for qubit usage that commutes along the X axis; i.e., other
     * instructions involving the corresponding qubit in mode COMMUTE_X may
     * commute.
     */
    COMMUTE_X,

    /**
     * Used for qubit usage that commutes along the Y axis; i.e., other
     * instructions involving the corresponding qubit in mode COMMUTE_Y may
     * commute.
     */
    COMMUTE_Y,

    /**
     * Used for qubit usage that commutes along the Z axis; i.e., other
     * instructions involving the corresponding qubit in mode COMMUTE_Z may
     * commute.
     */
    COMMUTE_Z,

    /**
     * Used when a qubit is measured and the result is stored in the implicit
     * bit register associated with the qubit.
     */
    MEASURE,

    /**
     * Used for operands which should be ignored by data dependency graph
     * construction, such as the third qubit operand of cz_park.
     */
    IGNORE

};
template <>
OperandMode initialize<OperandMode>();
template <>
void serialize(const OperandMode &obj, utils::tree::cbor::MapWriter &map);
template <>
OperandMode deserialize(const utils::tree::cbor::MapReader &map);
std::ostream &operator<<(std::ostream &os, const OperandMode &am);

/**
 * Wrapper class for primitives.
 */
template <class Ref, class Obj>
class Wrapper {
private:

    /**
     * The wrapped topology node.
     */
    Ref ref;

public:

    /**
     * Populates the topology node.
     */
    void populate(const Ref &new_ref) {
        if (ref.has_value()) {
            throw utils::Exception("attempt to populate non-empty primitive wrapper node");
        }
        ref = new_ref;
    }

    /**
     * Returns whether the node is populated.
     */
    utils::Bool is_populated() const {
        return ref.has_value();
    }

    /**
     * Dereference operator.
     */
    const Obj &operator*() const  {
        if (!ref.has_value()) {
            throw utils::Exception("attempt to dereference empty primitive wrapper node");
        }
        return *ref;
    }

    /**
     * Dereference operator.
     */
    const Obj *operator->() const {
        if (!ref.has_value()) {
            throw utils::Exception("attempt to dereference empty primitive wrapper node");
        }
        return &*ref;
    }

    /**
     * Pointer-based equality operator.
     */
    utils::Bool operator==(const Wrapper &rhs) const {
        return ref == rhs.ref;
    }

    /**
     * Pointer-based inequality operator.
     */
    utils::Bool operator!=(const Wrapper &rhs) const {
        return ref != rhs.ref;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator<(const Wrapper &rhs) const {
        return ref < rhs.ref;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator<=(const Wrapper &rhs) const {
        return ref <= rhs.ref;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator>(const Wrapper &rhs) const {
        return ref > rhs.ref;
    }

    /**
     * Pointer-based comparison operator.
     */
    utils::Bool operator>=(const Wrapper &rhs) const {
        return ref >= rhs.ref;
    }

};

/**
 * Wrapper for a reference to a topology.
 */
using Topology = Wrapper<com::CTopologyRef, com::Topology>;
template <>
void serialize(const Topology &obj, utils::tree::cbor::MapWriter &map);
template <>
Topology deserialize(const utils::tree::cbor::MapReader &map);
std::ostream &operator<<(std::ostream &os, const Topology &top);

/**
 * Wrapper for a reference to an architecture.
 */
using Architecture = Wrapper<arch::CArchitectureRef, arch::Architecture>;
template <>
void serialize(const Architecture &obj, utils::tree::cbor::MapWriter &map);
template <>
Architecture deserialize(const utils::tree::cbor::MapReader &map);
std::ostream &operator<<(std::ostream &os, const Architecture &top);

/**
 * Wrapper for a reference to a resource manager.
 *
 * TODO: serdes is inoperative! If the tree is transferred to Python and back,
 *  the resource manager must be copied from the original tree.
 */
using ResourceManager = Wrapper<rmgr::CRef, rmgr::Manager>;
template <>
void serialize(const ResourceManager &obj, utils::tree::cbor::MapWriter &map);
template <>
ResourceManager deserialize(const utils::tree::cbor::MapReader &map);
std::ostream &operator<<(std::ostream &os, const ResourceManager &top);

} // namespace prim
} // namespace ir
} // namespace ql
