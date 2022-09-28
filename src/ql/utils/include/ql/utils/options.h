/** \file
 * Option-parsing and storage implementation.
 */

#pragma once

#include <iostream>
#include <functional>
#include "ql/utils/ptr.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/utils/map.h"

namespace ql {
namespace utils {

/**
 * Represents an option. That is, some user-configurable value with some
 * (polymorphic) possible syntax. Note that the default value does not
 * necessarily have to confirm to the possible syntax.
 */
class Option {
private:

    /**
     * Name of the option.
     */
    const utils::Str name;

    /**
     * Description for additional information.
     */
    const utils::Str description;

    /**
     * The default value.
     */
    const utils::Str default_value;

    /**
     * The currently configured value.
     */
    utils::Str current_value;

    /**
     * Whether this option was manually configured. Note that it is possible for
     * default_value and current_value to be equal even if this is true.
     */
    utils::Bool configured;

    /**
     * List of callback functions, to be called when this option is set.
     */
    utils::List<std::function<void(Option&)>> callbacks;

    /**
     * Calls all the callbacks.
     */
    void value_changed();

protected:

    /**
     * Returns a description of the syntax for allowable values.
     */
    virtual utils::Str syntax() const;

    /**
     * Validates and optionally desugars the given input. Should throw an
     * Exception if the value is invalid.
     */
    virtual utils::Str validate(const utils::Str &val) const;

public:

    /**
     * Constructs a new Option.
     */
    explicit Option(
        utils::Str &&name,
        utils::Str &&description="",
        utils::Str &&default_value=""
    );

    /**
     * Destroys an Option.
     */
    virtual ~Option() = default;

    /**
     * Returns the name of this option.
     */
    const utils::Str &get_name() const;

    /**
     * Returns the description of this option.
     */
    const utils::Str &get_description() const;

    /**
     * Returns the default value for this option. If the option has no default
     * value, returns an empty string.
     */
    const utils::Str &get_default() const;

    /**
     * Returns the current value for this option. If the option has no default
     * value and is not configured, returns an empty string.
     */
    const utils::Str &as_str() const;

    /**
     * Returns the current value for this option as a boolean. This will return
     * true when the value is anything other than the empty string
     * (unconfigured) or "no".
     */
    utils::Bool as_bool() const;

    /**
     * Returns the current value for this option as an integer. This will return
     * -1 when the option value is not a valid integer.
     */
    utils::Int as_int() const;

    /**
     * Returns the current value for this option as an unsigned integer. This will
     * return 0 when the option value is not a valid integer.
     */
    utils::UInt as_uint() const;

    /**
     * Returns the current value for this option as a real number. This will return
     * 0 when the option value is not a valid integer.
     */
    utils::Real as_real() const;

    /**
     * If the given value is nonempty, configures this option with it. An
     * exception is thrown if the value is invalid. If the given value is empty,
     * resets to the default value.
     */
    void set(const utils::Str &val);

    /**
     * Same as set().
     */
    Option &operator=(const utils::Str &val);

    /**
     * Resets this option to the default value.
     */
    void reset();

    /**
     * Returns whether this option was manually configured.
     */
    bool is_set() const;

    /**
     * Writes a help message for this option to the given stream (or stdout).
     */
    void dump_help(std::ostream &os = std::cout, const utils::Str &line_prefix = "") const;

    /**
     * Registers a callback, to be called when the option changes.
     */
    Option &with_callback(const std::function<void(Option&)> &callback);

};

/**
 * Stream write operator for Option.
 */
std::ostream &operator<<(std::ostream &os, const Option &option);

/**
 * Represents a boolean option.
 */
class BooleanOption : public Option {
protected:

    /**
     * Returns a description of the syntax for allowable values.
     */
    utils::Str syntax() const override;

    /**
     * Validates and optionally desugars the given input. Should throw an
     * Exception if the value is invalid.
     */
    utils::Str validate(const utils::Str &val) const override;

public:

    /**
     * Constructs a new BooleanOption.
     */
    explicit BooleanOption(
        utils::Str &&name,
        utils::Str &&description="",
        utils::Bool default_value=false
    );

};

/**
 * Represents an option that can only be set to one of a given set of strings
 * (or unset, if no default is provided).
 */
class EnumerationOption : public Option {
private:

    /**
     * The allowable values for this option.
     */
    const utils::List<utils::Str> options;

    /**
     * Validates the given option. Just implements validate(), but must be
     * non-virtual to be usable in the constructor.
     */
    utils::Str validate_(const utils::Str &val) const;

protected:

    /**
     * Returns a description of the syntax for allowable values.
     */
    utils::Str syntax() const override;

    /**
     * Validates and optionally desugars the given input. Should throw an
     * Exception if the value is invalid.
     */
    utils::Str validate(const utils::Str &val) const override;

public:

    /**
     * Constructs a new EnumerationOption.
     */
    explicit EnumerationOption(
        utils::Str &&name,
        utils::Str &&description,
        utils::Str &&default_value,
        utils::List<utils::Str> &&options
    );

};

/**
 * Represents an option that can only be within some integer range, or
 * optionally within a set of allowable strings.
 */
class IntegerOption : public Option {
private:

