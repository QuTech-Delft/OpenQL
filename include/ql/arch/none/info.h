/** \file
 * Defines information about the no-op architecture.
 */

#pragma once

#include <iostream>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/set.h"
#include "ql/utils/ptr.h"
#include "ql/utils/json.h"
#include "ql/pmgr/manager.h"
#include "ql/arch/info_base.h"

namespace ql {
namespace arch {
namespace none {

/**
 * No-op architecture-specific information class.
 */
class Info : public InfoBase {
public:

    /**
     * Writes the documentation for this architecture to the given output
     * stream.
     */
    void dump_docs(std::ostream &os, const utils::Str &line_prefix) const override;

    /**
     * Returns a user-friendly type name for this architecture. Used for
     * documentation generation.
     */
    utils::Str get_friendly_name() const override;

    /**
     * Returns the name of the namespace for this architecture.
     */
    utils::Str get_namespace_name() const override;

    /**
     * Returns a list of strings accepted for the "eqasm_compiler" key in the
     * platform configuration file. This can be more than one, to support both
     * legacy (inconsistent) names and the new namespace names. The returned
     * set must include at least the name of the namespace.
     */
    utils::List<utils::Str> get_eqasm_compiler_names() const override;

    /**
     * Should generate a sane default platform JSON file, for when the user
     * constructs a Platform without JSON data. This is done by specifying an
     * architecture namespace identifier instead of a JSON filename. Optionally,
     * the user may specify a variant suffix, separated using a dot, to select
     * a variation of the architecture; for instance, for CC-light, there might
     * be variations for surface-5, surface-7, and surface-17. This JSON data
     * will still be preprocessed by preprocess_platform().
     */
    utils::Str get_default_platform(const utils::Str &variant) const override;

};

} // namespace none
} // namespace arch
} // namespace ql
