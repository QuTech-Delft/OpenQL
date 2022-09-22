/** \file
 * Structure for retaining information about a particular variant of an
 * architecture.
 */

#pragma once

#include <iostream>
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"

namespace ql {
namespace arch {

// Forward declarations.
class Factory;
class InfoBase;
class Architecture;

/**
 * Shared pointer reference to an architecture information wrapper.
 */
using InfoRef = utils::Ptr<InfoBase>;

/**
 * Immutable shared pointer reference to an architecture information wrapper.
 */
using CInfoRef = utils::Ptr<const InfoBase>;

/**
 * Shared pointer reference to an architecture variant wrapper.
 */
using ArchitectureRef = utils::Ptr<Architecture>;

/**
 * Immutable shared pointer reference to an architecture variant wrapper.
 */
using CArchitectureRef = utils::Ptr<const Architecture>;

} // namespace arch
} // namespace ql
