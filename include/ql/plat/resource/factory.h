/** \file
 * Resource factory implementation.
 */

#pragma once

#include <functional>
#include "ql/utils/ptr.h"
#include "ql/utils/str.h"
#include "ql/utils/map.h"
#include "ql/plat/platform.h"
#include "ql/plat/resource/base.h"

namespace ql {
namespace plat {
namespace resource {

/**
 * Factory class for constructing resources.
 */
class Factory {
private:

    /**
     * Function pointer object type that is used to construct resource class
     * instances.
     */
    using ConstructorFn = utils::Ptr<
        std::function<
            Ref(
                const utils::Str &instance_name,
                const plat::PlatformRef &platform,
                Direction direction
            )
        >
    >;

    /**
     * Map from (desugared) resource type name to a constructor function for
     * that particular resource type.
     */
    utils::Map<utils::Str, ConstructorFn> resource_types;

public:

    /**
     * Constructs a default resource factory for OpenQL.
     */
    Factory();

    /**
     * Registers a resource class with the given type name.
     */
    template <class ResourceType>
    void register_resource(const utils::Str &type_name) {
        ConstructorFn fn;
        fn.emplace([type_name](
            const utils::Str &instance_name,
            const plat::PlatformRef &platform,
            Direction direction
        ) {
            Ref resource;
            resource.emplace<ResourceType>(type_name, instance_name, platform, direction);
            return resource;
        });
        resource_types.set(type_name) = fn;
    }

    /**
     * Returns a copy of this pass factory with the following modifications made
     * to the map.
     *
     *  - Entries with a `dnu` path component in them are removed. If the type
     *    of the removed entry exists in dnu however, it will be reinserted with
     *    the `dnu` path component removed.
     *  - A copy is made of entries that include an `arch.<architecture>`
     *    component pair, with that pair stripped.
     *
     * The original factory is not modified.
     */
    Factory configure(
        const utils::Str &architecture,
        const utils::Set<utils::Str> &dnu
    ) const;

    /**
     * Builds a resource instance.
     */
    Ref build_resource(
        const utils::Str &type_name,
        const utils::Str &instance_name,
        const plat::PlatformRef &platform,
        Direction direction
    );

    /**
     * Dumps documentation for all resource types known by this factory.
     */
    void dump_resource_types(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    );

};

} // namespace resource
} // namespace plat
} // namespacq ql
