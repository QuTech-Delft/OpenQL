/** \file
 * Pass management.
 */

#include "ql/pmgr/manager.h"

#include "ql/utils/filesystem.h"
#include "ql/com/options.h"
#include "ql/arch/architecture.h"
#include "ql/ir/cqasm/write.h"

namespace ql {
namespace pmgr {

/**
 * Dumps the documentation for the pass JSON configuration structure.
 */
void Manager::dump_docs(std::ostream &os, const utils::Str &line_prefix) {
    utils::dump_str(os, line_prefix, R"(
    The compiler configuration JSON file (or JSON substructure) is expected to
    have the following structure:

    ```
    {
        "architecture": <optional string, default "">,
        "dnu": <optional list of strings, default []>,
        "pass-options": <optional object, default {}>,
        "compatibility-mode": <optional boolean, default false>,
        "passes": [
            <pass description>
        ]
    }
    ```

    The optional `"architecture"` key may be used to make shorthands for
    architecture- specific passes, normally prefixed with
    `"arch.<architecture>."`. If it's not specified or an empty string, no
    shorthand aliases are made.

    The optional `"dnu"` key may be used to specify a list of do-not-use pass
    types (experimental passes, deprecated passes, or any other pass that's
    considered unfit for "production" use) that you explicitly want to use,
    including the "dnu" namespace they are defined in. Once specified, you'll
    be able to use the pass type without the `"dnu"` namespace element. For
    example, if you would include `"dnu.whatever"` in the list, the pass type
    `"whatever"` may be used to add the pass.

    The optional `"pass-options"` key may be used to specify options common to
    all passes. The values may be booleans, integers, strings, or null, but
    nothing else. Null is used to reset an option to its hardcoded default
    value. An option need not exist for each pass affected by it; if it
    doesn't, the default value is silently ignored for that pass. However, if
    it *does* exist, it must be a valid value for the option with that name.
    These option values propagate through the pass tree recursively, so
    setting a default option in the root using this record will affect all
    passes.

    If `"compatibility-mode"` is enabled, some of OpenQL's global options add
    implicit entries to the `"pass-options"` structure when set, for backward
    compatibility. However, entries in `"pass-options"` always take precedence.
    The logic for which options map to which is mostly documented in the
    global option docs now, since those options don't do anything else
    anymore. Note that the global options by their original design have no
    way to specify what pass they refer to, so each option is attempted for
    each pass type! Which means we have to be a bit careful with picking
    option names for the passes that are included in compatibility mode.

    Pass descriptions can either be strings (in which case the string is
    interpreted as a pass type alias and everything else is
    inferred/default), or an object with the following structure.
    )"
#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    R"(
    ```
    {
        "type": <optional string, default "">,
        "name": <optional string, default "">,
        "options": <optional object, default {}>,
        "group-options": <optional object, default {}>,
        "group": [
            <optional list of pass descriptions>
        ]
    }
    ```
)"
#else
    R"(
    ```
    {
        "type": <optional string, default "">,
        "name": <optional string, default "">,
        "options": <optional object, default {}>
    }
    ```
    )"
#endif
    R"(
    The `"type"` key, if specified, must identify a pass type that OpenQL knows
    about. You can call `print_pass_types()` on a `ql.Compiler` object to get
    the list of available pass types (and their documentation) for your
    particular configuration (just make an empty compiler object initially), or
    you can read the documentation section on supported passes. If the `"type"`
    key is not specified or empty, a group is made instead, and `"group"` must
    be specified for the group to do anything.

    The `"name"` key, if specified, is a user-defined name for the pass, that
    must match `[a-zA-Z0-9_\-]+` and be unique within the surrounding pass
    list. If not specified, a name that complies with these requirements is
    generated automatically, but the actual generated name should not be
    relied upon to be consistent between OpenQL versions. The name may be
    used to programmatically refer to passes after construction, and passes
    may use it for logging or unique output filenames. However, passes should
    not use the name for anything that affects the behavior of the pass.

    The `"options"` key, if specified, may be an object that maps option names
    to option values. The values may be booleans, integers, strings, or null,
    but nothing else. Null is used to enforce usage of the OpenQL-default
    value for the option. The option names and values must be supported by
    the particular pass type.
    )"
