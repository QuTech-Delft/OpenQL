/** \file
 * Implementation for converting cQASM files to OpenQL's IR.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/tree.h"
#include "ql/ir/compat/compat.h"
#include "cqasm.hpp"

namespace ql::ir::compat::cqasm_reader::detail {

// Shorthands for namespaces.
namespace lqt = cqasm::tree;

namespace lqa = cqasm::v1x::analyzer;
namespace lqi = cqasm::v1x::instruction;
namespace lqp = cqasm::v1x::parser;
namespace lqs = cqasm::v1x::semantic;
namespace lqtyp = cqasm::v1x::types;
namespace lqv = cqasm::v1x::values;

using namespace utils;

/**
 * Annotation type for Variable nodes, used to store the mapping from cQASM
 * named variable to qubit/creg/breg indices in OpenQL.
 */
struct VarIndex {
    UInt index;
};

/**
 * Angle conversion method.
 */
enum class AngleConversionMethod {

    /**
     * cQASM value is in radians.
     */
    RADIANS,

    /**
     * cQASM value is in degrees.
     */
    DEGREES,

    /**
     * cQASM value is converted to radians using 2pi/2^k.
     */
    POWER_OF_TWO

};

/**
 * Interface class for parsing an OpenQL parameter (qubit, creg, breg, duration,
 * or angle) from the cQASM argument list.
 */
template <typename T>
class Value : public Node {
public:

    /**
     * Parses the value from the given gate operand list, for the given
     * single-gate-multiple-qubit index (in case there are multiple parallel
     * gates).
     */
    virtual T get(const lqt::Any<lqv::Node> &operands, UInt sgmq_index) const = 0;
};

/**
 * Implementation of Value that always returns a constant value, regardless of
 * gate operands.
 */
template <typename T>
class FixedValue : public Value<T> {
private:
    T value;
public:

    /**
     * Creates a trivial parser that always returns the given value.
     */
    explicit FixedValue(T value) : value(value) {}

    T get(const lqt::Any<lqv::Node> &operands, UInt sgmq_index) const override {
        (void)operands;
        (void)sgmq_index;
        return value;
    }
};

/**
 * Parses an integer-based OpenQL parameter (qubit, creg, breg, duration) from
 * a cQASM parameter of type integer, variable (index), qubit (index), or breg
 * (index). Type-checking is done by libqasm already, so no additional checks
 * are done here.
 */
class UIntFromParameter : public Value<UInt> {
private:
    UInt index;
public:

    /**
     * Creates a parser that parses the given operand index as an unsigned
     * integer.
     */
    explicit UIntFromParameter(UInt index);

    UInt get(const lqt::Any<lqv::Node> &operands, UInt sgmq_index) const override;
};

/**
 * Parses the OpenQL angle parameter from a cQASM parameter of type integer or
 * real using a given conversion method.
 */
class AngleFromParameter : public Value<Real> {
private:
    UInt index;
    AngleConversionMethod method;
public:

    /**
     * Creates a parser that parses the given operand index as an angle using
     * the given conversion method.
     */
    AngleFromParameter(UInt index, AngleConversionMethod method);

    Real get(const lqt::Any<lqv::Node> &operands, UInt sgmq_index) const override;
};

/**
 * Represents how a particular cQASM gate should be converted to its OpenQL
 * representation.
 */
class GateConversionRule {
public:

    /**
     * Smart pointer used to refer to a gate converter.
     */
    using Ptr = std::shared_ptr<GateConversionRule>;

    /**
     * cQASM instruction configuration.
     */
    lqi::Instruction cq_insn;

    /**
     * The name of the gate in OpenQL.
     */
    Str ql_name;

    /**
     * cQASM to OpenQL qubit argument converters.
     */
    Any<Value<UInt>> ql_qubits;

    /**
     * Flag specifying that all qubits in the platform should be appended to
     * the OpenQL qubit argument list.
     */
    Bool ql_all_qubits;

    /**
     * cQASM to OpenQL control register argument converters.
     */
    Any<Value<UInt>> ql_cregs;

    /**
     * Flag specifying that all cregs used in the program should be appended to
     * the OpenQL creg argument list.
     */
    Bool ql_all_cregs;

