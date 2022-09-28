/** \file
 * Forward declarations for the resource factory and manager classes.
 */

#pragma once

#include "ql/utils/ptr.h"

namespace ql {
namespace rmgr {

// Forward declaration for the resource factory and pass manager.
class Factory;
class Manager;
class State;

/**
 * Shared pointer reference to a resource manager.
 */
using Ref = utils::Ptr<Manager>;

/**
 * Immutable shared pointer reference to a resource manager.
 */
using CRef = utils::Ptr<const Manager>;

} // namespace rmgr
} // namespace ql
