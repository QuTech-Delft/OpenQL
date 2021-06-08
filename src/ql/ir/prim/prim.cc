/** \file
 * Defines basic primitive types used within the IR.
 */

#include "ql/ir/prim/prim.h"

namespace ql {
namespace ir {
namespace prim {

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

template <>
void serialize(const Complex &obj, utils::tree::cbor::MapWriter &map) {
    map.append_float("r", obj.real());
    map.append_float("i", obj.imag());
}

template <>
Complex deserialize(const utils::tree::cbor::MapReader &map) {
    return {map.at("r").as_float(), map.at("i").as_float()};
}

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
    std::vector<Real> data;
    data.reserve(ar.size());
    for (size_t i = 0; i < ar.size(); i++) {
        data[i] = ar.at(i).as_float();
    }
    return {data, num_cols};
}

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
    std::vector<Complex> data;
    data.reserve(ar.size() / 2);
    for (size_t i = 0; i < ar.size() / 2; i++) {
        data[i] = {ar.at(i*2).as_float(), ar.at(i*2+1).as_float()};
    }
    return {data, num_cols};
}

} // namespace prim
} // namespace ir
} // namespace ql