    /**
     * cQASM to OpenQL bit register argument converters.
     */
    Any<Value<UInt>> ql_bregs;

    /**
     * Flag specifying that all bregs used in the program should be appended to
     * the OpenQL breg argument list.
     */
    Bool ql_all_bregs;

    /**
     * cQASM to OpenQL duration parameter converter.
     */
    One<Value<UInt>> ql_duration;

    /**
     * cQASM to OpenQL angle parameter converter.
     */
    One<Value<Real>> ql_angle;

    /**
     * Flag specifying that a gate with multiple qubit arguments should be
     * treated as multiple single-qubit gates instead. This applies after
     * ql_all_qubits is processed, so you can make gates that implicitly apply
     * a single-qubit gate on all qubits.
     */
    Bool implicit_sgmq;

    /**
     * Flag specifying that implicit breg operands should be added for each
     * qubit operand in the final OpenQL gate. This is applied after
     * ql_implicit_sgmq.
     */
    Bool implicit_breg;

private:

    /**
     * Constructs a basic gate converter:
     *  - the name of the gate is as specified both in cQASM and in OpenQL;
     *  - params specifies the parameter set as a string in cQASM order, where
     *    `Q` is used for a qubit, `I` for a creg, `B` for a breg, `i` for the
     *    duration, and `r` for an optional angle in radians;
     *  - additional cQASM type specifiers may be specified in params, but they
     *    will be ignored in the conversion;
     *  - qubits, cregs, and bregs are ordered in the same way in OpenQL;
     *  - duration parameter of the OpenQL gate is set to 0;
     *  - gates can be made conditional;
     *  - gates can be parallel using single-gate-multiple-qubit notation (they
     *    will simply be expanded to multiple gates in the OpenQL syntax);
     *  - qubits may not be reused.
     * Above defaults can be modified after construction.
     */
    GateConversionRule(
        const Str &name,
        const Str &params
    );

    /**
     * Parses a string of the form "%i" where i is an index into the cQASM
     * parameter list specified by params. Returns MAX if the string is not of
     * the right form. Throws an exception if the ith parameter is out of range
     * or has a type code that's not in the allowed_types set.
     */
    static UInt parse_ref(const Str &ref, const Str &params, const Str &allowed_types);

    /**
     * Parses the a custom qubit/creg/breg argument list from JSON. json must
     * be an array or the string "all". The array entries must be integers to
     * specify fixed qubit/creg/breg indices, or strings of the form "%<idx>",
     * where idx refers to a parameter with cQASM typespec Q, B, or I (resp.
     * qubit reference, bit reference, or integer variable reference). params
     * specifies the cQASM parameter typespec for the associated gate to check
     * validity of aforementioned. The result is written to args/all_args.
     */
    static void refs_from_json(
        const Json &json,
        const Str &params,
        Any<Value<UInt>> &args,
        Bool &all_args
    );

public:

    /**
     * Constructs a basic gate converter:
     *  - the name of the gate is as specified both in cQASM and in OpenQL;
     *  - params specifies the parameter set as a string in cQASM order, where
     *    `Q` is used for a qubit, `I` for a creg, `B` for a breg, and `r` for
     *    an optional angle in radians;
     *  - additional cQASM type specifiers may be specified in params, but they
     *    will be ignored in the conversion;
     *  - qubits, cregs, and bregs are ordered in the same way in OpenQL;
     *  - duration parameter of the OpenQL gate is set to 0;
     *  - gates can be made conditional;
     *  - gates can be parallel using single-gate-multiple-qubit notation (they
     *    will simply be expanded to multiple gates in the OpenQL syntax);
     *  - qubits may not be reused.
     * Above defaults can be modified after construction.
     */
    static Ptr from_defaults(
        const Str &name,
        const Str &params,
        const Str &ql_name = ""
    );

