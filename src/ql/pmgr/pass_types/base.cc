/** \file
 * Defines the base classes for all passes.
 */

#include "ql/pmgr/pass_types/base.h"

#include <cctype>
#include <regex>
#include "ql/utils/filesystem.h"
#include "ql/ir/cqasm/write.h"
#include "ql/pmgr/manager.h"
#include "ql/pass/ana/statistics/report.h"

namespace ql {
namespace pmgr {
namespace pass_types {

/**
 * Returns whether the given user-specified name is a valid pass name.
 */
void Base::check_pass_name(
    const utils::Str &name,
    const utils::Map<utils::Str, Ref> &existing_pass_names
) {
    static const std::regex name_re{"[a-zA-Z0-9_\\-]+"};
    if (!std::regex_match(name, name_re)) {
        throw utils::Exception("pass name \"" + name + "\" is invalid");
    }
    if (existing_pass_names.count(name)) {
        throw utils::Exception("duplicate pass name \"" + name + "\"");
    }
}

/**
 * Returns a unique name generated from the given type name.
 */
utils::Str Base::generate_valid_pass_name(const utils::Str &type_name) const {

    // The type name will have type hierarchy separators (periods), and the
    // final entry will be TitleCase. However, instance names are normally
    // lower_case. So, we replace periods with underscores, and insert an
    // underscore before groups of uppercase characters.
    auto reference_name = type_name;
    if (type_name.empty()) {
        reference_name = "group";
    }
    utils::Str sub_instance_name;
    char prev = '_';
    for (char cur : reference_name) {
        if (cur == '.') {
            cur = '_';
        }
        if (std::isupper(cur) && !std::isupper(prev) && prev != '_') {
            sub_instance_name += '_';
        }
        sub_instance_name += std::tolower(cur);
        prev = cur;
    }

    // If the generated name doesn't exist yet, use it as is.
    if (sub_pass_names.find(sub_instance_name) == sub_pass_names.end()) {
        return sub_instance_name;
    }

    // Append numbers until we find a name that we haven't used yet.
    utils::UInt uniquifier = 1;
    while (true) {
        utils::Str uniquified_sub_instance_name = sub_instance_name + "_" + utils::to_string(uniquifier);
        if (sub_pass_names.find(uniquified_sub_instance_name) == sub_pass_names.end()) {
            return uniquified_sub_instance_name;
        }
        uniquifier++;
    }
}

/**
 * Makes a new pass. Used by the various functions that add passes.
 */
Ref Base::make_pass(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) const {

    // Generate a name if no name is provided.
    auto gen_instance_name = instance_name;
    if (instance_name.empty()) {
        gen_instance_name = generate_valid_pass_name(type_name);
    }

    // Check the name.
    check_pass_name(gen_instance_name, sub_pass_names);

    // Construct the pass.
    Ref pass = Factory::build_pass(pass_factory, type_name, gen_instance_name);

    // Set initial options.
    for (const auto &opt : options) {
        pass->set_option(opt.first, opt.second);
    }

    return pass;
}

/**
 * Returns an iterator for the sub_pass_order list corresponding with the
 * given target instance name, or throws an exception if no such pass is
 * found.
 */
utils::List<Ref>::iterator Base::find_pass(const utils::Str &target) {
    for (auto it = sub_pass_order.begin(); it != sub_pass_order.end(); ++it) {
        if ((*it)->get_name() == target) {
            return it;
        }
    }
    throw utils::Exception("pass with name \"" + target + "\" not found");
}

/**
 * Checks whether access to the sub-pass list is allowed. Throws an
 * exception if not.
 */
void Base::check_group_access_allowed() const {
    if (!is_group()) {
        throw utils::Exception("cannot access sub-pass list before construct() or for normal pass");
    }
}

/**
 * Checks whether access to the condition is allowed. Throws an exception
 * if not.
 */
void Base::check_condition_access_allowed() const {
    if (!is_conditional()) {
        throw utils::Exception("cannot access condition for non-conditional pass");
    }
}

/**
 * Constructs the abstract pass. No error checking here; this is up to the
 * parent pass group.
 */
Base::Base(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &type_name,
    const utils::Str &instance_name
) :
    pass_factory(pass_factory),
    type_name(type_name),
    instance_name(instance_name)
{
    options.add_str(
        "output_prefix",
        "Format string for the prefix used for all output products. "
        "`%n` is substituted with the user-specified name of the program. "
        "`%N` is substituted with the optionally uniquified name of the program. "
        "`%p` is substituted with the local name of the pass within its group. "
        "`%P` is substituted with the fully-qualified name of the pass, using "
        "periods as hierarchy separators (guaranteed unique). "
        "`%U` is substituted with the fully-qualified name of the pass, using "
        "underscores as hierarchy separators. This may not be completely unique,"
        "`%D` is substituted with the fully-qualified name of the pass, using "
        "slashes as hierarchy separators. "
        "Any directories that don't exist will be created as soon as an output "
        "file is written.",
        "%N.%P"
    );
    options.add_enum(
        "debug",
        "May be used to implicitly surround this pass with cQASM/report file "
        "output printers, to aid in debugging. Set to `no` to disable this "
        "functionality or to `yes` to write a tree dump and a cQASM file before "
        "and after, the latter of which includes statistics as comments. The "
        "filename is built using the output_prefix option, using suffix "
        "`_debug_[in|out].ir` for the IR dump, and `_debug_[in|out].cq` for "
        "the cQASM file. The option values `stats`, `cqasm`, and `both` are "
        "used for backward compatibility with the `write_qasm_files` and "
        "`write_report_files` global options; for `stats` and `both` a "
        "statistics report file is written with suffix `_[in|out].report`, "
        "and for `qasm` and `both` a cQASM file is written (without stats "
        "in the comments) with suffix `_[in|out].qasm`.",
        "no",
        {"no", "yes", "stats", "qasm", "both"}
    );
}

/**
 * Returns whether this is a legacy pass, i.e. one that operates on the old
 * IR. Returns false unless overridden.
 */
utils::Bool Base::is_legacy() const {
    return false;
}

/**
 * Returns `pass "<name>"` for normal passes and `root` for the root pass.
 * Used for error messages.
 */
utils::Str Base::describe() const {
    if (is_root()) {
        return "root";
    } else {
        return "pass \"" + instance_name + "\"";
    }
}

/**
 * Returns the full, desugared type name that this pass was constructed
 * with.
 */
const utils::Str &Base::get_type() const {
    return type_name;
}

/**
 * Returns the instance name of the pass within the surrounding group.
 */
const utils::Str &Base::get_name() const {
    return instance_name;
}

/**
 * Dumps the documentation for this pass to the given stream.
 */
void Base::dump_help(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (is_legacy()) {
        utils::dump_str(os, line_prefix, R"(
        NOTE: this is a legacy pass, operating on the old intermediate
        representation. If the program is using features that the old IR does
        not support when this pass is run, an internal compiler error will be
        thrown. Furthermore, kernel/block names may change regardless of whether
        the pass does anything with them, due to name uniquification logic.
        )");
        os << line_prefix << "\n";
    }
    dump_docs(os, line_prefix);
    os << line_prefix << "\n";
    os << line_prefix << "* Options *\n";
    os << line_prefix << "\n";
    options.dump_help(os, line_prefix + "  ");
}

/**
 * Dumps the current state of the options to the given stream. If only_set
 * is set to true, only the options that were explicitly configured are
 * dumped.
 */
void Base::dump_options(
    utils::Bool only_set,
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    options.dump_options(only_set, os, line_prefix);
}

/**
 * Dumps the entire compilation strategy including configured options of
 * this pass and all sub-passes.
 */
void Base::dump_strategy(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::Str first_indent, indent;
    if (!is_root()) {
        os << line_prefix << "- " << instance_name;
        if (!type_name.empty()) {
            os << ": " << type_name << "\n";
        }
        options.dump_options(true, os, line_prefix + "   |- ");
        first_indent = line_prefix + "   '- ";
        indent = line_prefix + "      ";
    } else {
        first_indent = line_prefix;
        indent = line_prefix;
    }
    if (is_group()) {
        switch (node_type) {
            case NodeType::GROUP_IF:
                os << first_indent << "if " << condition->to_string() << ":\n";
                break;
            case NodeType::GROUP_WHILE:
                os << first_indent << "while " << condition->to_string() << ":\n";
                break;
            case NodeType::GROUP_REPEAT_UNTIL_NOT:
                os << first_indent << "repeat:\n";
                break;
            default:
                if (!is_root()) {
                    os << first_indent << "passes:\n";
                }
                break;
        }
        for (const auto &pass : sub_pass_order) {
            if (is_root()) {
                pass->dump_strategy(os, indent);
            } else {
                pass->dump_strategy(os, indent + " |");
            }
        }
        if (node_type == NodeType::GROUP_REPEAT_UNTIL_NOT) {
            os << indent << "until " << condition->to_string() << "\n";
        }
    }
    os << line_prefix << std::endl;
}

/**
 * Sets an option. Periods may be used as hierarchy separators to set
 * options for sub-passes; the last element will be the option name, and the
 * preceding elements represent pass instance names. Furthermore, wildcards
 * may be used for the pass name elements (asterisks for zero or more
 * characters and a question mark for a single character) to select multiple
 * or all immediate sub-passes of that group, and a double asterisk may be
 * used for the element before the option name to chain to
 * set_option_recursively() instead. The return value is the number of
 * passes that were affected; passes are only affected when they are
 * selected by the option path AND have an option with the specified name.
 * If must_exist is set an exception will be thrown if none of the passes
 * were affected, otherwise 0 will be returned.
 */
utils::UInt Base::set_option(
    const utils::Str &option,
    const utils::Str &value,
    utils::Bool must_exist
) {

    // Handle double-asterisk recursive logic.
    if (option.rfind("**.", 0) == 0) {
        utils::Str option_name = option.substr(3);
        if (!is_constructed()) {

            // Pass not constructed; set option on this pass.
            if (must_exist && !options.has_option(option_name)) {
                throw utils::Exception(
                    "option " + option_name + " does not exist for " + describe()
                );
            }
            if (!is_constructed() && options.has_option(option_name)) {
                options[option_name] = value;
                return 1;
            } else {
                return 0;
            }

        } else if (!is_group()) {

            // Pass constructed but isn't a group, can't do anything.
            if (must_exist) {
                throw utils::Exception(
                    "cannot set option " + option_name + " on " + describe()
                    + " anymore, because the pass has already been constructed"
                );
            }
            return 0;

        } else {

            // Call recursively for all sub-passes.
            utils::UInt passes_affected = 0;
            for (auto &pass : sub_pass_order) {
                passes_affected += pass->set_option(option, value, false);
            }
            if (must_exist && !passes_affected) {
                throw utils::Exception(
                    "option " + option_name + " could not be set on any sub-pass of "
                    + describe()
                );
            }
            return passes_affected;

        }
    }

    // Handle setting an option on this pass.
    auto period = option.find('.');
    if (period == utils::Str::npos) {
        if (must_exist && is_constructed()) {
            throw utils::Exception(
                "cannot modify pass option after pass construction"
            );
        }
        if (must_exist && !options.has_option(option)) {
            throw utils::Exception(
                "option " + option + " does not exist for " + describe()
            );
        }
        if (!is_constructed() && options.has_option(option)) {
            options[option] = value;
            return 1;
        } else {
            return 0;
        }
    }

    // Handle setting options on sub-passes by pattern-matching.
    if (!is_constructed()) {
        throw utils::Exception(
            "cannot set sub-pass options before parent pass ("
            + describe() + ") is constructed"
        );
    }
    if (!is_group()) {
        throw utils::Exception(
            "cannot set sub-pass options for non-group " + describe()
        );
    }
    utils::Str sub_pass_pattern = option.substr(0, period);
    utils::Str sub_option = option.substr(period + 1);

    // Handle setting an option on all sub-passes.
    utils::Bool any_sub_passes_matched = false;
    utils::UInt passes_affected = 0;
    for (auto &pass : sub_pass_order) {
        if (utils::pattern_match(sub_pass_pattern, pass->instance_name)) {
            any_sub_passes_matched = true;
            passes_affected += pass->set_option(sub_option, value, false);
        }
    }
    if (must_exist) {
        if (!any_sub_passes_matched) {
            throw utils::Exception(
                "pattern " + sub_pass_pattern + " did not match any sub-passes of "
                + describe()
            );
        } else if (!passes_affected) {
            throw utils::Exception(
                "option " + sub_option + " could not be set on any matching sub-pass of "
                + describe()
            );
        }
    }
    return passes_affected;

}

/**
 * Sets an option for all sub-passes recursively. The return value is the
 * number of passes that were affected; passes are only affected when they
 * have an option with the specified name. If must_exist is set an exception
 * will be thrown if none of the passes were affected, otherwise 0 will be
 * returned.
 */
utils::UInt Base::set_option_recursively(
    const utils::Str &option,
    const utils::Str &value,
    utils::Bool must_exist
) {
    return set_option("**." + option, value, must_exist);
}

/**
 * Returns the current value of an option. Periods may be used as hierarchy
 * separators to get options from sub-passes (if any).
 */
const utils::Option &Base::get_option(const utils::Str &option) const {
    auto period = option.find('.');

    // Handle getting an option from this pass.
    if (period == utils::Str::npos) {
        return options[option];
    }

    // Handle getting an option from a sub-pass.
    utils::Str sub_pass = option.substr(0, period);
    utils::Str sub_option = option.substr(period + 1);
    auto sub_pass_it = sub_pass_names.find(sub_pass);
    if (sub_pass_it == sub_pass_names.end()) {
        throw utils::Exception("no sub-pass with name \"" + sub_pass + "\" in " + describe());
    }
    return sub_pass_it->second->get_option(sub_option);

}

/**
 * Returns mutable access to the embedded options object. This is allowed only
 * until construct() is called.
 */
utils::Options &Base::get_options() {
    if (is_constructed()) {
        throw utils::Exception("cannot modify pass option after pass construction");
    }
    return options;
}

/**
 * Returns read access to the embedded options object.
 */
const utils::Options &Base::get_options() const {
    return options;
}

/**
 * Constructs this pass. During construction, the pass implementation may
 * decide, based on its options, to become a group of passes or a normal
 * pass. If it decides to become a group, the group may be introspected or
 * modified by the user. The options are frozen after this, so set_option()
 * will start throwing exceptions when called. construct() may be called any
 * number of times, but becomes no-op after the first call.
 */
void Base::construct() {

    // If we've already been constructed, don't do it again.
    if (is_constructed()) {
        return;
    }

    // Run the construction implementation.
    utils::List<Ref> constructed_pass_order;
    condition::Ref constructed_condition;
    auto constructed_node_type = on_construct(
        pass_factory,
        constructed_pass_order,
        constructed_condition
    );

    // Check basic postconditions.
    switch (constructed_node_type) {
        case NodeType::NORMAL:
            QL_ASSERT(constructed_pass_order.empty());
            QL_ASSERT(!constructed_condition.has_value());
            break;
        case NodeType::GROUP:
            QL_ASSERT(!constructed_condition.has_value());
            break;
        case NodeType::GROUP_IF:
        case NodeType::GROUP_WHILE:
        case NodeType::GROUP_REPEAT_UNTIL_NOT:
            QL_ASSERT(constructed_condition.has_value());
            break;
        default:
            QL_ASSERT(false);
    }

    // Check validity and uniqueness of the names, and build the name to pass
    // map.
    utils::Map<utils::Str, Ref> constructed_pass_names;
    for (const auto &pass : constructed_pass_order) {
        check_pass_name(pass->get_name(), constructed_pass_names);
        constructed_pass_names.set(pass->get_name()) = pass;
    }

    // Commit the results.
    node_type = constructed_node_type;
    sub_pass_order = std::move(constructed_pass_order);
    sub_pass_names = std::move(constructed_pass_names);
    condition = std::move(constructed_condition);

}

/**
 * Recursively constructs this pass and all its sub-passes (if it constructs
 * or previously constructed into a group).
 */
void Base::construct_recursive(
    const utils::Str &pass_name_prefix
) {

    // Construct ourself.
    auto full_name = pass_name_prefix + instance_name;
    construct();

    // If we constructed into a group, construct the sub-passes recursively.
    if (is_group()) {
        for (const auto &pass : sub_pass_order) {
            pass->construct_recursive(
                full_name.empty() ? "" : (full_name + ".")
            );
        }
    }
}

/**
 * Returns whether this pass has been constructed yet.
 */
utils::Bool Base::is_constructed() const {
    return node_type != NodeType::UNKNOWN;
}

/**
 * Returns whether this pass has configurable sub-passes.
 */
utils::Bool Base::is_group() const {
    return node_type == NodeType::GROUP
        || node_type == NodeType::GROUP_IF
        || node_type == NodeType::GROUP_WHILE
        || node_type == NodeType::GROUP_REPEAT_UNTIL_NOT;
}

/**
 * Returns whether this pass is a simple group of which the sub-passes can
 * be collapsed into the parent pass group without affecting the strategy.
 */
utils::Bool Base::is_collapsible() const {
    return node_type == NodeType::GROUP;
}

/**
 * Returns whether this is the root pass group in a pass manager.
 */
utils::Bool Base::is_root() const {
    return instance_name.empty();
}

/**
 * Returns whether this pass contains a conditionally-executed group.
 */
utils::Bool Base::is_conditional() const {
    return node_type == NodeType::GROUP_IF
        || node_type == NodeType::GROUP_WHILE
        || node_type == NodeType::GROUP_REPEAT_UNTIL_NOT;
}

/**
 * If this pass constructed into a group of passes, appends a pass to the
 * end of its pass list. Otherwise, an exception is thrown. If type_name is
 * empty or unspecified, a generic subgroup is added. Returns a reference to
 * the constructed pass.
 */
Ref Base::append_sub_pass(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    check_group_access_allowed();
    Ref pass = make_pass(type_name, instance_name, options);
    sub_pass_order.push_back(pass);
    sub_pass_names.set(pass->instance_name) = pass;
    return pass;
}

/**
 * If this pass constructed into a group of passes, appends a pass to the
 * beginning of its pass list. Otherwise, an exception is thrown. If
 * type_name is empty or unspecified, a generic subgroup is added. Returns a
 * reference to the constructed pass.
 */
Ref Base::prefix_sub_pass(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    check_group_access_allowed();
    Ref pass = make_pass(type_name, instance_name, options);
    sub_pass_order.push_front(pass);
    sub_pass_names.set(pass->instance_name) = pass;
    return pass;
}

/**
 * If this pass constructed into a group of passes, inserts a pass
 * immediately after the target pass (named by instance). If target does not
 * exist or this pass is not a group of sub-passes, an exception is thrown.
 * If type_name is empty or unspecified, a generic subgroup is added.
 * Returns a reference to the constructed pass. Periods may be used in
 * target to traverse deeper into the pass hierarchy.
 */
Ref Base::insert_sub_pass_after(
    const utils::Str &target,
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    check_group_access_allowed();

    // Handle hierarchy separators.
    auto period = target.find('.');
    if (period != utils::Str::npos) {
        auto sub_group = get_sub_pass(target.substr(0, period));
        return sub_group->insert_sub_pass_after(
            target.substr(period + 1),
            type_name, instance_name, options);
    }

    auto it = find_pass(target);
    Ref pass = make_pass(type_name, instance_name, options);
    sub_pass_order.insert(std::next(it), pass);
    sub_pass_names.set(pass->instance_name) = pass;
    return pass;
}

/**
 * If this pass constructed into a group of passes, inserts a pass
 * immediately before the target pass (named by instance). If target does
 * not exist or this pass is not a group of sub-passes, an exception is
 * thrown. If type_name is empty or unspecified, a generic subgroup is
 * added. Returns a reference to the constructed pass. Periods may be used
 * in target to traverse deeper into the pass hierarchy.
 */
Ref Base::insert_sub_pass_before(
    const utils::Str &target,
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    check_group_access_allowed();

    // Handle hierarchy separators.
    auto period = target.find('.');
    if (period != utils::Str::npos) {
        auto sub_group = get_sub_pass(target.substr(0, period));
        return sub_group->insert_sub_pass_before(
            target.substr(period + 1),
            type_name, instance_name, options);
    }

    auto it = find_pass(target);
    Ref pass = make_pass(type_name, instance_name, options);
    sub_pass_order.insert(it, pass);
    sub_pass_names.set(pass->instance_name) = pass;
    return pass;
}

/**
 * If this pass constructed into a group of passes, looks for the pass with
 * the target instance name, and embeds it into a newly generated group. The
 * group will assume the name of the original pass, while the original pass
 * will be renamed as specified by sub_name. Note that this ultimately does
 * not modify the pass order. If target does not exist or this pass is not a
 * group of sub-passes, an exception is thrown. Returns a reference to the
 * constructed group. Periods may be used in target to traverse deeper into
 * the pass hierarchy.
 */
Ref Base::group_sub_pass(
    const utils::Str &target,
    const utils::Str &sub_name
) {
    check_group_access_allowed();

    // Handle hierarchy separators.
    auto period = target.find('.');
    if (period != utils::Str::npos) {
        auto sub_group = get_sub_pass(target.substr(0, period));
        return sub_group->group_sub_pass(target.substr(period + 1), sub_name);
    }

    // Find and remove the target pass from the pass list.
    auto it = find_pass(target);
    Ref pass = *it;
    it = sub_pass_order.erase(it);
    sub_pass_names.erase(pass->instance_name);

    // Create a group with the same name as the target pass.
    Ref group = make_pass("", pass->instance_name, {});
    QL_ASSERT(group->is_group());
    QL_ASSERT(group->is_constructed());
    QL_ASSERT(group->sub_pass_order.empty());
    sub_pass_order.insert(it, group);
    sub_pass_names.set(group->instance_name) = group;

    // Rename the child pass and add it to the group.
    pass->instance_name = sub_name;
    group->sub_pass_order.push_back(pass);
    group->sub_pass_names.set(pass->instance_name) = pass;

    return group;
}

/**
 * Like group_sub_pass(), but groups an inclusive range of passes into a
 * group with the given name, leaving the original pass names unchanged.
 * Periods may be used in from/to to traverse deeper into the pass
 * hierarchy, but the hierarchy prefix must be the same for from and to.
 */
Ref Base::group_sub_passes(
    const utils::Str &from,
    const utils::Str &to,
    const utils::Str &group_name
) {
    check_group_access_allowed();

    // Handle hierarchy separators.
    auto period_from = from.find('.');
    auto period_to = to.find('.');
    if (period_from != utils::Str::npos && period_to != utils::Str::npos) {
        if (from.substr(0, period_from) != to.substr(0, period_to)) {
            throw utils::Exception("hierarchy prefix must be the same for both from and to");
        }
        auto sub_group = get_sub_pass(from.substr(0, period_from));
        return sub_group->group_sub_passes(
            from.substr(period_from + 1),
            to.substr(period_to + 1),
            group_name
        );
    } else if (period_from != utils::Str::npos || period_to != utils::Str::npos) {
        throw utils::Exception("hierarchy prefix must be the same for both from and to");
    }

    // Get the pass range as iterators.
    auto begin = find_pass(from);
    auto end = std::next(find_pass(to));

    // Create a group for the sub-passes.
    Ref group = make_pass("", group_name, {});
    QL_ASSERT(group->is_group());
    QL_ASSERT(group->is_constructed());
    QL_ASSERT(group->sub_pass_order.empty());

    // Copy the passes in the range into the group, and remove the passes from
    // our name lookup
    for (auto it = begin; it != end; ++it) {
        auto pass = *it;
        group->sub_pass_order.push_back(pass);
        group->sub_pass_names.set(pass->instance_name) = pass;
        sub_pass_names.erase(pass->instance_name);
    }

    // Erase the passes from our pass list.
    auto it = sub_pass_order.erase(begin, end);

    // Add the group where we just removed the pass range.
    sub_pass_order.insert(it, group);
    sub_pass_names.set(group->instance_name) = group;

    return group;
}

/**
 * If this pass constructed into a group of passes, looks for the pass with
 * the target instance name, treats it as a generic group, and flattens its
 * contained passes into the list of sub-passes of its parent. The names of
 * the passes found in the collapsed subgroup are prefixed with name_prefix
 * before they are added to the parent group. Note that this ultimately does
 * not modify the pass order. If target does not exist, does not construct
 * into a group of passes (construct() is called automatically), or this
 * pass is not a group of sub-passes, an exception is thrown. Periods may be
 * used in target to traverse deeper into the pass hierarchy.
 */
void Base::flatten_subgroup(
    const utils::Str &target,
    const utils::Str &name_prefix
) {
    check_group_access_allowed();

    // Handle hierarchy separators.
    auto period = target.find('.');
    if (period != utils::Str::npos) {
        auto sub_group = get_sub_pass(target.substr(0, period));
        sub_group->flatten_subgroup(target.substr(period + 1), name_prefix);
    }

    // Find the target, ensure that it's a simple group, and then remove it.
    auto it = find_pass(target);
    auto group = *it;
    if (!group->is_constructed()) {
        throw utils::Exception("cannot collapse pass that isn't constructed yet");
    }
    if (!group->is_collapsible()) {
        throw utils::Exception("cannot collapse pass that isn't a simple group");
    }
    it = sub_pass_order.erase(it);
    sub_pass_names.erase(group->instance_name);

    // Rename and insert the passes from the group where we found the group.
    for (const auto &pass : group->sub_pass_order) {
        pass->instance_name = name_prefix + pass->instance_name;
        check_pass_name(pass->instance_name, sub_pass_names);
        sub_pass_order.insert(it, pass);
        sub_pass_names.set(pass->instance_name) = pass;
        ++it;
    }

}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the pass with the given instance name. If target does not exist or this
 * pass is not a group of sub-passes, an exception is thrown. Periods may be
 * used as hierarchy separators to get nested sub-passes.
 */
Ref Base::get_sub_pass(const utils::Str &target) const {
    check_group_access_allowed();

    // Handle hierarchy separators.
    auto period = target.find('.');
    if (period != utils::Str::npos) {
        auto sub_group = get_sub_pass(target.substr(0, period));
        return sub_group->get_sub_pass(target.substr(period + 1));
    }

    return sub_pass_names.get(target);
}

/**
 * If this pass constructed into a group of passes, returns whether a
 * sub-pass with the target instance name exists. Otherwise, an exception is
 * thrown. Periods may be used in target to traverse deeper into the pass
 * hierarchy.
 */
utils::Bool Base::does_sub_pass_exist(const utils::Str &target) const {
    check_group_access_allowed();

    // Handle hierarchy separators.
    auto period = target.find('.');
    if (period != utils::Str::npos) {
        auto sub_group = get_sub_pass(target.substr(0, period));
        return sub_group->does_sub_pass_exist(target.substr(period + 1));
    }

    return sub_pass_names.count(target) > 0;
}

/**
 * If this pass constructed into a group of passes, returns the total number
 * of immediate sub-passes. Otherwise, an exception is thrown.
 */
utils::UInt Base::get_num_sub_passes() const {
    check_group_access_allowed();
    return sub_pass_order.size();
}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the list containing all the sub-passes. Otherwise, an exception is
 * thrown.
 */
const utils::List<Ref> &Base::get_sub_passes() const {
    check_group_access_allowed();
    return sub_pass_order;
}

/**
 * If this pass constructed into a group of passes, returns an indexable
 * list of references to all immediate sub-passes with the given type.
 * Otherwise, an exception is thrown.
 */
utils::Vec<Ref> Base::get_sub_passes_by_type(const utils::Str &target) const {
    check_group_access_allowed();
    utils::Vec<Ref> retval;
    for (const auto &pass : sub_pass_order) {
        if (pass->type_name == target) {
            retval.push_back(pass);
        }
    }
    return retval;
}

/**
 * If this pass constructed into a group of passes, removes the sub-pass
 * with the target instance name. If target does not exist or this pass is
 * not a group of sub-passes, an exception is thrown. Periods may be used in
 * target to traverse deeper into the pass hierarchy.
 */
void Base::remove_sub_pass(const utils::Str &target) {
    check_group_access_allowed();

    // Handle hierarchy separators.
    auto period = target.find('.');
    if (period != utils::Str::npos) {
        auto sub_group = get_sub_pass(target.substr(0, period));
        sub_group->remove_sub_pass(target.substr(period + 1));
    }

    auto it = find_pass(target);
    Ref pass = *it;
    sub_pass_order.erase(it);
    sub_pass_names.erase(pass->instance_name);
}

/**
 * If this pass constructed into a group of passes, removes all sub-passes.
 * Otherwise, an exception is thrown.
 */
void Base::clear_sub_passes() {
    check_group_access_allowed();
    sub_pass_names.clear();
    sub_pass_order.clear();
}

/**
 * If this pass constructed into a conditional pass group, returns a const
 * reference to the configured condition. Otherwise, an exception is thrown.
 */
condition::CRef Base::get_condition() const {
    check_condition_access_allowed();
    return condition.as_const();
}

/**
 * If this pass constructed into a conditional pass group, returns a mutable
 * reference to the configured condition. Otherwise, an exception is thrown.
 */
condition::Ref Base::get_condition() {
    check_condition_access_allowed();
    return condition;
}

/**
 * Handles the debug option. Called once before and once after compile().
 * after_pass is false when run before, and true when run after.
 */
void Base::handle_debugging(
    const ir::Ref &ir,
    const Context &context,
    utils::Bool after_pass
) {
    utils::Str in_or_out = after_pass ? "out" : "in";
    auto debug_opt = options["debug"].as_str();
    if (debug_opt == "yes") {
        ir->dump_seq(
            utils::OutFile(context.output_prefix + "_debug_" + in_or_out + ".ir").unwrap()
        );

        ir::cqasm::WriteOptions write_options;
        write_options.include_statistics = true;
        ir::cqasm::write(
            ir, write_options,
            utils::OutFile(context.output_prefix + "_debug_" + in_or_out + ".cq").unwrap()
        );
    }
    if (debug_opt == "stats" || debug_opt == "both") {
        pass::ana::statistics::report::dump_all(
            ir,
            utils::OutFile(context.output_prefix + "_" + in_or_out + ".report").unwrap()
        );
    }
    if (debug_opt == "qasm" || debug_opt == "both") {
        ir::cqasm::write(
            ir, {},
            utils::OutFile(context.output_prefix + "_" + in_or_out + ".qasm").unwrap()
        );
    }
}

/**
 * Wrapper around running the main pass implementation for this pass, taking
 * care of logging, profiling, etc.
 */
utils::Int Base::run_main_pass(
    const ir::Ref &ir,
    const Context &context
) const {
    QL_IOUT("starting pass \"" << context.full_pass_name << "\" of type \"" << type_name << "\"...");
    auto retval = run_internal(ir, context);
    QL_IOUT("completed pass \"" << context.full_pass_name << "\"; return value is " << retval);
    return retval;
}

/**
 * Wrapper around running the sub-passes for this pass, taking care of logging,
 * profiling, etc.
 */
void Base::run_sub_passes(
    const ir::Ref &ir,
    const Context &context
) const {
    utils::Str sub_prefix = context.full_pass_name.empty() ? "" : (context.full_pass_name + ".");
    for (const auto &pass : sub_pass_order) {
        pass->compile(ir, sub_prefix);
    }
}

/**
 * Executes this pass or pass group on the given platform and program.
 */
void Base::compile(
    const ir::Ref &ir,
    const utils::Str &pass_name_prefix
) {

    // The passes should already have been constructed by the pass manager.
    QL_ASSERT(is_constructed());

    // Construct pass context.
    Context context{
        pass_name_prefix + instance_name,   // -> .full_pass_name
        {},                                 // -> .output_prefix
        options                             // -> .options
    };

    // Apply substitution rules for the output prefix option.
    utils::Bool special = false;
    for (auto c : options["output_prefix"].as_str()) {
        if (special) {
            switch (c) {
                case '%':
                    context.output_prefix += '%';
                    break;
                case 'n':
                    if (!ir->program.empty()) {
                        context.output_prefix += ir->program->name;
                    }
                    break;
                case 'N':
                    if (!ir->program.empty()) {
                        context.output_prefix += ir->program->unique_name;
                    }
                    break;
                case 'p':
                    context.output_prefix += instance_name;
                    break;
                case 'P':
                    context.output_prefix += context.full_pass_name;
                    break;
                case 'U':
                    context.output_prefix += utils::replace_all(context.full_pass_name, ".", "_");
                    break;
                case 'D':
                    context.output_prefix += utils::replace_all(context.full_pass_name, ".", "/");
                    break;
                default:
                    throw utils::Exception(
                        "undefined substitution sequence in output_prefix option "
                        "for pass " + context.full_pass_name + ": %" + c
                    );
            }
            special = false;
        } else if (c == '%') {
            special = true;
        } else {
            context.output_prefix += c;
        }
    }
    if (special) {
        throw utils::Exception(
            "unterminated substitution sequence in output_prefix option "
            "for pass " + context.full_pass_name
        );
    }

    std::string compile_phase = "<not defined>";
    try {
        // Handle configured debugging actions before running the pass.
        compile_phase = "debugging.before";
        handle_debugging(ir, context, false);

        // Traverse our level of the pass tree based on our node type.
        compile_phase = "main";
        switch (node_type) {
            case NodeType::NORMAL: {
                run_main_pass(ir, context);
                break;
            }

            case NodeType::GROUP: {
                run_sub_passes(ir, context);
                break;
            }

            case NodeType::GROUP_IF: {
                auto retval = run_main_pass(ir, context);
                if (condition->evaluate(retval)) {
                    QL_IOUT("pass condition returned true, running sub-passes...");
                    run_sub_passes(ir, context);
                } else {
                    QL_IOUT("pass condition returned false, skipping " << sub_pass_order.size() << " sub-pass(es)");
                }
                break;
            }

            case NodeType::GROUP_WHILE: {
                QL_IOUT("entering loop pass loop...");
                while (true) {
                    auto retval = run_main_pass(ir, context);
                    if (!condition->evaluate(retval)) {
                        QL_IOUT("pass condition returned false, exiting loop");
                        break;
                    } else {
                        QL_IOUT("pass condition returned true, continuing loop...");
                    }
                    run_sub_passes(ir, context);
                }
                break;
            }

            case NodeType::GROUP_REPEAT_UNTIL_NOT: {
                QL_IOUT("entering loop pass loop...");
                while (true) {
                    run_sub_passes(ir, context);
                    auto retval = run_main_pass(ir, context);
                    if (!condition->evaluate(retval)) {
                        QL_IOUT("pass condition returned false, exiting loop");
                        break;
                    } else {
                        QL_IOUT("pass condition returned true, continuing loop...");
                    }
                }
                break;
            }

            default: QL_ASSERT(false);
        }

        // Handle configured debugging actions after running the pass.
        compile_phase = "debugging.after";
        handle_debugging(ir, context, true);

    } catch (utils::Exception &e) {
        if (context.full_pass_name.size() != 0) {   // not at top level
            e.add_context("in pass " + context.full_pass_name + ", phase " + compile_phase);
        }
        throw;
    }
}

} // namespace pass_types
} // namespace pmgr
} // namespace ql