#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    R"(
    The `"group-options"` key, if specified, works just like "pass-options" in
    the root, but affects only the sub-passes of this pass (so *not* this
    pass itself). Any option specified here will override any
    previously-specified option. Specifying null resets the option to its
    OpenQL-hardcoded default value.

    The `"group"` key must only be used when `"type"` is set to an empty string
    or left unspecified, turning the pass into a basic group. The list then
    specifies the sub-passes for the group. A normal pass may or may not have
    configurable sub-passes depending on its type and configuration; if it
    doesn't, `"group"` must not be specified.
    )");
#else
    );
#endif
}

/**
 * Constructs a new pass manager.
 */
Manager::Manager(
    const utils::Str &architecture,
    const utils::Set<utils::Str> &dnu,
    const Factory &factory
) {
    pass_factory = factory.configure(architecture, dnu);
    root = Factory::build_pass(pass_factory, "", "");
}

/**
 * Converts a JSON pass option value to its internal string representation.
 */
static utils::Str option_value_from_json(const utils::Json &json) {

    // Shorthand.
    using JsonType = utils::Json::value_t;

    if (json.type() == JsonType::boolean) {
        return json.get<utils::Bool>() ? "yes" : "no";
    } else if (json.type() == JsonType::number_integer) {
        return utils::to_string(json.get<utils::Int>());
    } else if (json.type() == JsonType::number_unsigned) {
        return utils::to_string(json.get<utils::UInt>());
    } else if (json.type() == JsonType::string) {
        return json.get<utils::Str>();
    } else if (json.type() == JsonType::null) {
        return "!@#NULL";
    } else {
        throw utils::Exception("pass option value must be a boolean, integer, string, or null");
    }

}

/**
 * Load passes into the given pass group from a JSON array of pass descriptions.
 */
static void add_passes_from_json(
    const PassRef &group,
    const utils::Json &json,
    const utils::Map<utils::Str, utils::Str> &pass_default_options
) {

    // Shorthand.
    using JsonType = utils::Json::value_t;

    for (const auto &pass_description : json) {

        // Parse the JSON structure.
        utils::Str type;
        utils::Str name;
        utils::Map<utils::Str, utils::Str> options;
        utils::Map<utils::Str, utils::Str> sub_pass_default_options = pass_default_options;
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
                            options.set(opt_it.key()) = option_value_from_json(opt_it.value());
                        }
                    } else {
                        throw utils::Exception("pass options must be an object if specified");
                    }
#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
                } else if (it.key() == "group-options") {
                    if (it.value().type() == JsonType::object) {
                        for (auto opt_it = it->begin(); opt_it != it->end(); ++opt_it) {
                            sub_pass_default_options.set(opt_it.key()) = option_value_from_json(opt_it.value());
                        }
                    } else {
                        throw utils::Exception("group-options must be an object if specified");
                    }
                } else if (it.key() == "group") {
                    if (it.value().type() == JsonType::array) {
                        sub_passes = &it.value();
                    } else {
                        throw utils::Exception("pass group must be an array of pass descriptions");
                    }
#endif
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

        // Set pass options based on the pass-options and group-options keys
        // specified by parent passes.
        auto &opts = pass->get_options();
        for (const auto &it : pass_default_options) {

            // Silently ignore options that don't exist for this particular
            // pass.
            if (opts.has_option(it.first)) {

                // Only apply the option when it wasn't already set during
                // construction.
                if (!opts[it.first].is_set()) {
                    opts[it.first] = it.second;
                }

            }

        }

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
        // If the pass has sub-passes, construct it and add them by recursively
        // calling ourselves.
        if (sub_passes != nullptr) {
            pass->construct();
            if (!pass->is_group()) {
                throw utils::Exception("pass type " + type + " does not support sub-passes");
            }
            add_passes_from_json(pass, *sub_passes, sub_pass_default_options);
        }
#endif

    }
}

/**
 * Generates a map from (common!) pass option name to option value based on the
 * global options for backward-compatibility purposes.
 *
 * FIXME: there should probably be some way to namespace option names
 *  by pass type if we're going to be setting them like this. For
 *  now the option names are just named such that there are no
 *  conflicts, so we can make do. But it'd be a bit weird if all the
 *  option names for a particular pass start with the pass type name
 *  while configuring, so something more intelligent is needed, and that
 *  just doesn't exist right now. On the other hand, there shouldn't
 *  ever be any more global options like this, so maybe this is good
 *  enough.
 *  NB: also see the note in developer/docs/passes.rst
 */
