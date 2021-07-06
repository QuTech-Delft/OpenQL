/** \file
 * Defines basic primitive types used within the IR.
 */

#include "ql/ir/prim.h"

#include "ql/arch/architecture.h"
#include "ql/arch/factory.h"
#include "ql/rmgr/manager.h"

namespace ql {
namespace ir {
namespace prim {

//==============================================================================
// Str
//==============================================================================

template <>
Str initialize<Str>() { return ""; }

template <>
void serialize(const Str &obj, utils::tree::cbor::MapWriter &map) {
    map.append_binary("x", obj);
}

template <>
Str deserialize(const utils::tree::cbor::MapReader &map) {
    return map.at("x").as_binary();
}

//==============================================================================
// Json
//==============================================================================

/**
 * Builds a JSON data structure.
 */
Json::Json(const utils::Json &data) : data(data) {
}

/**
 * Dereference operator.
 */
const utils::Json &Json::operator*() const {
    return data;
}

/**
 * Dereference operator.
 */
utils::Json &Json::operator*() {
    return data;
}

/**
 * Dereference operator.
 */
const utils::Json *Json::operator->() const {
    return &data;
}

/**
 * Dereference operator.
 */
utils::Json *Json::operator->() {
    return &data;
}

/**
 * Equality operator.
 */
utils::Bool Json::operator==(const Json &rhs) const {
    return data == rhs.data;
}

/**
 * Inequality operator.
 */
utils::Bool Json::operator!=(const Json &rhs) const {
    return data != rhs.data;
}

template <>
Json initialize<Json>() { return "{}"_json; }

template <>
void serialize(const Json &obj, utils::tree::cbor::MapWriter &map) {
    map.append_binary("x", obj->dump());
}

template <>
Json deserialize(const utils::tree::cbor::MapReader &map) {
    return utils::parse_json(map.at("x").as_binary());
}

std::ostream &operator<<(std::ostream &os, const Json &json) {
    return os << json->dump(2);
}

//==============================================================================
// Bool
//==============================================================================

template <>
Bool initialize<Bool>() { return false; }

template <>
void serialize(const Bool &obj, utils::tree::cbor::MapWriter &map) {
    map.append_bool("x", obj);
}

template <>
Bool deserialize(const utils::tree::cbor::MapReader &map) {
    return map.at("x").as_bool();
}

//==============================================================================
// Int
//==============================================================================

template <>
Int initialize<Int>() { return 0; }

template <>
void serialize(const Int &obj, utils::tree::cbor::MapWriter &map) {
    map.append_int("x", obj);
}

template <>
Int deserialize(const utils::tree::cbor::MapReader &map) {
    return map.at("x").as_int();
}

//==============================================================================
// UInt
//==============================================================================

template <>
UInt initialize<UInt>() { return 0; }

template <>
void serialize(const UInt &obj, utils::tree::cbor::MapWriter &map) {
    map.append_int("x", (Int)obj);
}

template <>
UInt deserialize(const utils::tree::cbor::MapReader &map) {
    return (UInt)map.at("x").as_int();
}

//==============================================================================
// UIntVec
//==============================================================================

template <>
void serialize(const UIntVec &obj, utils::tree::cbor::MapWriter &map) {
    auto aw = map.append_array("v");
    for (const auto &value : obj) {
        aw.append_int((utils::Int)value);
    }
    aw.close();
}

template <>
UIntVec deserialize(const utils::tree::cbor::MapReader &map) {
    auto ar = map.at("v").as_array();
    utils::Vec<utils::UInt> data;
    data.reserve(ar.size());
    for (size_t i = 0; i < ar.size(); i++) {
        data[i] = (utils::UInt)ar.at(i).as_int();
    }
    return data;
}

//==============================================================================
// Real
//==============================================================================

template <>
Real initialize<Real>() { return 0.0; }

template <>
void serialize(const Real &obj, utils::tree::cbor::MapWriter &map) {
    map.append_float("x", obj);
}

template <>
Real deserialize(const utils::tree::cbor::MapReader &map) {
    return map.at("x").as_float();
}

//==============================================================================
// Complex
//==============================================================================

template <>
void serialize(const Complex &obj, utils::tree::cbor::MapWriter &map) {
    map.append_float("r", obj.real());
    map.append_float("i", obj.imag());
}

template <>
Complex deserialize(const utils::tree::cbor::MapReader &map) {
    return {map.at("r").as_float(), map.at("i").as_float()};
}

//==============================================================================
// RMatrix
//==============================================================================

template <>
void serialize(const RMatrix &obj, utils::tree::cbor::MapWriter &map) {
    map.append_int("c", obj.size_cols());
    auto aw = map.append_array("d");
    for (const auto &value : obj.get_data()) {
        aw.append_float(value);
    }
    aw.close();
}

template <>
RMatrix deserialize(const utils::tree::cbor::MapReader &map) {
    size_t num_cols = map.at("c").as_int();
    auto ar = map.at("d").as_array();
    utils::Vec<Real> data;
    data.reserve(ar.size());
    for (size_t i = 0; i < ar.size(); i++) {
        data[i] = ar.at(i).as_float();
    }
    return {data, num_cols};
}

//==============================================================================
// CMatrix
//==============================================================================

template <>
void serialize(const CMatrix &obj, utils::tree::cbor::MapWriter &map) {
    map.append_int("c", obj.size_cols());
    auto aw = map.append_array("d");
    for (const auto &value : obj.get_data()) {
        aw.append_float(value.real());
        aw.append_float(value.imag());
    }
    aw.close();
}

template <>
CMatrix deserialize(const utils::tree::cbor::MapReader &map) {
    size_t num_cols = map.at("c").as_int();
    auto ar = map.at("d").as_array();
    utils::Vec<Complex> data;
    data.reserve(ar.size() / 2);
    for (size_t i = 0; i < ar.size() / 2; i++) {
        data[i] = {ar.at(i*2).as_float(), ar.at(i*2+1).as_float()};
    }
    return {data, num_cols};
}

//==============================================================================
// AccessMode
//==============================================================================

template <>
OperandMode initialize<OperandMode>() { return OperandMode::WRITE; }

template <>
void serialize(const OperandMode &obj, utils::tree::cbor::MapWriter &map) {
    map.append_int("x", (utils::Int)obj);
}

template <>
OperandMode deserialize(const utils::tree::cbor::MapReader &map) {
    return (OperandMode)map.at("x").as_int();
}

std::ostream &operator<<(std::ostream &os, const OperandMode &am) {
    switch (am) {
        case OperandMode::WRITE:     return os << "write";
        case OperandMode::READ:      return os << "read";
        case OperandMode::LITERAL:   return os << "literal";
        case OperandMode::COMMUTE_X: return os << "commute-X";
        case OperandMode::COMMUTE_Y: return os << "commute-Y";
        case OperandMode::COMMUTE_Z: return os << "commute-Z";
        case OperandMode::MEASURE:   return os << "measure";
        case OperandMode::IGNORE:    return os << "ignore";
    }
    return os << "unknown";
}

//==============================================================================
// Topology
//==============================================================================

template <>
void serialize(const Topology &obj, utils::tree::cbor::MapWriter &map) {
    map.append_int("n", obj->get_num_qubits());
    map.append_binary("j", obj->get_json().dump());
}

template <>
Topology deserialize(const utils::tree::cbor::MapReader &map) {
    com::CTopologyRef top;
    top.emplace(
        map.at("n").as_int(),
        utils::parse_json(map.at("j").as_binary())
    );
    Topology wrap;
    wrap.populate(top);
    return wrap;
}

std::ostream &operator<<(std::ostream &os, const Topology &top) {
    if (top.is_populated()) {
        top->dump(os);
    } else {
        os << "<EMPTY>";
    }
    return os;
}

//==============================================================================
// Architecture
//==============================================================================

template <>
void serialize(const Architecture &obj, utils::tree::cbor::MapWriter &map) {
    map.append_string("n", obj->family->get_namespace_name());
    map.append_string("v", obj->variant);
}

template <>
Architecture deserialize(const utils::tree::cbor::MapReader &map) {
    Architecture wrap;
    wrap.populate(arch::Factory().build_from_namespace(
        map.at("n").as_string() + "." + map.at("v").as_string())
    );
    return wrap;
}

std::ostream &operator<<(std::ostream &os, const Architecture &arch) {
    if (arch.is_populated()) {
        os << arch->get_friendly_name();
    } else {
        os << "<EMPTY>";
    }
    return os;
}

//==============================================================================
// ResourceManager
//==============================================================================

template <>
void serialize(const ResourceManager &obj, utils::tree::cbor::MapWriter &map) {
}

template <>
ResourceManager deserialize(const utils::tree::cbor::MapReader &map) {
    return ResourceManager();
}

std::ostream &operator<<(std::ostream &os, const ResourceManager &rm) {
    if (rm.is_populated()) {
        rm->dump_config(os);
    } else {
        os << "<EMPTY>";
    }
    return os;
}

} // namespace prim
} // namespace ir
} // namespace ql
