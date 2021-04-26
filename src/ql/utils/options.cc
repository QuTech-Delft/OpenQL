/** \file
 * Option-parsing and storage implementation.
 */

#include "ql/utils/options.h"

#include "ql/utils/exception.h"
#include "ql/utils/logger.h"
#include "ql/utils/filesystem.h"

namespace ql {
namespace utils {

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
void Option::dump_help(std::ostream &os, const utils::Str &line_prefix) const {
    os << line_prefix << "* `" << name << "` *\n";
    StrStrm ss;
    ss << "Must be " << syntax() << ", ";
    if (configured) {
        ss << "currently `" << current_value << "`";
        if (!default_value.empty()) {
            ss << " (default `" << default_value << "`)";
        }
    } else if (current_value.empty()) {
        ss << "no default value";
    } else {
        ss << "default `" << current_value << "`";
    }
    ss << ".";
    if (!description.empty()) {
        ss << " " << description;
    }
    wrap_str(os, line_prefix + "  ", ss.str());
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
    option.dump_help(os);
    return os;
}

/**
 * Returns a description of the syntax for allowable values.
 */
Str BooleanOption::syntax() const {
    return "`yes` or `no`";
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
    return "one of " + options.to_string("`", "`, `", "`", "`, or `", "` or `");
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
            s << " or one of " + string_options.to_string("`", "`, `", "`", "`, or `", "` or `");
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
            s << " or one of " + string_options.to_string("`", "`, `", "`", "`, or `", "` or `");
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
 * Returns whether an option with the given name exists.
 */
utils::Bool Options::has_option(const utils::Str &key) const {
    auto it = options.find(key);
    return it != options.end();
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
void Options::dump_help(std::ostream &os, const utils::Str &line_prefix) const {
    if (options.empty()) {
        os << line_prefix << "no options exist" << std::endl;
        return;
    }
    for (const auto &it : options) {
        it.second->dump_help(os, line_prefix);
        os << std::endl;
    }
}

/**
 * Dumps all options (or only options which were explicitly set) to the
 * given stream (or stdout).
 */
void Options::dump_options(bool only_set, std::ostream &os, const utils::Str &line_prefix) const {
    bool any = false;
    for (const auto &it : options) {
        if (it.second->is_set() || !only_set) {
            os << line_prefix << it.second->get_name() << ": " << it.second->as_str() << std::endl;
            any = true;
        }
    }
    if (!any) {
        os << line_prefix << "no options to dump" << std::endl;
    }
}

/**
 * Stream write operator for Options.
 */
std::ostream &operator<<(std::ostream &os, const Options &options) {
    options.dump_help(os);
    return os;
}

} // namespace utils
} // namespace ql