static utils::Map<utils::Str, utils::Str> convert_global_to_pass_options() {
    utils::Map<utils::Str, utils::Str> retval;

    // Set output_prefix based on output_dir and unique_output.
    utils::StrStrm ss;
    ss << com::options::global["output_dir"].as_str() << "/";
    if (com::options::global["unique_output"].as_bool()) {
        ss << "%N";
    } else {
        ss << "%n";
    }
    ss << "_%P";
    retval.set("output_prefix") = ss.str();

    // Set the debug option based on write_qasm_files and
    // write_report_files.
    if (com::options::global["write_qasm_files"].as_bool()) {
        if (com::options::global["write_report_files"].as_bool()) {
            retval.set("debug") = "both";
        } else {
            retval.set("debug") = "qasm";
        }
    } else if (com::options::global["write_report_files"].as_bool()) {
        retval.set("debug") = "stats";
    }

    // Set options for the scheduler.
    const auto &scheduler = com::options::global["scheduler"];
    const auto &scheduler_uniform = com::options::global["scheduler_uniform"];
    if (scheduler.is_set() || scheduler_uniform.is_set()) {
        if (scheduler_uniform.as_bool()) {
            retval.set("scheduler_target") = "uniform";
        } else if (scheduler.as_str() == "ASAP") {
            retval.set("scheduler_target") = "asap";
        } else {
            retval.set("scheduler_target") = "alap";
        }
    }

    // Set options for both the scheduler and mapper (since the mapper has
    // a scheduler built into it, they share some options).
    const auto &scheduler_commute = com::options::global["scheduler_commute"];
    if (scheduler_commute.is_set()) {
        retval.set("commute_multi_qubit") = scheduler_commute.as_str();
    }
    const auto &scheduler_commute_rotations = com::options::global["scheduler_commute_rotations"];
    if (scheduler_commute_rotations.is_set()) {
        retval.set("commute_single_qubit") = scheduler_commute_rotations.as_str();
    }
    const auto &scheduler_heuristic = com::options::global["scheduler_heuristic"];
    if (scheduler_heuristic.is_set()) {
        retval.set("scheduler_heuristic") = scheduler_heuristic.as_str();
    }
    const auto &print_dot_graphs = com::options::global["print_dot_graphs"];
    if (print_dot_graphs.is_set()) {
        retval.set("write_dot_graphs") = print_dot_graphs.as_str();
    }

    // Set options for the mapper.
    const auto &initialplace = com::options::global["initialplace"];
    if (initialplace.is_set()) {
        if (initialplace.as_str() == "no") {
            retval.set("enable_mip_placer") = "no";
        } else {
            if (initialplace.as_str() != "yes") {
                QL_WOUT("timeout set for initial placement, but this is not supported right now");
            }
            retval.set("enable_mip_placer") = "yes";
        }
    }
    const auto &initialplace2qhorizon = com::options::global["initialplace2qhorizon"];
    if (initialplace2qhorizon.is_set()) {
        retval.set("mip_horizon") = initialplace2qhorizon.as_str();
    }
    const auto &mapper = com::options::global["mapper"];
    if (mapper.is_set() && mapper.as_str() != "no") {
        retval.set("route_heuristic") = mapper.as_str();
    }
    const auto &mapmaxalters = com::options::global["mapmaxalters"];
    if (mapmaxalters.is_set()) {
        retval.set("max_alternative_routes") = mapmaxalters.as_str();
    }
    const auto &mapinitone2one = com::options::global["mapinitone2one"];
    if (mapinitone2one.is_set()) {
        retval.set("initialize_one_to_one") = mapinitone2one.as_str();
    }
    const auto &mapassumezeroinitstate = com::options::global["mapassumezeroinitstate"];
    if (mapassumezeroinitstate.is_set()) {
        retval.set("assume_initialized") = mapassumezeroinitstate.as_str();
    }
    const auto &mapprepinitsstate = com::options::global["mapprepinitsstate"];
    if (mapprepinitsstate.is_set()) {
        retval.set("assume_prep_only_initializes") = mapprepinitsstate.as_str();
    }
    const auto &maplookahead = com::options::global["maplookahead"];
    if (maplookahead.is_set()) {
        retval.set("lookahead_mode") = maplookahead.as_str();
    }
    const auto &mappathselect = com::options::global["mappathselect"];
    if (mappathselect.is_set()) {
        retval.set("path_selection_mode") = mappathselect.as_str();
    }
    const auto &mapselectswaps = com::options::global["mapselectswaps"];
    if (mapselectswaps.is_set()) {
        retval.set("swap_selection_mode") = mapselectswaps.as_str();
    }
    const auto &maprecNN2q = com::options::global["maprecNN2q"];
    if (maprecNN2q.is_set()) {
        retval.set("recurse_on_nn_two_qubit") = maprecNN2q.as_str();
    }
    const auto &mapselectmaxlevel = com::options::global["mapselectmaxlevel"];
    if (mapselectmaxlevel.is_set()) {
        retval.set("recursion_depth_limit") = mapselectmaxlevel.as_str();
    }
    const auto &mapselectmaxwidth = com::options::global["mapselectmaxwidth"];
    if (mapselectmaxwidth.is_set()) {
        if (mapselectmaxwidth.as_str() == "min") {
            retval.set("recursion_width_factor") = "1.0";
        } else if (mapselectmaxwidth.as_str() == "minplusone") {
            retval.set("recursion_width_factor") = "1.0000000001";
        } else if (mapselectmaxwidth.as_str() == "minplushalfmin") {
            retval.set("recursion_width_factor") = "1.5";
        } else if (mapselectmaxwidth.as_str() == "minplusmin") {
            retval.set("recursion_width_factor") = "2.0";
        } else {
            retval.set("recursion_width_factor") = "100000000000";
        }
    }
    const auto &maptiebreak = com::options::global["maptiebreak"];
    if (maptiebreak.is_set()) {
        retval.set("tie_break_method") = maptiebreak.as_str();
    }
    const auto &mapusemoves = com::options::global["mapusemoves"];
    if (mapusemoves.is_set()) {
        retval.set("use_moves") = mapusemoves.as_str();
    }
    const auto &mapreverseswap = com::options::global["mapreverseswap"];
    if (mapreverseswap.is_set()) {
        retval.set("reverse_swap_if_better") = mapreverseswap.as_str();
    }

#if 0   // FIXME: removed, use pass options
    // Set options for CC backend.
    const auto &backend_cc_map_input_file = com::options::global["backend_cc_map_input_file"];
    if (backend_cc_map_input_file.is_set()) {
        retval.set("map_input_file") = backend_cc_map_input_file.as_str();
    }
    const auto &backend_cc_verbose = com::options::global["backend_cc_verbose"];
    if (backend_cc_verbose.is_set()) {
        retval.set("verbose") = backend_cc_verbose.as_str();
    }
    const auto &backend_cc_run_once = com::options::global["backend_cc_run_once"];
    if (backend_cc_run_once.is_set()) {
        retval.set("run_once") = backend_cc_run_once.as_str();
    }
#endif

    return retval;
}

