/** \file
 * Defines the resource manager.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/utils/map.h"
#include "ql/rmgr/resource_types/base.h"
#include "ql/rmgr/factory.h"
#include "ql/rmgr/state.h"

namespace ql {
namespace rmgr {

/**
 * A collection of resources corresponding to a particular platform.
 */
class Manager {
private:

    /**
     * Factory for constructing resources.
     */
    const Factory factory;

    /**
     * The platform that this resource manager is built for.
     */
    const plat::PlatformRef &platform;

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
        const plat::PlatformRef &platform,
        const utils::Str &architecture = "",
        const utils::Set<utils::Str> &dnu = {},
        const Factory &factory = {}
    );

    /**
     * Constructs a resource manager based on the given JSON configuration.
     *
     * Two JSON structures are supported: one for compatibility with older
     * platform configuration files, and one extended structure. The extended
     * structure has the following syntax:
     *
     * ```json
     * {
     *     "architecture": <optional string, default "">,
     *     "dnu": <optional list of strings, default []>,
     *     "resources": {
     *         "<name>": {
     *             "type": "<type>",
     *             "config": {
     *                 <optional configuration>
     *             }
     *         }
     *         ...
     *     }
     * }
     * ```
     *
     * The optional "architecture" key may be used to make shorthands for
     * architecture- specific resources, normally prefixed with
     * "arch.<architecture>.". If it's not specified or an empty string,
     * the architecture is derived from the platform.
     *
     * The optional "dnu" key may be used to specify a list of do-not-use
     * resource types (experimental, deprecated, or any other resource that's
     * considered unfit for "production" use) that you explicitly want to use,
     * including the "dnu" namespace they are defined in. Once specified, you'll
     * be able to use the resource type without the "dnu" namespace element. For
     * example, if you would include "dnu.whatever" in the list, the resource
     * type "whatever" may be used to add the resource.
     *
     * The "resources" key specifies the actual resource list. This consists of
     * a map from unique resource names matching `[a-zA-Z0-9_\-]+` to a resource
     * configuration. The configuration object must have a "type" key, which
     * must identify a resource type that OpenQL knows about. The "config" key
     * is optional, and is used to pass type-specific configuration data to the
     * resource. If not specified, an empty JSON object will be passed to the
     * resource instead.
     *
     * If the "resources" key is not present, the old structure is used instead.
     * This has the following simplified form:
     *
     * ```json
     * {
     *     "<type>": {
     *         <configuration>
     *     },
     *     ...
     * }
     * ```
     */
    static Manager from_json(
        const plat::PlatformRef &platform,
        const utils::Json &json,
        const Factory &factory = {}
    );

    /**
     * Builds the default resource manager for the platform. The JSON data is
     * taken from platform.resources.
     */
    static Manager from_defaults(
        const plat::PlatformRef &platform,
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
