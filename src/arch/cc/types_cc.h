/** \file
 * Shorthand/semantical type names for some primitive/utils types.
 *
 * Among other things, this prevents the "namespace { using namespace utils; }"
 * hack, which was causing name conflicts with the new abbreviated class names
 * (now that everything is in ql::arch::cc).
 */

#pragma once

#include "utils/str.h"
#include "utils/json.h"
#include "utils/ptr.h"
#include "utils/vec.h"
#include "utils/map.h"

namespace ql {
namespace arch {
namespace cc {

using Bool = utils::Bool;
using UInt = utils::UInt;
using Int  = utils::Int;
using Real = utils::Real;
using Str  = utils::Str;
using Json = utils::Json;

using Digital   = uint32_t;
using Codeword  = uint32_t;

using StrStrm   = utils::StrStrm;

template <class T>
using RawPtr = utils::RawPtr<T>;

template <class T>
using Ptr = utils::Ptr<T>;

template <class T>
using Vec = utils::Vec<T>;

template <class K, class V>
using Map = utils::Map<K, V>;

} // namespace cc
} // namespace arch
} // namespace ql