    /**
     * The minimum integer value.
     */
    const utils::Int minimum;

    /**
     * The maximum integer value.
     */
    const utils::Int maximum;

    /**
     * The allowable non-integer values for this option, if any.
     */
    const utils::List<utils::Str> string_options;

    /**
     * Validates the given option. Just implements validate(), but must be
     * non-virtual to be usable in the constructor.
     */
    utils::Str validate_(const utils::Str &val) const;

protected:

    /**
     * Returns a description of the syntax for allowable values.
     */
    utils::Str syntax() const override;

    /**
     * Validates and optionally desugars the given input. Should throw an
     * Exception if the value is invalid.
     */
    utils::Str validate(const utils::Str &val) const override;

public:

    /**
     * Constructs a new IntegerOption.
     */
    explicit IntegerOption(
        utils::Str &&name,
        utils::Str &&description="",
        utils::Str &&default_value="",
        utils::Int minimum = utils::MIN,
        utils::Int maximum = utils::MAX,
        utils::List<utils::Str> &&string_options={}
    );

};

/**
 * Represents an option that can only be within some real number range, or
 * optionally within a set of allowable strings.
 */
class RealOption : public Option {
private:

    /**
     * The minimum value.
     */
    const utils::Real minimum;

    /**
     * The maximum value.
     */
    const utils::Real maximum;

    /**
     * The allowable non-integer values for this option, if any.
     */
    const utils::List<utils::Str> string_options;

    /**
     * Validates the given option. Just implements validate(), but must be
     * non-virtual to be usable in the constructor.
     */
    utils::Str validate_(const utils::Str &val) const;

protected:

    /**
     * Returns a description of the syntax for allowable values.
     */
    utils::Str syntax() const override;

    /**
     * Validates and optionally desugars the given input. Should throw an
     * Exception if the value is invalid.
     */
    utils::Str validate(const utils::Str &val) const override;

public:

    /**
     * Constructs a new RealOption.
     */
    explicit RealOption(
        utils::Str &&name,
        utils::Str &&description = "",
        utils::Str &&default_value = "",
        utils::Real minimum = -utils::INF,
        utils::Real maximum = utils::INF,
        utils::List<utils::Str> &&string_options = {}
    );

};

/**
 * A set of user-configurable options.
 */
class Options {
private:

    /**
     * Map of all available options.
     */
    utils::Map<utils::Str, utils::Ptr<Option>> options;

    /**
     * Order in which the options were added. Used for documentation output.
     */
    utils::List<utils::Str> option_order;

public:

    /**
     * Adds a configuration option.
     */
    template<typename T, typename ...Args, typename = typename std::enable_if<std::is_base_of<Option, T>::value>::type>
    Option &add(Args&&... args) {
        auto option = utils::Ptr<Option>();
        option.emplace<T>(std::forward<Args>(args)...);
        options.set(option->get_name()) = option;
        option_order.push_back(option->get_name());
        return *option;
    }

    /**
     * Adds a string option.
     */
    Option &add_str(
        utils::Str &&name,
        utils::Str &&description="",
        utils::Str &&default_value=""
    );

    /**
     * Adds a boolean (yes/no) option.
     */
    Option &add_bool(
        utils::Str &&name,
        utils::Str &&description="",
        utils::Bool default_value=false
    );

    /**
     * Adds an enumeration option.
     */
    Option &add_enum(
        utils::Str &&name,
        utils::Str &&description,
        utils::Str &&default_value,
        utils::List<utils::Str> &&options
    );

    /**
     * Adds an integer option.
     */
    Option &add_int(
        utils::Str &&name,
        utils::Str &&description="",
        utils::Str &&default_value="",
        utils::Int minimum = utils::MIN,
        utils::Int maximum = utils::MAX,
        utils::List<utils::Str> &&string_options={}
    );

    /**
     * Adds a real number option.
     */
    Option &add_real(
        utils::Str &&name,
        utils::Str &&description="",
        utils::Str &&default_value="",
        utils::Real minimum = -utils::INF,
        utils::Real maximum = utils::INF,
        utils::List<utils::Str> &&string_options={}
    );

    /**
     * Returns mutable access to a configuration option.
     */
    Option &operator[](const utils::Str &key);

    /**
     * Returns immutable access to a configuration option.
     */
    const Option &operator[](const utils::Str &key) const;

    /**
     * Returns whether an option with the given name exists.
     */
    utils::Bool has_option(const utils::Str &key) const;

    /**
     * Updates our options with the values from the src object. The supported
     * options should be compatible.
     */
    void update_from(const Options &src);

    /**
     * Resets all options to their default values.
     */
    void reset();

    /**
     * Writes a help message for this option to the given stream (or stdout).
     */
    void dump_help(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Dumps all options (or only options which were explicitly set) to the
     * given stream (or stdout).
     */
    void dump_options(
        bool only_set = false,
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

};

/**
 * Stream write operator for Options.
 */
std::ostream &operator<<(std::ostream &os, const Options &options);

} // namespace utils
} // namespace ql
