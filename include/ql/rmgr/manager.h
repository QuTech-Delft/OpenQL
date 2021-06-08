/** \file
 * Defines the resource manager.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/utils/map.h"
#include "ql/rmgr/declarations.h"
#include "ql/rmgr/resource_types/base.h"
#include "ql/rmgr/factory.h"
#include "ql/rmgr/state.h"

namespace ql {
namespace rmgr {

/**
 * A collection of resources corresponding to a particular platform.
 */
class Manager {
public:

    /**
     * Dumps the documentation for the resource JSON configuration structure.
     */
    static void dump_docs(std::ostream &os = std::cout, const utils::Str &line_prefix = "");

private:

    /**
     * Factory for constructing resources.
     */
    const Factory factory;

    /**
     * The platform that this resource manager is built for.
     */
    const ir::compat::PlatformRef &platform;

    /**
     * The list of resources.
     */
    utils::Map<utils::Str, ResourceRef> resources;

    /**
     * Returns whether the given user-specified name is a valid resource name.
     */
    void check_resource_name(const utils::Str &name) const;

    /**
     * Returns a unique name generated from the given type name.
     */
    utils::Str generate_valid_resource_name(const utils::Str &type_name) const;

public:

    /**
     * Constructs a new, empty resource manager.
     */
    explicit Manager(
        const ir::compat::PlatformRef &platform,
        const utils::Str &architecture = "",
        const utils::Set<utils::Str> &dnu = {},
        const Factory &factory = {}
    );

    /**
     * Constructs a resource manager based on the given JSON configuration.
     * Refer to dump_docs() for more information.
     */
    static Manager from_json(
        const ir::compat::PlatformRef &platform,
        const utils::Json &json,
        const Factory &factory = {}
    );

    /**
     * Builds the default resource manager for the platform. The JSON data is
     * taken from platform.resources.
     */
    static Manager from_defaults(
        const ir::compat::PlatformRef &platform,
        const Factory &factory = {}
    );

    /**
     * Writes documentation for the available resource types to the given output
     * stream.
     */
    void dump_resource_types(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Writes information about the current configuration of this set of
     * resources to the given output stream.
     */
    void dump_config(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Adds a resource. If no instance name is provided, a unique name is
     * generated.
     */
    void add_resource(
        const utils::Str &type_name,
        const utils::Str &instance_name = "",
        const utils::Json &configuration = {}
    );

    /**
     * Returns whether a resource with the target instance name exists.
     */
    utils::Bool does_resource_exist(
        const utils::Str &target
    );

    /**
     * Removes the resource with the given target instance name, or throws an
     * exception if no such resource exists.
     */
    void remove_resource(
        const utils::Str &target
    );

    /**
     * Builds a state tracker from the configured list of resources.
     */
    State build(Direction direction = Direction::UNDEFINED) const;

};

} // namespace rmgr
} // namespacq ql