/**
 * Constructs a pass manager based on the given JSON configuration. Refer
 * to dump_docs() for details.
 */
Manager Manager::from_json(
    const utils::Json &json,
    const Factory &factory
) {

    // Shorthand.
    using JsonType = utils::Json::value_t;

    // Check toplevel type.
    if (json.type() != JsonType::object) {
        throw utils::Exception("pass manager configuration must be an object");
    }

    // Read the strategy structure.
    utils::Str architecture = {};
    utils::Set<utils::Str> dnu = {};
    const utils::Json *pass_options = nullptr;
    utils::Bool compatibility_mode = false;
    const utils::Json *passes = nullptr;
    for (auto it = json.begin(); it != json.end(); ++it) {
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
        } else if (it.key() == "pass-options") {
            if (it.value().type() == JsonType::object) {
                pass_options = &it.value();
            } else {
                throw utils::Exception("strategy.pass-options must be an object");
            }
        } else if (it.key() == "compatibility-mode") {
            if (it.value().type() == JsonType::boolean) {
                compatibility_mode = it.value().get<utils::Bool>();
            } else {
                throw utils::Exception("strategy.compatibility-mode must be a boolean if specified");
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

    // Build the default pass options record.
    utils::Map<utils::Str, utils::Str> pass_default_options;
    if (compatibility_mode) {
        pass_default_options = convert_global_to_pass_options();
    }
    if (pass_options) {
        for (auto it = pass_options->begin(); it != pass_options->end(); ++it) {
            pass_default_options.set(it.key()) = option_value_from_json(it.value());
        }
    }

    // Construct the pass manager.
    Manager manager{architecture, dnu};

    // Add passes from the pass descriptions.
    add_passes_from_json(manager.get_root(), *passes, pass_default_options);

    return manager;
}

/**
 * Generate a pass manager with a strategy that aims to mimic the flow of
 * the OpenQL compiler as it was before pass management as closely as
 * possible. The actual pass list is derived from the eqasm_compiler key
 * in the configuration file and from the global options (similar to the
 * "compatibility-mode" key in the JSON strategy definition format).
 */
Manager Manager::from_defaults(const ir::compat::PlatformRef &platform) {

    // If the platform includes a compiler configuration JSON object, load from
    // that.
    if (platform->compiler_settings.type() == utils::Json::value_t::object) {
        return from_json(platform->compiler_settings);
    }

    // Otherwise, generate a default manager.
    Manager manager;

    // Generate preprocessing passes that are common to all eqasm_compiler
    // names.
    manager.append_pass(
        "io.cqasm.Report",
        "initialqasmwriter",
        {
            {"output_prefix", com::options::global["output_dir"].as_str() + "/%N"},
            {"output_suffix", ".qasm"},
            {"with_timing", "no"}
        }
    );
    if (com::options::global["clifford_prescheduler"].as_bool()) {
        manager.append_pass(
            "opt.clifford.Optimize",
            "clifford_prescheduler"
        );
    }
    if (com::options::global["prescheduler"].as_bool()) {
        if (
            com::options::global["scheduler_uniform"].as_bool() ||
            com::options::global["scheduler_heuristic"].is_set()
        ) {
            manager.append_pass(
                "sch.Schedule",
                "prescheduler",
                {
                    {"resource_constraints", "no"}
                }
            );
        } else {
            manager.append_pass(
                "sch.ListSchedule",
                "prescheduler",
                {
                    {"resource_constraints", "no"},
                    {"scheduler_heuristic", "none"}
                }
            );
        }
    }
    if (com::options::global["clifford_postscheduler"].as_bool()) {
        manager.append_pass(
            "opt.clifford.Optimize",
            "clifford_postscheduler"
        );
    }
    manager.append_pass(
        "io.cqasm.Report",
        "scheduledqasmwriter",
        {
            {"output_prefix", com::options::global["output_dir"].as_str() + "/%N"},
            {"output_suffix", "_scheduled.qasm"}
        }
    );

    // Generate the backend passes.
    platform->architecture->populate_backend_passes(manager);

#if 0   // FIXME: deprecated
    // Sweep point writing (whatever that is) was done hardcoded at the end of
    // even the initial pass manager implementation. Now it's an actual pass.
    manager.append_pass(
        "io.sweep_points.Write",
        "config"
    );
#endif

    // Set the pass options using the compatibility mode option name/value
    // converter.
    auto options = convert_global_to_pass_options();
    for (const auto &pass : manager.get_passes()) {
        for (const auto &it : options) {
            const auto &option = it.first;
            const auto &value = it.second;
            if (pass->get_options().has_option(option)) {
                if (!pass->get_options()[option].is_set()) {
                    pass->set_option(option, value);
                }
            }
        }
    }

    return manager;
}


/**
 * Returns a reference to the root pass group.
 */
const PassRef &Manager::get_root() {
    return root;
}

/**
 * Returns a reference to the root pass group.
 */
CPassRef Manager::get_root() const {
    return root.as_const();
}

/**
 * Dumps documentation for all available pass types, as well as the option
 * documentation for the passes.
 */
void Manager::dump_pass_types(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    Factory::dump_pass_types(pass_factory, os, line_prefix);
}

/**
 * Dumps the currently configured compilation strategy to the given stream.
 */
void Manager::dump_strategy(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    root->dump_strategy(os, line_prefix);
}

/**
 * Sets a pass option. Periods are used as hierarchy separators; the last
 * element will be the option name, and the preceding elements represent
 * pass instance names. Furthermore, wildcards may be used for the pass name
 * elements (asterisks for zero or more characters and a question mark for a
 * single character) to select multiple or all immediate sub-passes of that
 * group, and a double asterisk may be used for the element before the
 * option name to chain to set_option_recursively() instead. The return
 * value is the number of passes that were affected; passes are only
 * affected when they are selected by the option path AND have an option
 * with the specified name. If must_exist is set an exception will be thrown
 * if none of the passes were affected, otherwise 0 will be returned.
 */
utils::UInt Manager::set_option(
    const utils::Str &path,
    const utils::Str &value,
    utils::Bool must_exist
) {
    return root->set_option(path, value, must_exist);
}

/**
 * Sets an option for all passes recursively. The return value is the number
 * of passes that were affected; passes are only affected when they have an
 * option with the specified name. If must_exist is set an exception will be
 * thrown if none of the passes were affected, otherwise 0 will be returned.
 */
utils::UInt Manager::set_option_recursively(
    const utils::Str &option,
    const utils::Str &value,
    utils::Bool must_exist
) {
    return root->set_option_recursively(option, value, must_exist);
}

/**
 * Returns the current value of an option. Periods are used as hierarchy
 * separators; the last element will be the option name, and the preceding
 * elements represent pass instance names.
 */
const utils::Option &Manager::get_option(const utils::Str &path) const {
    return root->get_option(path);
}

/**
 * Appends a pass to the end of the pass list. If type_name is empty
 * or unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
PassRef Manager::append_pass(
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
PassRef Manager::prefix_pass(
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
 * constructed pass. Periods may be used in target to traverse deeper into
 * the pass hierarchy.
 */
PassRef Manager::insert_pass_after(
    const utils::Str &target,
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    return root->insert_sub_pass_after(target, type_name, instance_name, options);
}

/**
 * Inserts a pass immediately before the target pass (named by instance). If
 * target does not exist, an exception is thrown. If type_name is empty or
 * unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass. Periods may be used in target to traverse deeper into
 * the pass hierarchy.
 */
PassRef Manager::insert_pass_before(
    const utils::Str &target,
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    return root->insert_sub_pass_before(target, type_name, instance_name, options);
}

/**
 * Looks for the pass with the target instance name, and embeds it into a
 * newly generated group. The group will assume the name of the original
 * pass, while the original pass will be renamed as specified by sub_name.
 * Note that this ultimately does not modify the pass order. If target does
 * not exist or this pass is not a group of sub-passes, an exception is
 * thrown. Returns a reference to the constructed group. Periods may be used
 * in target to traverse deeper into the pass hierarchy.
 */
PassRef Manager::group_pass(
    const utils::Str &target,
    const utils::Str &sub_name
) {
    return root->group_sub_pass(target, sub_name);
}

/**
 * Like group_pass(), but groups an inclusive range of passes into a
 * group with the given name, leaving the original pass names unchanged.
 * Periods may be used in from/to to traverse deeper into the pass
 * hierarchy, but the hierarchy prefix must be the same for from and to.
 */
PassRef Manager::group_passes(
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
 * not an unconditional group, an exception is thrown. Periods may be used
 * in target to traverse deeper into the pass hierarchy.
 */
void Manager::flatten_subgroup(
    const utils::Str &target,
    const utils::Str &name_prefix
) {
    root->flatten_subgroup(target, name_prefix);
}

/**
 * Returns a reference to the pass with the given instance name. If no such
 * pass exists, an exception is thrown. Periods may be used as hierarchy
 * separators to get nested sub-passes.
 */
PassRef Manager::get_pass(const utils::Str &target) const {
    return root->get_sub_pass(target);
}

/**
 * Returns whether a pass with the target instance name exists. Periods may be
 * used in target to traverse deeper into the pass hierarchy.
 */
utils::Bool Manager::does_pass_exist(const utils::Str &target) const {
    return root->does_sub_pass_exist(target);
}

/**
 * Returns the total number of passes in the root hierarchy.
 */
utils::UInt Manager::get_num_passes() const {
    return root->get_num_sub_passes();
}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the list containing all the sub-passes. Otherwise, an exception is
 * thrown.
 */
const utils::List<PassRef> &Manager::get_passes() const {
    return root->get_sub_passes();
}

/**
 * Returns an indexable list of references to all passes with the given
 * type within the root hierarchy.
 */
utils::Vec<PassRef> Manager::get_sub_passes_by_type(const utils::Str &target) const {
    return root->get_sub_passes_by_type(target);
}

/**
 * Removes the pass with the given target instance name, or throws an
 * exception if no such pass exists.
 */
void Manager::remove_pass(const utils::Str &target) {
    return root->remove_sub_pass(target);
}

/**
 * Clears the entire pass list.
 */
void Manager::clear_passes() {
    return root->clear_sub_passes();
}

/**
 * Constructs all passes recursively. This freezes the pass options, but
 * allows subtrees to be modified.
 */
void Manager::construct() {
    root->construct_recursive();
}

/**
 * Executes this pass or pass group on the given platform and program.
 */
void Manager::compile(const ir::Ref &ir) {

    // Ensure that all passes are constructed.
    construct();

    // Compile the program.
    root->compile(ir, "");

}

} // namespace pmgr
} // namespace ql
