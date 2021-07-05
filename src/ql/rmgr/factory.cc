/** \file
 * Resource factory implementation.
 */

#include "ql/rmgr/factory.h"

#include <ql/utils/list.h>
#include <ql/utils/pair.h>

// Resource definition headers. This list should be generated at some point.
#include "ql/resource/qubit.h"
#include "ql/resource/instrument.h"
#include "ql/resource/inter_core_channel.h"

namespace ql {
namespace rmgr {

/**
 * Constructs a default resource factory for OpenQL.
 */
Factory::Factory() {

    // Default resource registration. This list should be generated at some point.
    register_resource<resource::qubit::Resource>("Qubit");
    register_resource<resource::instrument::Resource>("Instrument");
    register_resource<resource::inter_core_channel::Resource>("InterCoreChannel");

    // Register with old CC-light names for backward-compatibility.
    resource_types.set("arch.cc_light.qubits") = resource_types.at("Qubit");
    register_resource<resource::instrument::Resource>("arch.cc_light.qwgs");
    register_resource<resource::instrument::Resource>("arch.cc_light.meas_units");
    register_resource<resource::instrument::Resource>("arch.cc_light.edges");
    register_resource<resource::instrument::Resource>("arch.cc_light.detuned_qubits");
    register_resource<resource::instrument::Resource>("arch.cc_light.detuned_qubits");
    register_resource<resource::inter_core_channel::Resource>("arch.cc_light.channels");

}

/**
 * Returns a copy of this resource factory with the following modifications made
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

    // Clone this resource factory into a smart pointer.
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
    const ir::compat::PlatformRef &platform,
    const utils::Json &configuration
) const {
    auto it = resource_types.find(type_name);
    if (it == resource_types.end()) {
        throw utils::Exception("unknown resource type \"" + type_name + "\"");  // FIXME: this is a JSON error, provide context to user
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

    // Gather all aliases for each particular resource type.
    utils::Map<const ConstructorFn::Data*, utils::List<utils::Str>> aliases;
    for (const auto &pair : resource_types) {
        const auto &type_name = pair.first;
        const auto &constructor_fn = pair.second;
        aliases.set(constructor_fn.unwrap().get()).push_back(type_name);
    }

    // Sort resource types by full resource type name.
    utils::Map<utils::Str, utils::Pair<ResourceRef, utils::List<utils::Str>>> resource_types;
    for (const auto &pair : aliases) {
        const auto *constructor_fn_ptr = pair.first;
        const auto &type_aliases = pair.second;
        auto resource = (*constructor_fn_ptr)("", {}, {});
        const auto &full_type_name = resource->get_type();
        QL_ASSERT(resource_types.find(full_type_name) == resource_types.end());
        resource_types.set(full_type_name) = {resource, type_aliases};
    }

    // Dump docs for the discovered resources.
    for (const auto &pair : resource_types) {
        const auto &resource = pair.second.first;
        const auto &type_aliases = pair.second.second;

        os << line_prefix << "* " << resource->get_friendly_type() << " *\n";
        os << line_prefix << "  Type names: " << type_aliases.to_string("`", "`, `", "`", "`, or `", "` or `") << ".\n";
        os << line_prefix << "  \n";
        resource->dump_docs(os, line_prefix + "  ");
        os << line_prefix << "\n";
    }

}

} // namespace rmgr
} // namespace ql
