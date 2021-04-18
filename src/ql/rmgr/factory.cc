/** \file
 * Resource factory implementation.
 */

#include "ql/rmgr/factory.h"

#include <ql/utils/list.h>
#include <ql/utils/pair.h>

// Resource definition headers. This list should be generated at some point.
#include "ql/resource/todo.h"

namespace ql {
namespace rmgr {

/**
 * Constructs a default resource factory for OpenQL.
 */
Factory::Factory() {

    // Default resource registration. This list should be generated at some point.
    register_resource<resource::Qubits>("arch.cc_light.qubits");
    register_resource<resource::QWGs>("arch.cc_light.qwgs");
    register_resource<resource::MeasUnits>("arch.cc_light.meas_units");
    register_resource<resource::Edges>("arch.cc_light.edges");
    register_resource<resource::DetunedQubits>("arch.cc_light.detuned_qubits");
    register_resource<resource::Channels>("arch.cc_light.channels");

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
Factory Factory::configure(
    const utils::Str &architecture,
    const utils::Set<utils::Str> &dnu
) const {

    // Clone this pass factory into a smart pointer.
    Factory retval(*this);

    // Pull the selected DNU resources into the main namespace, and remove all
    // other DNUs.
    // NOTE: iterating over original resource_types to avoid iterator
    // invalidation!
    for (const auto &pair : resource_types) {
        const auto &type_name = pair.first;
        const auto &constructor_fn = pair.second;

        // Iterate over the period-separated namespace elements of the type
        // name.
        utils::UInt start = 0;
        utils::UInt end;
        utils::Bool is_dnu = false;
        utils::Str stripped_type_name;
        do {

            // Find the next element.
            end = type_name.find('.', start);
            const auto &element = type_name.substr(start, end);
            start = end + 1;

            // If the element is "dnu", this is a "dnu" type, which will be
            // removed if it's not in the dnu set. If it is in the dnu set, we
            // want to replace the key with the type name as it would have been
            // without the dnu elements, so we construct that type name as well.
            if (element == "dnu") {
                is_dnu = true;
            } else {
                if (!stripped_type_name.empty()) {
                    stripped_type_name += '.';
                }
                stripped_type_name += element;
            }
        } while (end != utils::Str::npos);

        // Ignore non-dnu types.
        if (!is_dnu) {
            continue;
        }

        // Delete the original entry for a dnu type unconditionally.
        retval.resource_types.erase(type_name);

        // Make a new entry if the original type name is in the dnu set.
        if (dnu.find(type_name) != dnu.end()) {
            retval.resource_types.set(stripped_type_name) = constructor_fn;
        }

    }

    // Make shorthands for the selected architecture, if one is specified.
    if (!architecture.empty()) {
        auto prefix = "arch." + architecture;
        utils::List<utils::Pair<utils::Str, ConstructorFn>> to_be_added;
        for (const auto &pair : retval.resource_types) {
            const auto &type_name = pair.first;
            const auto &constructor_fn = pair.second;
            if (type_name.rfind(prefix, 0) == 0) {
                to_be_added.emplace_back(type_name.substr(prefix.size() + 1), constructor_fn);
            }
        }
        for (const auto &pair : to_be_added) {
            const auto &type_name = pair.first;
            const auto &constructor_fn = pair.second;
            retval.resource_types.set(type_name) = constructor_fn;
        }
    }

    return retval;
}

/**
 * Builds a resource instance.
 */
ResourceRef Factory::build_resource(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const plat::PlatformRef &platform,
    const utils::Json &configuration
) const {
    auto it = resource_types.find(type_name);
    if (it == resource_types.end()) {
        throw utils::Exception("unknown pass type \"" + type_name + "\"");
    }
    return (*it->second)(instance_name, platform, configuration);
}

/**
 * Dumps documentation for all resource types known by this factory.
 */
void Factory::dump_resource_types(
    std::ostream &os,
    const utils::Str &line_prefix
) const {

    // Gather all aliases for each particular pass type.
    utils::Map<const ConstructorFn::Data*, utils::List<utils::Str>> aliases;
    for (const auto &pair : resource_types) {
        const auto &type_name = pair.first;
        const auto &constructor_fn = pair.second;
        aliases.set(constructor_fn.unwrap().get()).push_back(type_name);
    }

    // Sort pass types by full pass type name.
    utils::Map<utils::Str, utils::Pair<ResourceRef, utils::List<utils::Str>>> pass_types;
    for (const auto &pair : aliases) {
        const auto *constructor_fn_ptr = pair.first;
        const auto &type_aliases = pair.second;
        auto pass = (*constructor_fn_ptr)("", {}, {});
        const auto &full_type_name = pass->get_type();
        QL_ASSERT(pass_types.find(full_type_name) == pass_types.end());
        pass_types.set(full_type_name) = {pass, type_aliases};
    }

    // Dump docs for the discovered passes.
    for (const auto &pair : pass_types) {
        const auto &full_type_name = pair.first;
        const auto &pass = pair.second.first;
        const auto &type_aliases = pair.second.second;

        os << line_prefix << "Pass " << full_type_name;
        os << " with alias(es) " << type_aliases.to_string("", ", ", "", ", or ", " or ");
        os << ":\n";
        os << line_prefix << "\n";
        pass->dump_docs(os, line_prefix + "  ");
        os << line_prefix << "\n";
    }

}

} // namespace rmgr
} // namespace ql
