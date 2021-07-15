/** \file
 * Pass factory.
 */

#include "ql/pmgr/factory.h"

#include "ql/utils/pair.h"
#include "ql/pmgr/group.h"

// Pass definition headers. This list should be generated at some point.
#include "ql/pass/ana/visualize/circuit.h"
#include "ql/pass/ana/visualize/interaction.h"
#include "ql/pass/ana/visualize/mapping.h"
#include "ql/pass/ana/statistics/clean.h"
#include "ql/pass/ana/statistics/report.h"
#include "ql/pass/io/cqasm/read.h"
#include "ql/pass/io/cqasm/report.h"
#include "ql/pass/io/sweep_points/write.h"
#include "ql/pass/dec/instructions/instructions.h"
#include "ql/pass/dec/generalize/generalize.h"
#include "ql/pass/dec/specialize/specialize.h"
#include "ql/pass/dec/structure/structure.h"
#include "ql/pass/opt/clifford/optimize.h"
#include "ql/pass/sch/schedule/schedule.h"
#include "ql/pass/sch/list_schedule/list_schedule.h"
//#include "ql/pass/map/qubits/place_mip/place_mip.h" // Broken: need half-decent IR for gates and virtual vs real qubit operands first.
#include "ql/pass/map/qubits/map/map.h"
#include "ql/arch/cc/pass/gen/vq1asm/vq1asm.h"
#include "ql/arch/diamond/pass/gen/microcode/microcode.h"

namespace ql {
namespace pmgr {

/**
 * Constructs a default pass factory for OpenQL.
 */
Factory::Factory() {

    // Default pass registration. This list should be generated at some point.
    register_pass<::ql::pass::ana::visualize::circuit::Pass>("ana.visualize.Circuit");
    register_pass<::ql::pass::ana::visualize::interaction::Pass>("ana.visualize.Interaction");
    register_pass<::ql::pass::ana::visualize::mapping::Pass>("ana.visualize.Mapping");
    register_pass<::ql::pass::ana::statistics::clean::Pass>("ana.statistics.Clean");
    register_pass<::ql::pass::ana::statistics::report::Pass>("ana.statistics.Report");
    register_pass<::ql::pass::io::cqasm::read::Pass>("io.cqasm.Read");
    register_pass<::ql::pass::io::cqasm::report::Pass>("io.cqasm.Report");
    register_pass<::ql::pass::io::sweep_points::write::Pass>("io.sweep_points.Write");
    register_pass<::ql::pass::dec::instructions::Pass>("dec.Instructions");
    register_pass<::ql::pass::dec::generalize::Pass>("dec.Generalize");
    register_pass<::ql::pass::dec::specialize::Pass>("dec.Specialize");
    register_pass<::ql::pass::dec::structure::Pass>("dec.Structure");
    register_pass<::ql::pass::opt::clifford::optimize::Pass>("opt.clifford.Optimize");
    register_pass<::ql::pass::sch::schedule::Pass>("sch.Schedule");
    register_pass<::ql::pass::sch::list_schedule::Pass>("sch.ListSchedule");
    //register_pass<::ql::pass::map::qubits::place_mip::Pass>("map.qubits.PlaceMIP"); // Broken: need half-decent IR for gates and virtual vs real qubit operands first.
    register_pass<::ql::pass::map::qubits::map::Pass>("map.qubits.Map");
    register_pass<::ql::arch::cc::pass::gen::vq1asm::Pass>("arch.cc.gen.VQ1Asm");
    register_pass<::ql::arch::diamond::pass::gen::microcode::Pass>("arch.diamond.gen.Microcode");

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
CFactoryRef Factory::configure(
    const utils::Str &architecture,
    const utils::Set<utils::Str> &dnu
) const {

    // Clone this pass factory into a smart pointer.
    FactoryRef ref;
    ref.emplace(*this);

    // Pull the selected DNU passes into the main namespace, and remove all
    // other DNUs.
    // NOTE: iterating over original pass_types to avoid iterator invalidation!
    for (const auto &pair : pass_types) {
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
        ref->pass_types.erase(type_name);

        // Make a new entry if the original type name is in the dnu set.
        if (dnu.find(type_name) != dnu.end()) {
            ref->pass_types.set(stripped_type_name) = constructor_fn;
        }

    }

    // Make shorthands for the selected architecture, if one is specified.
    if (!architecture.empty()) {
        auto prefix = "arch." + architecture;
        utils::List<utils::Pair<utils::Str, ConstructorFn>> to_be_added;
        for (const auto &pair : ref->pass_types) {
            const auto &type_name = pair.first;
            const auto &constructor_fn = pair.second;
            if (type_name.rfind(prefix, 0) == 0) {
                to_be_added.emplace_back(type_name.substr(prefix.size() + 1), constructor_fn);
            }
        }
        for (const auto &pair : to_be_added) {
            const auto &type_name = pair.first;
            const auto &constructor_fn = pair.second;
            ref->pass_types.set(type_name) = constructor_fn;
        }
    }

    return ref.as_const();
}

/**
 * Builds a pass instance.
 */
PassRef Factory::build_pass(
    const CFactoryRef &pass_factory,
    const utils::Str &type_name,
    const utils::Str &instance_name
) {
    if (type_name.empty()) {
        PassRef ref;
        ref.emplace<Group>(pass_factory, instance_name);
        return ref;
    }
    auto it = pass_factory->pass_types.find(type_name);
    if (it == pass_factory->pass_types.end()) {
        throw utils::Exception("unknown pass type \"" + type_name + "\"");
    }
    return (*it->second)(pass_factory, instance_name);
}

/**
 * Dumps documentation for all pass types known by this factory, as well as
 * the option documentation for each pass.
 */
void Factory::dump_pass_types(
    const CFactoryRef &pass_factory,
    std::ostream &os,
    const utils::Str &line_prefix
) {

    // Gather all aliases for each particular pass type.
    utils::Map<const ConstructorFn::Data*, utils::List<utils::Str>> aliases;
    for (const auto &pair : pass_factory->pass_types) {
        const auto &type_name = pair.first;
        const auto &constructor_fn = pair.second;
        aliases.set(constructor_fn.unwrap().get()).push_back(type_name);
    }

    // Sort pass types by full pass type name.
    utils::Map<utils::Str, utils::Pair<PassRef, utils::List<utils::Str>>> pass_types;
    for (const auto &pair : aliases) {
        const auto *constructor_fn_ptr = pair.first;
        const auto &type_aliases = pair.second;
        auto pass = (*constructor_fn_ptr)(pass_factory, "");
        const auto &full_type_name = pass->get_type();
        QL_ASSERT(pass_types.find(full_type_name) == pass_types.end());
        pass_types.set(full_type_name) = {pass, type_aliases};
    }

    // Dump docs for the discovered passes.
    for (const auto &pair : pass_types) {
        const auto &pass = pair.second.first;
        const auto &type_aliases = pair.second.second;

        os << line_prefix << "* " << pass->get_friendly_type() << " *\n";
        os << line_prefix << "  Type name(s): " << type_aliases.to_string("`", "`, `", "`", "`, or `", "` or `") << ".\n";
        os << line_prefix << "  \n";
        pass->dump_help(os, line_prefix + "  ");
        os << line_prefix << "\n";
    }

}

} // namespace pmgr
} // namespace ql
