/** \file
 * Option-parsing and storage implementation.
 */

#include "options.h"

#include "utils/exception.h"
#include "utils/logger.h"
#include "utils/filesystem.h"

namespace ql {
namespace options {

using namespace utils;

/**
 * Calls all the callbacks.
 */
void Option::value_changed() {
    for (auto &cb : callbacks) {
        cb(*this);
    }
}

/**
 * Returns a description of the syntax for allowable values.
 */
Str Option::syntax() const {
    return "any string";
}

/**
 * Validates and optionally desugars the given input. Should throw an
 * Exception if the value is invalid.
 */
Str Option::validate(const Str &val) const {
    return val;
}

/**
 * Constructs a new Option.
 */
Option::Option(
    Str &&name,
    Str &&description,
    Str &&default_value
) :
    name(std::move(name)),
    description(std::move(description)),
    default_value(default_value),
    current_value(std::move(default_value)),
    configured(false)
{
}

/**
 * Returns the name of this option.
 */
const Str &Option::get_name() const {
    return name;
}

/**
 * Returns the description of this option.
 */
const Str &Option::get_description() const {
    return description;
}

/**
 * Returns the default value for this option. If the option has no default
 * value, returns an empty string.
 */
const Str &Option::get_default() const {
    return default_value;
}

/**
 * Returns the current value for this option. If the option has no default
 * value and is not configured, returns an empty string.
 */
const Str &Option::as_str() const {
    return current_value;
}

/**
 * Returns the current value for this option as a boolean. This will return true
 * when the value is anything other than the empty string (unconfigured) or
 * "no".
 */
Bool Option::as_bool() const {
    return !(current_value.empty() || current_value == "no");
}

/**
 * Returns the current value for this option as an integer. This will return
 * -1 when the option value is not a valid integer.
 */
Int Option::as_int() const {
    return parse_int(current_value, -1);
}

/**
 * Returns the current value for this option as an unsigned integer. This will
 * return 0 when the option value is not a valid integer.
 */
UInt Option::as_uint() const {
    return parse_uint(current_value, 0);
}

/**
 * Returns the current value for this option as a real number. This will return
 * 0 when the option value is not a valid integer.
 */
Real Option::as_real() const {
    return parse_real(current_value, 0);
}

/**
 * If the given value is nonempty, configures this option with it. An exception
 * is thrown if the value is invalid. If the given value is empty, resets to the
 * default value.
 */
void Option::set(const Str &val) {
    if (val.empty()) {
        reset();
    } else {
        current_value = validate(val);
        configured = true;
        value_changed();
    }
}

/**
 * Same as set().
 */
Option &Option::operator=(const Str &val) {
    set(val);
    return *this;
}

/**
 * Resets this option to the default value.
 */
void Option::reset() {
    current_value = default_value;
    configured = false;
    value_changed();
}

/**
 * Returns whether this option was manually configured.
 */
bool Option::is_set() const {
    return configured;
}

/**
 * Writes a help message for this option to the given stream (or stdout).
 */
void Option::help(std::ostream &os) const {
    os << "Option " << name << ": " << syntax() << ", ";
    if (configured) {
        os << "currently " << current_value;
        if (!default_value.empty()) {
            os << " (default " << default_value << ")";
        }
    } else if (current_value.empty()) {
        os << "not configured";
    } else {
        os << "using default " << current_value;
    }
    if (!description.empty()) {
        os << ": " << description;
    }
}

/**
 * Registers a callback, to be called when the option changes.
 */
Option &Option::with_callback(const std::function<void(Option&)> &callback) {
    callbacks.push_back(callback);
    return *this;
}

/**
 * Stream write operator for Option.
 */
std::ostream &operator<<(std::ostream &os, const Option &option) {
    option.help(os);
    return os;
}

/**
 * Returns a description of the syntax for allowable values.
 */
Str BooleanOption::syntax() const {
    return "yes or no";
}

/**
 * Validates and optionally desugars the given input. Should throw an
 * Exception if the value is invalid.
 */
Str BooleanOption::validate(const Str &val) const {
    auto lower = to_lower(val);
    if (lower == "true" || lower == "yes" || lower == "y" || lower == "1") {
        return "yes";
    } else if (lower == "false" || lower == "no" || lower == "n" || lower == "0") {
        return "no";
    } else {
        throw UserError("invalid value for yes/no option " + get_name() + ": " + val);
    }
}

/**
 * Constructs a new BooleanOption.
 */
BooleanOption::BooleanOption(
    Str &&name,
    Str &&description,
    Bool default_value
) :
    Option(
        std::forward<Str>(name),
        std::forward<Str>(description),
        default_value ? "yes" : "no"
    )
{
}

/**
 * Validates the given option. Just implements validate(), but must be
 * non-virtual to be usable in the constructor.
 */
Str EnumerationOption::validate_(const Str &val) const {
    auto x = to_lower(val);
    for (const auto &option : options) {
        if (to_lower(option) == x) {
            return option;
        }
    }
    StrStrm s{};
    s << "invalid value for option " << get_name() << ":";
    s << " possible values are " << options.to_string("", ", ", "", ", or ", " or ");
    s << ", but " << val << " was given";
    throw UserError(s.str());
}

/**
 * Returns a description of the syntax for allowable values.
 */
Str EnumerationOption::syntax() const {
    return "one of " + options.to_string("", ", ", "", ", or ", " or ");
}

/**
 * Validates and optionally desugars the given input. Should throw an
 * Exception if the value is invalid.
 */
Str EnumerationOption::validate(const Str &val) const {
    return validate_(val);
}

/**
 * Constructs a new EnumerationOption.
 */
EnumerationOption::EnumerationOption(
    Str &&name,
    Str &&description,
    Str &&default_value,
    List<Str> &&options
) :
    Option(
        std::forward<Str>(name),
        std::forward<Str>(description),
        std::forward<Str>(default_value)
    ),
    options(std::move(options))
{
    if (!default_value.empty()) {
        default_value = validate_(default_value);
    }
}

/**
 * Validates the given option. Just implements validate(), but must be
 * non-virtual to be usable in the constructor.
 */
Str IntegerOption::validate_(const Str &val) const {
    bool success;
    auto int_val = parse_int(val, 0, &success);
    if (success && int_val >= minimum && int_val <= maximum) {
        return val;
    }
    auto x = to_lower(val);
    for (const auto &option : string_options) {
        if (to_lower(option) == x) {
            return option;
        }
    }
    StrStrm s{};
    s << "invalid value for option " << get_name() << ":";
    s << " value must be " << syntax();
    s << ", but " << val << " was given";
    throw UserError(s.str());
}

/**
 * Returns a description of the syntax for allowable values.
 */
Str IntegerOption::syntax() const {
    StrStrm s{};
    if (minimum == MIN) {
        if (maximum == MAX) {
            s << "any integer";
        } else {
            s << "an integer greater than or equal to " << maximum;
        }
    } else {
        if (maximum == MAX) {
            s << "an integer less than or equal to " << minimum;
        } else {
            s << "an integer between " << minimum << " and " << maximum << " inclusive";
        }
    }
    if (!string_options.empty()) {
        if (string_options.size() == 1) {
            s << " or " << string_options.front();
        } else {
            s << " or one of " + string_options.to_string("", ", ", "", ", or ", " or ");
        }
    }
    return s.str();
}

/**
 * Validates and optionally desugars the given input. Should throw an
 * Exception if the value is invalid.
 */
Str IntegerOption::validate(const Str &val) const {
    return validate_(val);
}

/**
 * Constructs a new IntegerOption.
 */
IntegerOption::IntegerOption(
    Str &&name,
    Str &&description,
    Str &&default_value,
    Int minimum,
    Int maximum,
    List<Str> &&string_options
) :
    Option(
        std::forward<Str>(name),
        std::forward<Str>(description),
        std::forward<Str>(default_value)
    ),
    minimum(minimum),
    maximum(maximum),
    string_options(std::move(string_options))
{
    if (!default_value.empty()) {
        default_value = validate_(default_value);
    }
}

/**
 * Validates the given option. Just implements validate(), but must be
 * non-virtual to be usable in the constructor.
 */
Str RealOption::validate_(const Str &val) const {
    bool success;
    auto real_val = parse_real(val, 0, &success);
    if (success && real_val >= minimum && real_val <= maximum) {
        return val;
    }
    auto x = to_lower(val);
    for (const auto &option : string_options) {
        if (to_lower(option) == x) {
            return option;
        }
    }
    StrStrm s{};
    s << "invalid value for option " << get_name() << ":";
    s << " value must be " << syntax();
    s << ", but " << val << " was given";
    throw UserError(s.str());
}

/**
 * Returns a description of the syntax for allowable values.
 */
Str RealOption::syntax() const {
    StrStrm s{};
    if (minimum == -INF) {
        if (maximum == INF) {
            s << "any real number";
        } else {
            s << "an real number greater than or equal to " << maximum;
        }
    } else {
        if (maximum == INF) {
            s << "an real number less than or equal to " << minimum;
        } else {
            s << "an real number between " << minimum << " and " << maximum << " inclusive";
        }
    }
    if (!string_options.empty()) {
        if (string_options.size() == 1) {
            s << " or " << string_options.front();
        } else {
            s << " or one of " + string_options.to_string("", ", ", "", ", or ", " or ");
        }
    }
    return s.str();
}

/**
 * Validates and optionally desugars the given input. Should throw an
 * Exception if the value is invalid.
 */
Str RealOption::validate(const Str &val) const {
    return validate_(val);
}

/**
 * Constructs a new IntegerOption.
 */
RealOption::RealOption(
    Str &&name,
    Str &&description,
    Str &&default_value,
    Real minimum,
    Real maximum,
    List<Str> &&string_options
) :
    Option(
        std::forward<Str>(name),
        std::forward<Str>(description),
        std::forward<Str>(default_value)
    ),
    minimum(minimum),
    maximum(maximum),
    string_options(std::move(string_options))
{
    if (!default_value.empty()) {
        default_value = validate_(default_value);
    }
}

/**
 * Adds a string option.
 */
Option &Options::add_str(
    Str &&name,
    Str &&description,
    Str &&default_value
) {
    return add<Option>(
        std::forward<Str>(name),
        std::forward<Str>(description),
        std::forward<Str>(default_value)
    );
}

/**
 * Adds a boolean (yes/no) option.
 */
Option &Options::add_bool(
    Str &&name,
    Str &&description,
    Bool default_value
) {
    return add<BooleanOption>(
        std::forward<Str>(name),
        std::forward<Str>(description),
        default_value
    );
}

/**
 * Adds an enumeration option.
 */
Option &Options::add_enum(
    Str &&name,
    Str &&description,
    Str &&default_value,
    List<Str> &&options
) {
    return add<EnumerationOption>(
        std::forward<Str>(name),
        std::forward<Str>(description),
        std::forward<Str>(default_value),
        std::forward<List<Str>>(options)
    );
}

/**
 * Adds an integer option.
 */
Option &Options::add_int(
    Str &&name,
    Str &&description,
    Str &&default_value,
    Int minimum,
    Int maximum,
    List<Str> &&string_options
) {
    return add<IntegerOption>(
        std::forward<Str>(name),
        std::forward<Str>(description),
        std::forward<Str>(default_value),
        minimum,
        maximum,
        std::forward<List<Str>>(string_options)
    );
}

/**
 * Adds a real number option.
 */
Option &Options::add_real(
    Str &&name,
    Str &&description,
    Str &&default_value,
    Real minimum,
    Real maximum,
    List<Str> &&string_options
) {
    return add<RealOption>(
        std::forward<Str>(name),
        std::forward<Str>(description),
        std::forward<Str>(default_value),
        minimum,
        maximum,
        std::forward<List<Str>>(string_options)
    );
}

/**
 * Returns mutable access to a configuration option.
 */
Option &Options::operator[](const Str &key) {
    auto it = options.find(key);
    if (it == options.end()) {
        throw UserError("unknown option: " + key);
    }
    return *it->second;
}

/**
 * Returns immutable access to a configuration option.
 */
const Option &Options::operator[](const Str &key) const {
    auto it = options.find(key);
    if (it == options.end()) {
        throw UserError("unknown option: " + key);
    }
    return *it->second;
}

/**
 * Updates our options with the values from the src object. The supported
 * options should be compatible.
 */
void Options::update_from(const Options &src) {
    for (const auto &it : src.options) {
        if (it.second->is_set()) {
            operator[](it.first).set(it.second->as_str());
        }
    }
}

/**
 * Resets all options to their default values.
 */
void Options::reset() {
    for (auto &it : options) {
        it.second->reset();
    }
}

/**
 * Writes a help message for this option to the given stream (or stdout).
 */
void Options::help(std::ostream &os) const {
    if (options.empty()) {
        os << "no options have been added!" << std::endl;
        return;
    }
    for (const auto &it : options) {
        os << it.second << std::endl;
    }
}

/**
 * Dumps all options (or only options which were explicitly set) to the
 * given stream (or stdout).
 */
void Options::dump(bool only_set, std::ostream &os) const {
    bool any = false;
    for (const auto &it : options) {
        if (it.second->is_set() || !only_set) {
            os << it.second->get_name() << ": " << it.second->as_str() << std::endl;
            any = true;
        }
    }
    if (!any) {
        os << "no options to dump" << std::endl;
    }
}

/**
 * Stream write operator for Options.
 */
std::ostream &operator<<(std::ostream &os, const Options &options) {
    options.help(os);
    return os;
}

/**
 * Makes a new options record for OpenQL.
 */
Options make_ql_options() {
    auto options = Options();

    options.add_enum("log_level", "Log levels", "LOG_NOTHING", {"LOG_NOTHING", "LOG_CRITICAL", "LOG_ERROR", "LOG_WARNING", "LOG_INFO", "LOG_DEBUG"}).with_callback([](Option &x){logger::set_log_level(x.as_str());});
    options.add_str ("output_dir", "Name of output directory", "test_output").with_callback([](Option &x){make_dirs(x.as_str());});;
    options.add_bool("unique_output", "Make output files unique");
    options.add_bool("prescheduler", "Run qasm (first) scheduler?", true);
    options.add_bool("scheduler_post179", "Issue 179 solution included", true);
    options.add_bool("print_dot_graphs", "Print (un-)scheduled graphs in DOT format");
    options.add_enum("scheduler", "scheduler type", "ALAP", {"ASAP", "ALAP"});
    options.add_bool("scheduler_uniform", "Do uniform scheduling or not");
    options.add_bool("scheduler_commute", "Commute two-qubit gates when possible, or not");
    options.add_bool("scheduler_commute_rotations", "Commute rotation gates and with two-qubit gates when possible, or not");
    options.add_bool("use_default_gates", "Use default gates or not", "yes");
    options.add_bool("optimize", "optimize or not");
    options.add_bool("clifford_prescheduler", "clifford optimize before prescheduler yes or not");
    options.add_bool("clifford_postscheduler", "clifford optimize after prescheduler yes or not");
    options.add_bool("clifford_premapper", "clifford optimize before mapping yes or not");
    options.add_bool("clifford_postmapper", "clifford optimize after mapping yes or not");
    options.add_enum("decompose_toffoli", "Type of decomposition used for toffoli", "no", {"no", "NC", "AM"});
    options.add_enum("quantumsim", "Produce quantumsim output, and of which kind", "no", {"no", "yes", "qsoverlay"});
    options.add_bool("issue_skip_319", "Issue skip instead of wait in bundles");
    options.add_str ("backend_cc_map_input_file", "Name of CC input map file");
    options.add_enum("cz_mode", "CZ mode", "manual", {"manual", "auto"});
    options.add_enum("mapper", "Mapper heuristic", "no", {"no", "base", "baserc", "minextend", "minextendrc", "maxfidelity"});
    options.add_bool("mapinitone2one", "Initialize mapping of virtual qubits one to one to real qubits", true);
    options.add_bool("mapprepinitsstate", "Prep gate leaves qubit in zero state");
    options.add_bool("mapassumezeroinitstate", "Assume that qubits are initialized to zero state");
    options.add_enum("initialplace", "Initialplace qubits before mapping", "no", {"no", "yes", "1s", "10s", "1m", "10m", "1h", "1sx", "10sx", "1mx", "10mx", "1hx"});
    options.add_int ("initialplace2qhorizon", "Initialplace considers only this number of initial two-qubit gates", "0", 0, 100);
    options.add_enum("maplookahead", "Strategy wrt selecting next gate(s) to map", "noroutingfirst", {"no", "1qfirst", "noroutingfirst", "all"});
    options.add_enum("mappathselect", "Which paths: all or borders", "all", {"all", "borders"});
    options.add_enum("mapselectswaps", "Select only one swap, or earliest, or all swaps for one alternative", "all", {"one", "all", "earliest"});
    options.add_bool("maprecNN2q", "Recursing also on NN 2q gate?");
    options.add_int ("mapselectmaxlevel", "Maximum recursion in selecting alternatives on minimum extension", "0", 0, 10, {"inf"});
    options.add_enum("mapselectmaxwidth", "Maximum width number of alternatives to enter recursion with", "min", {"min", "minplusone", "minplushalfmin", "minplusmin", "all"});
    options.add_enum("maptiebreak", "Tie break method", "random", {"first", "last", "random", "critical"});
    options.add_int ("mapusemoves", "Use unused qubit to move thru", "yes", 0, 20, {"no", "yes"});
    options.add_bool("mapreverseswap", "Reverse swap operands when better", true);
    options.add_bool("write_qasm_files", "write (un-)scheduled (with and without resource-constraint) qasm files");
    options.add_bool("write_report_files", "write report files on circuit characteristics and pass results");

    return options;
}

/**
 * Global options object for all of OpenQL.
 */
Options global = make_ql_options();

/**
 * Convenience function for getting an option value as a string from the global
 * options record.
 */
const Str &get(const Str &key) {
    return global[key].as_str();
}

/**
 * Convenience function for setting an option value for the global options
 * record.
 */
void set(const Str &key, const Str &value) {
    global[key] = value;
}

} // namespace options
} // namespace ql
