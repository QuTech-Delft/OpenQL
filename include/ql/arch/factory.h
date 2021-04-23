/** \file
 * Resource factory implementation.
 */

#pragma once

#include <functional>
#include "ql/utils/ptr.h"
#include "ql/utils/str.h"
#include "ql/utils/map.h"
#include "ql/utils/json.h"
#include "ql/arch/info_base.h"

namespace ql {
namespace arch {

/**
 * Factory class for constructing architecture wrappers.
 */
class Factory {
private:

    /**
     * Map from architecture namespace name to a constructor function for that
     * particular architecture type.
     */
    utils::Map<utils::Str, InfoRef> namespace_names = {};

    /**
     * Map from "eqasm_compiler" key value to a constructor function for that
     * particular architecture type.
     */
    utils::Map<utils::Str, InfoRef> eqasm_compiler_names = {};

public:

    /**
     * Constructs a default architecture factory for OpenQL.
     */
    Factory();

    /**
     * Registers an architecture class with the given type name.
     */
    template <class ArchitectureType>
    void register_architecture() {
        InfoRef architecture;
        architecture.emplace<ArchitectureType>();
        namespace_names.set(architecture->get_namespace_name()) = architecture;
        for (const auto &name : architecture->get_eqasm_compiler_names()) {
            eqasm_compiler_names.set(name) = architecture;
        }
    }

    /**
     * Builds an architecture from a namespace name. Returns an empty reference
     * if none was found.
     */
    CInfoRef build_from_namespace(const utils::Str &namspace) const;

    /**
     * Builds an architecture from an "eqasm_compiler" name. Returns an empty
     * reference if none was found.
     */
    CInfoRef build_from_eqasm_compiler(const utils::Str &eqasm_compiler) const;

    /**
     * Dumps documentation for all architectures known by this factory.
     */
    void dump_architectures(std::ostream &os = std::cout, const utils::Str &line_prefix = "") const;

};

} // namespace arch
} // namespacq ql
