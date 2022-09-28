/** \file
 * Defines some basic types used by all resources.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/ir/compat/compat.h"
#include "ql/ir/ir.h"
#include "ql/rmgr/declarations.h"

namespace ql {
namespace rmgr {

/**
 * The direction in which gates are presented to a resource, allowing the
 * resource to optimize its state.
 */
enum class Direction {

    /**
     * Gates are only reserved with non-decreasing cycle numbers.
     */
    FORWARD,

    /**
     * Gates are only reserved with non-increasing cycle numbers.
     */
    BACKWARD,

    /**
     * available() and reserve() may be called with any cycle number.
     */
    UNDEFINED

};

/**
 * Stream operator for Direction.
 */
std::ostream &operator<<(std::ostream &os, Direction dir);

/**
 * Context for constructing resource instances.
 */
struct Context {

    /**
     * The full type name for the resource. This is the full name that was used
     * when the resource was registered with the resource factory. The same
     * resource class may be registered with multiple type names, in which case
     * the pass implementation may use this to differentiate.
     */
    utils::Str type_name;

    /**
     * The instance name for the resource, i.e. the name that the user assigned
     * to it or the name that was assigned to it automatically. Must match
     * `[a-zA-Z0-9_\-]+`, and must be unique within a resource manager.
     * Instance names should NOT have a semantic meaning; they are only intended
     * for logging.
     */
    utils::Str instance_name;

    /**
     * The old-IR platform being compiled for. This is currently always valid,
     * regardless of whether the new or old IR is used. However, when the old IR
     * is phased out, it should be removed. The relevant information can then be
     * taken from ir.
     */
    ir::compat::PlatformRef platform;

    /**
     * The root of the new IR tree that's being compiled. This is empty when the
     * old IR is used.
     */
    ir::Ref ir;

    /**
     * Unparsed JSON configuration data for the resource.
     */
    utils::Json configuration;

};

} // namespace rmgr
} // namespacq ql
