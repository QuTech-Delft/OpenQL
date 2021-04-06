/** \file
 * Pass management.
 */

#include "ql/pmgr/pass_manager.h"

#include "ql/utils/filesystem.h"
#include "ql/com/options/options.h"

// Pass definition headers. This list should be generated at some point.
#include "ql/pass/ana/visualize/circuit.h"
#include "ql/pass/ana/visualize/interaction.h"
#include "ql/pass/ana/visualize/mapping.h"

namespace ql {
namespace pmgr {

/**
 * Constructs the pass group. No error checking here; this is up to the
 * parent pass group. Note that the type name is missing, and that
 * instance_name defaults to the empty string; generic passes always have
 * an empty type name, and the root group has an empty instance name as
 * well.
 */
PassGroup::PassGroup(
    const utils::Ptr<const PassFactory> &pass_factory,
    const utils::Str &instance_name
) : pass_types::Group(pass_factory, instance_name, "") {
}

/**
 * Implementation for the initial pass list. This is no-op for a generic
 * pass group.
 */
void PassGroup::get_passes(
    const utils::Ptr<const PassFactory> &factory,
    utils::List<PassRef> &passes
) {
}

/**
 * Writes the documentation for a basic pass group to the given stream.
 */
void PassGroup::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    os << line_prefix << "A basic pass group. These pass groups are just a tool for abstracting\n";
    os << line_prefix << "a number of passes that together perform a single logical transformation\n";
    os << line_prefix << "into a single pass. A pass group has exactly the same behavior as just\n";
    os << line_prefix << "putting all the contained passes in the parent pass group.\n";
}

/**
 * Constructs a default pass factory for OpenQL.
 */
PassFactory::PassFactory() {

    // Default pass registration. This list should be generated at some point.
    register_pass<::ql::pass::ana::visualize::circuit::Pass>("ana.visualize.Circuit");
    register_pass<::ql::pass::ana::visualize::interaction::Pass>("ana.visualize.Interaction");
    register_pass<::ql::pass::ana::visualize::mapping::Pass>("ana.visualize.Mapping");

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
 * Furthermore, the debug_dumpers member is modified accordingly. The
 * original factory is not modified.
 */
CPassFactoryRef PassFactory::configure(
    const utils::Str &architecture,
    const utils::Set<utils::Str> &dnu,
    const utils::List<utils::Pair<utils::Str, utils::Str>> &debug_dumpers
) const {

    // Clone this pass factory into a smart pointer.
    PassFactoryRef ref;
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

    // Set debug dumper list.
    ref->debug_dumpers = debug_dumpers;

    return ref.as_const();
}

/**
 * Builds a pass instance.
 */
PassRef PassFactory::build_pass(
    const CPassFactoryRef &pass_factory,
    const utils::Str &type_name,
    const utils::Str &instance_name
) {
    if (type_name.empty()) {
        PassRef ref;
        ref.emplace<PassGroup>(pass_factory, instance_name);
        return ref;
    }
    auto it = pass_factory->pass_types.find(type_name);
    if (it == pass_factory->pass_types.end()) {
        throw utils::Exception("unknown pass type \"" + type_name + "\"");
    }
    return (*it->second)(pass_factory, instance_name);
}

/**
 * Prefixes and suffixes the given pass list with the debug dumpers
 * configured for this factory.
 */
void PassFactory::add_debug_dumpers(
    const CPassFactoryRef &pass_factory,
    utils::List<PassRef> &passes
) {
    auto front_it = passes.begin();
    for (const auto &pair : pass_factory->debug_dumpers) {
        const auto &type_name = pair.first;
        const auto &instance_name = pair.second;
        front_it = std::next(passes.insert(front_it, build_pass(pass_factory, type_name, instance_name + "_pre")));
        passes.push_back(build_pass(pass_factory, type_name, instance_name + "_post"));
    }
}

/**
 * Dumps documentation for all pass types known by this factory, as well as
 * the option documentation for each pass.
 */
void PassFactory::dump_pass_types(
    const CPassFactoryRef &pass_factory,
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
        const auto &full_type_name = pair.first;
        const auto &pass = pair.second.first;
        const auto &type_aliases = pair.second.second;

        os << line_prefix << "Pass " << full_type_name;
        os << " with alias(es) " << type_aliases.to_string("", ", ", "", ", or ", " or ");
        os << ":\n";
        os << line_prefix << "\n";
        pass->dump_help(os, line_prefix + "  ");
        os << line_prefix << "\n";
    }

}

/**
 * Constructs a new pass manager.
 */
PassManager::PassManager(
    const utils::Str &architecture,
    const utils::Set<utils::Str> &dnu,
    const utils::List<utils::Pair<utils::Str, utils::Str>> &debug_dumpers,
    const PassFactory &factory
) {
    pass_factory = factory.configure(architecture, dnu, debug_dumpers);
    root = PassFactory::build_pass(pass_factory, "", "");
}

/**
 * Load passes into the given pass group from a JSON array of pass descriptions.
 */
static void add_passes_from_json(const PassRef &group, const utils::Json &json) {

    // Shorthand.
    using JsonType = utils::Json::value_t;

    for (const auto &pass_description : json) {

        // Parse the JSON structure.
        utils::Str type;
        utils::Str name;
        utils::Map<utils::Str, utils::Str> options;
        const utils::Json *sub_passes = nullptr;
        if (pass_description.type() == JsonType::string) {
            type = pass_description.get<utils::Str>();
        } else if (pass_description.type() == JsonType::object) {
            for (auto it = pass_description.begin(); it != pass_description.end(); ++it) {
                if (it.key() == "type") {
                    if (it.value().type() == JsonType::string) {
                        type = it.value().get<utils::Str>();
                    } else {
                        throw utils::Exception("pass type must be a string if specified");
                    }
                } else if (it.key() == "name") {
                    if (it.value().type() == JsonType::string) {
                        name = it.value().get<utils::Str>();
                    } else {
                        throw utils::Exception("pass name must be a string if specified");
                    }
                } else if (it.key() == "options") {
                    if (it.value().type() == JsonType::object) {
                        for (auto opt_it = it->begin(); opt_it != it->end(); ++opt_it) {
                            if (opt_it.value().type() == JsonType::boolean) {
                                options.set(opt_it.key()) = opt_it.value().get<utils::Bool>() ? "yes" : "no";
                            } else if (opt_it.value().type() == JsonType::number_integer) {
                                options.set(opt_it.key()) = utils::to_string(opt_it.value().get<utils::Int>());
                            } else if (opt_it.value().type() == JsonType::number_unsigned) {
                                options.set(opt_it.key()) = utils::to_string(opt_it.value().get<utils::UInt>());
                            } else if (opt_it.value().type() == JsonType::string) {
                                options.set(opt_it.key()) = opt_it.value().get<utils::Str>();
                            } else {
                                throw utils::Exception("pass option value must be a boolean, integer, or string");
                            }
                        }
                    } else {
                        throw utils::Exception("pass options must be an object if specified");
                    }
                } else if (it.key() == "group") {
                    if (it.value().type() == JsonType::array) {
                        sub_passes = &it.value();
                    } else {
                        throw utils::Exception("pass group must be an array of pass descriptions");
                    }
                } else {
                    throw utils::Exception("unknown key in pass description: " + it.key());
                }
            }
        } else {
            throw utils::Exception("pass description must be a string or an object");
        }
        if (type.empty() && sub_passes == nullptr) {
            throw utils::Exception("either pass type or pass group must be specified");
        }

        // Add the pass.
        auto pass = group->append_sub_pass(type, name, options);

        // If the pass has sub-passes, construct it and add them by recursively
        // calling ourselves.
        if (sub_passes != nullptr) {
            pass->construct();
            if (!pass->is_group()) {
                throw utils::Exception("pass type " + type + " does not support sub-passes");
            }
            add_passes_from_json(pass, *sub_passes);
        }

    }
}

/**
 * Constructs a pass manager based on the given JSON configuration.
 *
 * Refer to the header file for details.
 */
PassManager PassManager::from_json(
    const utils::Json &json,
    const PassFactory &factory
) {
    // TODO JvS: need proper schema validation in some JSON structure wrapper!
    //  All this repetition is bad.

    // Shorthand.
    using JsonType = utils::Json::value_t;

    // Look for the strategy key. Ignore any other keys in the toplevel
    // structure.
    auto it = json.find("strategy");
    if (it == json.end()) {
        throw utils::Exception("missing strategy key");
    }
    const auto &strategy = *it;
    if (strategy.type() != utils::Json::value_t::object) {
        throw utils::Exception("strategy key must be an object");
    }

    // Read the strategy structure.
    utils::Str architecture = {};
    utils::Set<utils::Str> dnu = {};
    const utils::Json *passes = nullptr;
    for (it = strategy.begin(); it != strategy.end(); ++it) {
        if (it.key() == "architecture") {
            if (it.value().type() == JsonType::string) {
                architecture = it.value().get<utils::Str>();
            } else {
                throw utils::Exception("strategy.architecture must be a string if specified");
            }
        } else if (it.key() == "dnu") {
            if (it.value().type() == JsonType::string) {
                dnu.insert(it.value().get<utils::Str>());
            } else if (it.value().type() == JsonType::array) {
                for (const auto &val : it.value()) {
                    if (val.type() == JsonType::string) {
                        dnu.insert(it.value().get<utils::Str>());
                    } else {
                        throw utils::Exception("strategy.dnu.* must be a string");
                    }
                }
            } else {
                throw utils::Exception("strategy.dnu must be a string or array of strings if specified");
            }
        } else if (it.key() == "passes") {
            if (it.value().type() == JsonType::array) {
                passes = &it.value();
            } else {
                throw utils::Exception("strategy.passes must be an array of pass descriptions");
            }
        } else {
            throw utils::Exception("unknown key in strategy: " + it.key());
        }
    }
    if (!passes) {
        throw utils::Exception("missing strategy.passes key");
    }

    // Construct the pass manager.
    PassManager pm{architecture, dnu};

    // Add passes from the pass descriptions.
    add_passes_from_json(pm.get_root(), *passes);

    return pm;
}

/**
 * Returns a reference to the root pass group.
 */
const PassRef &PassManager::get_root() {
    return root;
}

/**
 * Returns a reference to the root pass group.
 */
CPassRef PassManager::get_root() const {
    return root.as_const();
}

/**
 * Dumps documentation for all available pass types, as well as the option
 * documentation for the passes.
 */
void PassManager::dump_pass_types(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    PassFactory::dump_pass_types(pass_factory, os, line_prefix);
}

/**
 * Dumps the currently configured compilation strategy to the given stream.
 */
void PassManager::dump_strategy(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    root->dump_strategy(os, line_prefix);
}

/**
 * Appends a pass to the end of the pass list. If type_name is empty
 * or unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
PassRef PassManager::append_pass(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    return root->append_sub_pass(type_name, instance_name, options);
}

/**
 * Appends a pass to the beginning of the pass list. If type_name is empty
 * or unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
PassRef PassManager::prefix_pass(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    return root->prefix_sub_pass(type_name, instance_name, options);
}

/**
 * Inserts a pass immediately after the target pass (named by instance). If
 * target does not exist, an exception is thrown. If type_name is empty or
 * unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
PassRef PassManager::insert_pass_after(
    const utils::Str &target,
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    return root->insert_sub_pass_after(target, type_name, instance_name);
}

/**
 * Inserts a pass immediately before the target pass (named by instance). If
 * target does not exist, an exception is thrown. If type_name is empty or
 * unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
PassRef PassManager::insert_pass_before(
    const utils::Str &target,
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    return root->insert_sub_pass_before(target, type_name, instance_name);
}

/**
 * Looks for the pass with the target instance name, and embeds it into a
 * newly generated group. The newly created group will assume the name of
 * the original pass, while the original pass will be renamed as specified
 * by sub_name. Note that this ultimately does not modify the pass order.
 * If the target
 */
PassRef PassManager::group_pass(
    const utils::Str &target,
    const utils::Str &sub_name
) {
    return root->group_sub_pass(target, sub_name);
}

/**
 * Like group_pass(), but groups an inclusive range of passes into a group
 * with the given name, leaving the original pass names unchanged.
 */
PassRef PassManager::group_passes(
    const utils::Str &from,
    const utils::Str &to,
    const utils::Str &group_name
) {
    return root->group_sub_passes(from, to, group_name);
}

/**
 * Looks for an unconditional pass group with the target instance name and
 * flattens its contained passes into its parent group. The names of the
 * passes found in the collapsed group are prefixed with name_prefix before
 * they are added to the parent group. Note that this ultimately does not
 * modify the pass order. If the target instance name does not exist or is
 * not an unconditional group, an exception is thrown.
 */
void PassManager::flatten_subgroup(
    const utils::Str &target,
    const utils::Str &name_prefix
) {
    root->flatten_subgroup(target, name_prefix);
}

/**
 * Returns a reference to the pass with the given instance name. If no such
 * pass exists, an exception is thrown.
 */
PassRef PassManager::get_pass(const utils::Str &target) const {
    return root->get_sub_pass(target);
}

/**
 * Returns whether a pass with the target instance name exists.
 */
utils::Bool PassManager::does_pass_exist(const utils::Str &target) const {
    return root->does_sub_pass_exist(target);
}

/**
 * Returns the total number of passes in the root hierarchy.
 */
utils::UInt PassManager::get_num_passes() const {
    return root->get_num_sub_passes();
}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the list containing all the sub-passes. Otherwise, an exception is
 * thrown.
 */
const utils::List<PassRef> &PassManager::get_passes() const {
    return root->get_sub_passes();
}

/**
 * Returns an indexable list of references to all passes with the given
 * type within the root hierarchy.
 */
utils::Vec<PassRef> PassManager::get_sub_passes_by_type(const utils::Str &target) const {
    return root->get_sub_passes_by_type(target);
}

/**
 * Removes the pass with the given target instance name, or throws an
 * exception if no such pass exists.
 */
void PassManager::remove_pass(const utils::Str &target) {
    return root->remove_sub_pass(target);
}

/**
 * Clears the entire pass list.
 */
void PassManager::clear_passes() {
    return root->clear_sub_passes();
}

/**
 * Constructs all passes recursively. This freezes the pass options, but
 * allows subtrees to be modified.
 */
void PassManager::construct() {
    utils::Bool still_preprocessing_platform = true;
    root->construct_recursive(still_preprocessing_platform);
}

/**
 * Executes this pass or pass group on the given platform and program.
 */
void PassManager::compile(const ir::ProgramRef &program) {

    // Ensure that all passes are constructed.
    construct();

    // Ensure that the output directory exists.
    utils::make_dirs(com::options::get("output_dir"));

    // Compile the program.
    root->compile(program, "");

}

} // namespace pmgr
} // namespace ql
