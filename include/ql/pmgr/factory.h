/** \file
 * Pass factory.
 */

#pragma once

#include <functional>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/map.h"
#include "ql/ir/compat/compat.h"
#include "ql/pmgr/declarations.h"
#include "ql/pmgr/pass_types/base.h"

namespace ql {
namespace pmgr {

// Forward declaration.
class Factory;

/**
 * Mutable reference to a pass factory.
 */
using FactoryRef = utils::Ptr<Factory>;

/**
 * Immutable reference to a pass factory.
 */
using CFactoryRef = utils::Ptr<const Factory>;

/**
 * Factory class for constructing passes.
 */
class Factory {
private:

    /**
     * Function pointer object type that is used to construct pass class
     * instances.
     */
    using ConstructorFn = utils::Ptr<
        std::function<
            PassRef(
                const CFactoryRef &pass_factory,
                const utils::Str &instance_name
            )
        >
    >;

    /**
     * Map from (desugared) pass type name to a constructor function for that
     * particular pass type.
     */

    static utils::Map<utils::Str, ConstructorFn>& pass_types() {
        static utils::Map<utils::Str, ConstructorFn> pass_types{};

        return pass_types;
    }

public:

    /**
     * Registers a pass class with the given type name.
     */
    template <class PassType>
    static bool register_pass(const utils::Str &type_name) {
        ConstructorFn fn;
        fn.emplace([type_name](
            const CFactoryRef &pass_factory,
            const utils::Str &instance_name
        ) {
            PassRef pass;
            pass.emplace<PassType>(pass_factory, type_name, instance_name);
            return pass;
        });
        pass_types().set(type_name) = fn;
        return true;
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
    CFactoryRef configure(
        const utils::Str &architecture,
        const utils::Set<utils::Str> &dnu
    ) const;

    /**
     * Builds a pass instance.
     */
    static PassRef build_pass(
        const CFactoryRef &pass_factory,
        const utils::Str &type_name,
        const utils::Str &instance_name
    );

    /**
     * Dumps documentation for all pass types known by this factory, as well as
     * the option documentation for each pass.
     */
    static void dump_pass_types(
        const CFactoryRef &pass_factory,
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    );

};

} // namespace pmgr
} // namespace ql