    /**
     * Constructs a gate convertor from a JSON description. The JSON value must
     * be a map, supporting the following keys:
     *
     *     {
     *         "name": "<name>",               # mandatory
     *         "params": "<typespec>",         # mandatory, refer to cqasm::types::from_spec()
     *         "allow_conditional": <bool>,    # whether conditional gates of this type are accepted, defaults to true
     *         "allow_parallel": <bool>,       # whether parallel gates of this type are accepted, defaults to true
     *         "allow_reused_qubits": <bool>,  # whether reused qubit args for this type are accepted, defaults to false
     *         "ql_name": "<name>",            # defaults to "name"
     *         "ql_qubits": [                  # list or "all", defaults to the "Q" args
     *             0,                          # hardcoded qubit index
     *             "%0"                        # reference to argument 0, which can be a qubitref, bitref, or int
     *         ],
     *         "ql_cregs": [                   # list or "all", defaults to the "I" args
     *             0,                          # hardcoded creg index
     *             "%0"                        # reference to argument 0, which can be an int variable reference, or int for creg index
     *         ],
     *         "ql_bregs": [                   # list or "all", defaults to the "B" args
     *             0,                          # hardcoded breg index
     *             "%0"                        # reference to argument 0, which can be an int variable reference, or int for creg index
     *         ],
     *         "ql_duration": 0,               # duration; int to hardcode or "%i" to take from param i (must be of type int), defaults to 0
     *         "ql_angle": 0.0,                # angle; float to hardcode or "%i" to take from param i (must be of type int or real), defaults to first arg of type real or 0.0
     *         "ql_angle_type": "<type>",      # interpretation of angle arg; one of "rad" (radians), "deg" (degrees), or "pow2" (2pi/2^k radians), defaults to "rad"
     *         "implicit_sgmq": <bool>,        # if multiple qubit args are present, a single-qubit gate of this type should be replicated for these qubits (instead of a single gate with many qubits)
     *         "implicit_breg": <bool>         # the breg operand(s) that implicitly belongs to the qubit operand(s) in the gate should be added to the OpenQL operand list
     *     }
     */
    static Ptr from_json(const Json &json);

};

/**
 * Private implementation for the opaque public Reader class; as in it's not in
 * public headers, reducing compile time.
 */
class ReaderImpl {
private:

    /**
     * OpenQL platform reference to compile for.
     */
    const ir::compat::PlatformRef &platform;

    /**
     * OpenQL program to add loaded circuits to.
     */
    const ir::compat::ProgramRef &program;

    /**
     * Represents the supported set of gates. This differs from the platform
     * JSON file gateset; this vector describes the gates as they should be
     * interpreted by libqasm and how they should be converted to the gates in
     * the platform configuration file, rather than those gates themselves.
     */
    Vec<typename GateConversionRule::Ptr> gateset;

    /**
     * Number of subcircuits added using this reader.
     */
    UInt subcircuit_count;

    /**
     * Builds a libqasm Analyzer for the configured gateset. If no gateset is
     * configured (i.e. gateset is empty), then backward-compatible defaults are
     * inserted.
     */
    lqa::Analyzer build_analyzer();

    /**
     * Handles the parse result of string2circuit() and file2circuit().
     */
    void handle_parse_result(lqa::AnalysisResult &&ar);

public:

    /**
     * Constructs a reader.
     */
    ReaderImpl(const ir::compat::PlatformRef &platform, const ir::compat::ProgramRef &program);

    /**
     * Load libqasm gateset and conversion rules to OpenQL gates from a JSON
     * object. Any existing gateset conversion rules are first deleted.
     *
     * The toplevel JSON object should be an array of objects, where each object
     * represents a libqasm gate (overload) and its conversion to OpenQL. The
     * expected structure of these objects is described in
     * GateConverter::from_json().
     */
    void load_gateset(const Json &json);

    /**
     * Parses a cQASM string using the gateset selected when the Reader is
     * constructed, converts the cQASM kernels to OpenQL kernels, and adds those
     * kernels to the selected OpenQL program.
     */
    void string2circuit(const utils::Str &cqasm_str);

    /**
     * Parses a cQASM file using the gateset selected when the Reader is
     * constructed, converts the cQASM kernels to OpenQL kernels, and adds those
     * kernels to the selected OpenQL program.
     */
    void file2circuit(const utils::Str &cqasm_fname);

};

} // namespace ql::ir::compat::cqasm_reader::detail
