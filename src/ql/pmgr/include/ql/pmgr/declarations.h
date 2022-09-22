/** \file
 * Forward declarations for the pass factory and manager classes.
 */

#pragma once

#include "ql/utils/ptr.h"

namespace ql {
namespace pmgr {

// Forward declaration for the pass factory and pass manager.
class Factory;
class Manager;

/**
 * Shared pointer reference to a pass manager.
 */
using Ref = utils::Ptr<Manager>;

/**
 * Immutable shared pointer reference to a pass manager.
 */
using CRef = utils::Ptr<const Manager>;

} // namespace pmgr
} // namespace ql
