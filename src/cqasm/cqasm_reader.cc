/** \file
 * Implementation for converting cQASM files to OpenQL's IR.
 */

#include "cqasm_reader.h"

#include "utils/tree.h"
#include "platform.h"
#include "kernel.h"
#include "program.h"
#include "cqasm.hpp"

namespace ql {
namespace cqasm {

// Shorthands for namespaces.
namespace lqt { using namespace ::cqasm::tree; }
namespace lqs { using namespace ::cqasm::semantic; }
namespace lqv { using namespace ::cqasm::values; }
namespace lqtyp { using namespace ::cqasm::types; }
namespace lqi { using namespace ::cqasm::instruction; }
namespace lqa { using namespace ::cqasm::analyzer; }
namespace lqp { using namespace ::cqasm::parser; }

using namespace utils;

/**
 * Extracts the location annotation from a node and returns it as a string.
 */
static Str location(const lqt::Annotatable &node) {
    auto loc = node.get_annotation_ptr<lqp::SourceLocation>();
    if (loc) {
        return to_string(*loc);
    } else {
        return "<unknown>";
    }
}

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
 * Converts the given angle using the given conversion method.
 */
static Real convert_angle(Real angle, AngleConversionMethod method) {
    switch (method) {
        case AngleConversionMethod::RADIANS:
            return angle;
        case AngleConversionMethod::DEGREES:
            return angle / 180.0 * PI;
        default: // AngleConversionMethod::POWER_OF_TWO
            return 2.0 * PI / pow(2, angle);
    }
}

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
    explicit UIntFromParameter(UInt index) : index(index) {}

    UInt get(const lqt::Any<lqv::Node> &operands, UInt sgmq_index) const override {
        if (auto c = operands[index]->as_const_int()) {

            // Constant integer parameter; just return the integer as specified.
            return itou(c->value);

        } else if (auto v = operands[index]->as_variable_ref()) {

            // Variable reference. Variables are mapped to qubits, cregs, or
            // bregs depending on their type (qubit, int, bit). This mapping is
            // constructed prior to gates being converted, and the chosen
            // indices are stored as an annotation on the variable.
            return v->variable->get_annotation<VarIndex>().index;

        } else if (auto q = operands[index]->as_qubit_refs()) {

            // Legacy qubit reference, using integer indices in cQASM already.
            return itou(q->index[sgmq_index]->value);

        } else if (auto b = operands[index]->as_bit_refs()) {

            // Legacy bit reference, using integer indices in cQASM already.
            // These always map to the first N bregs, where N is the number of
            // qubits in the platform.
            return itou(b->index[sgmq_index]->value);

        } else {
            throw Exception("unexpected operand type at " + location(*operands[index]));
        }
    }
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
    AngleFromParameter(UInt index, AngleConversionMethod method) : index(index), method(method) {}

    Real get(const lqt::Any<lqv::Node> &operands, UInt sgmq_index) const override {
        (void)sgmq_index;
        Real val;
        if (auto i = operands[index]->as_const_int()) {
            val = i->value;
        } else if (auto r = operands[index]->as_const_int()) {
            val = r->value;
        } else {
            throw Exception("expected a real number at " + location(*operands[index]));
        }
        return convert_angle(val, method);
    }
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
    ) :
        cq_insn(name, params),
        ql_name(name),
        ql_all_qubits(false),
        ql_all_cregs(false),
        ql_all_bregs(false),
        implicit_sgmq(false),
        implicit_breg(false)
    {
        // Automatically map the cQASM parameter types to OpenQL parameters.
        for (UInt idx = 0; idx < params.size(); idx++) {
            char c = params.at(idx);
            switch (c) {
                case 'Q':
                    ql_qubits.add(make<UIntFromParameter>(idx));
                    break;
                case 'I':
                    ql_cregs.add(make<UIntFromParameter>(idx));
                    break;
                case 'B':
                    ql_bregs.add(make<UIntFromParameter>(idx));
                    break;
                case 'i':
                    if (ql_duration.empty()) {
                        ql_duration = make<UIntFromParameter>(idx);
                    }
                    break;
                case 'r':
                    if (ql_angle.empty()) {
                        ql_angle = make<AngleFromParameter>(idx, AngleConversionMethod::RADIANS);
                    }
                    break;
                default:
                    break;
            }
        }

        // Default duration and angle to 0.
        ql_duration = make<FixedValue<UInt>>(0);
        if (ql_angle.empty()) {
            ql_angle = make<FixedValue<Real>>(0.0);
        }
    }

    /**
     * Parses a string of the form "%i" where i is an index into the cQASM
     * parameter list specified by params. Returns MAX if the string is not of
     * the right form. Throws an exception if the ith parameter is out of range
     * or has a type code that's not in the allowed_types set.
     */
    static UInt parse_ref(const Str &ref, const Str &params, const Str &allowed_types) {
        if (ref.size() >= 2 && ref.at(0) == '%') {
            auto param_idx = parse_uint(ref.substr(1));
            if (param_idx >= params.size()) {
                throw Exception("parameter index out of range");
            }
            if (allowed_types.find_first_of(params.at(param_idx)) == Str::npos) {
                throw Exception("parameter " + to_string(param_idx) + " has unexpected type");
            }
            return param_idx;
        } else {
            return MAX;
        }
    }

    /**
     * Parses the a custom qubit/creg/breg argument list from JSON. json must
     * be an array or the string "all". The array entries must be integers to
     * specify fixed qubit/creg/breg indices, or strings of the form "%<idx>",
     * where idx refers to a parameter with cQASM typespec Q, B, or I (resp.
     * qubit reference, bit reference, or integer variable reference). params
     * specifies the cQASM parameter typespec for the associated gate to check
     * validity of aforementioned. The result is written to args/all_args.
     */
    static void refs_from_json(const Json &json, const Str &params, Any<Value<UInt>> &args, Bool &all_args) {
        args.reset();
        if (json.is_string() && json.get<Str>() == "all") {
            all_args = true;
            return;
        } else if (!json.is_array()) {
            throw Exception("invalid value for ql_qubits/ql_cregs/ql_bregs: " + to_string(json));
        }
        all_args = false;
        for (const auto &json_ent : json) {
            if (json_ent.is_number()) {
                args.add(make<FixedValue<UInt>>(json_ent.get<UInt>()));
                continue;
            }
            if (json_ent.is_string()) {
                auto param_idx = parse_ref(json_ent.get<Str>(), params, "QBI");
                if (param_idx != MAX) {
                    args.add(make<UIntFromParameter>(param_idx));
                    continue;
                }
            }
            throw Exception("invalid entry for ql_qubits/ql_cregs/ql_bregs: " + to_string(json));
        }
    }

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
    ) {
        auto gcr = Ptr{new GateConversionRule(name, params)};
        gcr->cq_insn.set_annotation<typename GateConversionRule::Ptr>(gcr);
        if (!ql_name.empty()) {
            gcr->ql_name = ql_name;
        }
        return gcr;
    }
    
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
    static Ptr from_json(const Json &json) {

        // Construct default gate conversion from the mandatory arguments.
        auto name = json_get<Str>(json, "name");
        auto params = json_get<Str>(json, "params");
        Ptr gcr{ new GateConversionRule(name, params) };
        gcr->cq_insn.set_annotation<GateConversionRule::Ptr>(gcr);

        // Reconfigure the cQASM gate flags based on optional parameters.
        auto it = json.find("allow_conditional");
        if (it != json.end()) {
            gcr->cq_insn.allow_conditional = it->get<Bool>();
        }
        it = json.find("allow_parallel");
        if (it != json.end()) {
            gcr->cq_insn.allow_parallel = it->get<Bool>();
        }
        it = json.find("allow_reused_qubits");
        if (it != json.end()) {
            gcr->cq_insn.allow_reused_qubits = it->get<Bool>();
        }

        // Allow the OpenQL gate name to be overridden to support name
        // conversions.
        it = json.find("ql_name");
        if (it != json.end()) {
            gcr->ql_name = it->get<Str>();
        }

        // Allow operands to be overridden to reorder them, add constants, etc.
        it = json.find("ql_qubits");
        if (it != json.end()) {
            refs_from_json(*it, params, gcr->ql_qubits, gcr->ql_all_qubits);
        }
        it = json.find("ql_cregs");
        if (it != json.end()) {
            refs_from_json(*it, params, gcr->ql_cregs, gcr->ql_all_cregs);
        }
        it = json.find("ql_bregs");
        if (it != json.end()) {
            refs_from_json(*it, params, gcr->ql_bregs, gcr->ql_all_bregs);
        }

        // Parse duration operand configuration. If a number is specified, that
        // number is used as the duration. If it's a string of the form "%i"
        // where i is a parameter index, the duration is taken from a cQASM
        // parameter of type i (int). If no ql_duration key is specified, the
        // default from the GateConverter constructor is adequate.
        it = json.find("ql_duration");
        if (it != json.end()) {
            Bool ok = false;
            if (it->is_number()) {
                gcr->ql_duration = make<FixedValue<UInt>>(it->get<UInt>());
                ok = true;
            } else if (it->is_string()) {
                auto param_idx = parse_ref(it->get<Str>(), params, "i");
                if (param_idx != MAX) {
                    gcr->ql_duration = make<UIntFromParameter>(param_idx);
                    ok = true;
                }
            }
            if (!ok) {
                throw Exception("invalid entry for ql_duration: " + to_string(*it));
            }
        }

        // Parse the conversion method for the angle parameter, if any.
        AngleConversionMethod angle_method = AngleConversionMethod::RADIANS;
        it = json.find("ql_angle_method");
        if (it != json.end()) {
            auto method_name = it->get<Str>();
            if (method_name == "rad") {
                angle_method = AngleConversionMethod::RADIANS;
            } else if (method_name == "deg") {
                angle_method = AngleConversionMethod::DEGREES;
            } else if (method_name == "pow2") {
                angle_method = AngleConversionMethod::POWER_OF_TWO;
            } else {
                throw Exception("invalid entry for ql_angle_method: " + to_string(*it));
            }
        }

        // Parse angle operand configuration. If a number is specified, that
        // number is used as the angle, converted using the angle conversion
        // method. If it's a string of the form "%i" where i is a parameter
        // index, the duration is taken from a cQASM parameter of type i (int)
        // or r (real). If no ql_angle key is specified and there is a cQASM
        // parameter of type r, the first of those is used (this is also the
        // behavior of the GateConverter constructor, but it may need to be
        // reconstructed because the angle conversion method differs). Otherwise
        // the angle is simply set to 0.
        it = json.find("ql_angle");
        if (it != json.end()) {
            Bool ok = false;
            if (it->is_number()) {
                gcr->ql_angle = make<FixedValue<Real>>(convert_angle(it->get<Real>(), angle_method));
                ok = true;
            } else if (it->is_string()) {
                auto param_idx = parse_ref(it->get<Str>(), params, "ri");
                if (param_idx != MAX) {
                    gcr->ql_angle = make<AngleFromParameter>(param_idx, angle_method);
                    ok = true;
                }
            }
            if (!ok) {
                throw Exception("invalid entry for ql_angle: " + to_string(*it));
            }
        } else {
            auto param_idx = params.find_first_of('r');
            if (param_idx != Str::npos) {
                gcr->ql_angle = make<AngleFromParameter>(param_idx, angle_method);
            }
        }

        // Configure special conversion flags.
        it = json.find("implicit_sgmq");
        if (it != json.end()) {
            gcr->implicit_sgmq = it->get<Bool>();
        }
        it = json.find("implicit_breg");
        if (it != json.end()) {
            gcr->implicit_breg = it->get<Bool>();
        }

        return gcr;
    }

};

/**
 * Asserts that the given value is a condition register, and returns its index.
 */
static UInt expect_condition_reg(const lqv::Value &val) {
    if (auto v = val->as_variable_ref()) {
        return v->variable->get_annotation<VarIndex>().index;
    } else if (auto b = val->as_bit_refs()) {
        if (b->index.size() != 1) {
            throw Exception("expected a single condition at " + location(*val) + ", multiple found");
        }
        return itou(b->index[0]->value);
    } else {
        throw Exception("expected a condition variable " + location(*val));
    }
}

/**
 * Assign location information to the given node from the given set of values.
 * FIXME: this should really be in libqasm itself, and be used in its default
 *  function set.
 */
static void assign_location_from(lqt::Annotatable &node, const lqv::Values &vs) {
    Opt<lqp::SourceLocation> loc;
    for (const auto &v : vs) {
        if (auto newloc = v->get_annotation_ptr<lqp::SourceLocation>()) {
            if (loc) {
                loc->expand_to_include(newloc->first_line, newloc->first_column);
                loc->expand_to_include(newloc->last_line, newloc->last_column);
            } else {
                loc.emplace(*newloc);
            }
        }
    }
    if (loc) {
        node.set_annotation<lqp::SourceLocation>(*loc);
    }
}

/**
 * Boolean NOT operator for conditions.
 */
static lqv::Value op_linv_b(const lqv::Values &v) {
    lqv::Value retval;
    if (auto c = v[0]->as_const_bool()) {
        retval = lqt::make<lqv::ConstBool>(!c->value);
    } else {
        retval = lqt::make<lqv::Function>("operator!", v, lqt::make<lqtyp::Bool>());
    }
    assign_location_from(*retval, v);
    return retval;
}

/**
 * Boolean AND operator for conditions.
 */
static lqv::Value op_land_bb(const lqv::Values &v) {
    lqv::Value retval;
    auto a = v[0];
    auto b = v[1];
    if (b->as_const_bool()) {
        std::swap(a, b);
    }
    if (auto ca = a->as_const_bool()) {
        if (auto cb = b->as_const_bool()) {
            retval = lqt::make<lqv::ConstBool>(ca->value && cb->value);
        } else if (ca->value) {
            retval = b;
        } else {
            retval = lqt::make<lqv::ConstBool>(false);
        }
    } else {
        retval = lqt::make<lqv::Function>("operator&&", v, lqt::make<lqtyp::Bool>());
    }
    assign_location_from(*retval, v);
    return retval;
}

/**
 * Boolean OR operator for conditions.
 */
static lqv::Value op_lor_bb(const lqv::Values &v) {
    lqv::Value retval;
    auto a = v[0];
    auto b = v[1];
    if (b->as_const_bool()) {
        std::swap(a, b);
    }
    if (auto ca = a->as_const_bool()) {
        if (auto cb = b->as_const_bool()) {
            retval = lqt::make<lqv::ConstBool>(ca->value || cb->value);
        } else if (ca->value) {
            retval = lqt::make<lqv::ConstBool>(true);
        } else {
            retval = b;
        }
    } else {
        retval = lqt::make<lqv::Function>("operator||", v, lqt::make<lqtyp::Bool>());
    }
    assign_location_from(*retval, v);
    return retval;
}

/**
 * Boolean XOR operator for conditions.
 */
static lqv::Value op_lxor_bb(const lqv::Values &v) {
    lqv::Value retval;
    auto a = v[0];
    auto b = v[1];
    if (b->as_const_bool()) {
        std::swap(a, b);
    }
    if (auto ca = a->as_const_bool()) {
        if (auto cb = b->as_const_bool()) {
            retval = lqt::make<lqv::ConstBool>(ca->value || cb->value);
        } else if (ca->value) {
            lqv::Values vs;
            vs.add(b);
            retval = lqt::make<lqv::Function>("operator!", vs, lqt::make<lqtyp::Bool>());
        } else {
            retval = b;
        }
    } else {
        retval = lqt::make<lqv::Function>("operator^^", v, lqt::make<lqtyp::Bool>());
    }
    assign_location_from(*retval, v);
    return retval;
}

/**
 * Private implementation for the opaque public Reader class; as in it's not in
 * public headers, reducing compile time.
 */
class ReaderImpl {
private:

    /**
     * OpenQL platform reference to compile for.
     */
    const quantum_platform &platform;

    /**
     * OpenQL program to add loaded circuits to.
     */
    quantum_program &program;

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
    lqa::Analyzer build_analyzer() {
        
        // If no gateset has been specified yet, add a default one for backward
        // compatibility purposes. This default emulates the behavior of the
        // convertor from before it was configurable.
        //
        // Note that some instructions were previously just silently ignored
        // due to not being recognized. These are:
        //  - U (single-qubit unitary)
        //  - measure_parity
        //  - display_binary
        //  - display for specific bitset
        //  - not
        //  - reset-averaging
        //  - load_state
        // They result in libqasm errors now instead. Note that skip is not in
        // the list above but also not below; it is processed separately to set
        // instruction cycles.
        if (gateset.empty()) {
            gateset.push_back(GateConversionRule::from_defaults("measure", "Q", "measz"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.push_back(GateConversionRule::from_defaults("measure_x", "Q", "measx"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.push_back(GateConversionRule::from_defaults("measure_y", "Q", "measy"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.push_back(GateConversionRule::from_defaults("measure_z", "Q", "measz"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.push_back(GateConversionRule::from_defaults("prep", "Q", "prepz"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.push_back(GateConversionRule::from_defaults("prep_x", "Q", "prepx"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.push_back(GateConversionRule::from_defaults("prep_y", "Q", "prepy"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.push_back(GateConversionRule::from_defaults("prep_z", "Q", "prepz"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.push_back(GateConversionRule::from_defaults("i", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("h", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("x", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("y", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("z", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("s", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("sdag", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("t", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("tdag", "Q"));
            gateset.push_back(GateConversionRule::from_defaults("x90", "Q", "rx90"));
            gateset.push_back(GateConversionRule::from_defaults("y90", "Q", "ry90"));
            gateset.push_back(GateConversionRule::from_defaults("mx90", "Q", "xm90"));
            gateset.push_back(GateConversionRule::from_defaults("my90", "Q", "ym90"));
            gateset.push_back(GateConversionRule::from_defaults("rx", "Qr"));
            gateset.push_back(GateConversionRule::from_defaults("ry", "Qr"));
            gateset.push_back(GateConversionRule::from_defaults("rz", "Qr"));
            gateset.push_back(GateConversionRule::from_defaults("cnot", "QQ"));
            gateset.push_back(GateConversionRule::from_defaults("cz", "QQ"));
            gateset.push_back(GateConversionRule::from_defaults("swap", "QQ"));
            gateset.push_back(GateConversionRule::from_defaults("cr", "QQr"));
            gateset.push_back(GateConversionRule::from_defaults("crk", "QQi"));
            gateset.back()->ql_angle = make<AngleFromParameter>(2, AngleConversionMethod::POWER_OF_TWO);
            gateset.push_back(GateConversionRule::from_defaults("toffoli", "QQQ"));
            gateset.push_back(GateConversionRule::from_defaults("measure_all", "", "measz"));
            gateset.back()->ql_all_qubits = true;
            gateset.back()->implicit_sgmq = true;
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.back()->cq_insn.allow_parallel = false;
            gateset.push_back(GateConversionRule::from_defaults("display", ""));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.back()->cq_insn.allow_parallel = false;
            gateset.push_back(GateConversionRule::from_defaults("wait", ""));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.back()->cq_insn.allow_parallel = false;
            gateset.push_back(GateConversionRule::from_defaults("wait", "i"));
            gateset.back()->cq_insn.allow_conditional = false;
            gateset.back()->cq_insn.allow_parallel = false;
        }
        
        // Construct the actual analyzer.
        auto a = lqa::Analyzer("1.1");
        a.register_default_functions_and_mappings();
        a.register_function("operator!", "b", op_linv_b);
        a.register_function("operator&&", "bb", op_land_bb);
        a.register_function("operator^^", "bb", op_lxor_bb);
        a.register_function("operator||", "bb", op_lor_bb);
        for (const auto &gate : gateset) {
            a.register_instruction(gate->cq_insn);
            a.register_instruction("skip", "i", false, false);
        }
        return a;
    }

    /**
     * Handles the parse result of string2circuit() and file2circuit().
     */
    void handle_parse_result(lqa::AnalysisResult &&ar) {

        // If parsing failed, print any parse errors using OpenQL's logging
        // facilities, then throw an exception.
        if (!ar.errors.empty()) {
            StrStrm errs;
            Bool first = true;
            for (const auto &error : ar.errors) {
                QL_EOUT(error);
                if (first) {
                    first = false;
                } else {
                    errs << "; ";
                }
                errs << error;
            }
            throw Exception(errs.str());
        }

        // Error models are not supported by OpenQL.
        if (!ar.root->error_model.empty()) {
            QL_IOUT("ignoring cQASM error model");
        }

        // Map cQASM variables and qubits to OpenQL qubits, cregs, and bregs
        // using the following rules:
        //  - The qubits of the qubits statement (if any, it's optional in 1.1)
        //    are mapped to the first N qubits.
        //  - Qubit variables are mapped to qubits after that in the order in
        //    which they appear in the file. Liveness analysis etc. is NOT
        //    performed; qubit indices are never reused.
        //  - Integer variables are mapped to cregs in the order in which they
        //    appear.
        //  - Boolean variables are mapped to explicit bregs, i.e. after the
        //    ones that have a qubit associated with them.
        UInt num_qubits = itou(ar.root->num_qubits);
        UInt num_cregs = 0;
        UInt num_bregs = platform.qubit_number;
        for (auto &var : ar.root->variables) {
            if (var->typ->as_qubit()) {
                var->set_annotation(VarIndex{num_qubits++});
            } else if (var->typ->as_int()) {
                var->set_annotation(VarIndex{num_cregs++});
            } else if (var->typ->as_bool()) {
                var->set_annotation(VarIndex{num_bregs++});
            } else {
                throw Exception("only int, bool, and qubit variables are supported by OpenQL (" + location(*var) + ")");
            }
        }
        if (num_qubits > platform.qubit_number) {
            throw Exception("cQASM file needs " + to_string(num_qubits) + " qubits, but platform only supports " + to_string(platform.qubit_number));
        }
        if (num_qubits > program.qubit_count) {
            QL_IOUT("increasing program qubit count from " << program.qubit_count << " to " << num_qubits);
            program.qubit_count = num_qubits;
        }
        if (num_cregs > program.creg_count) {
            QL_IOUT("increasing program creg count from " << program.creg_count << " to " << num_cregs);
            program.creg_count = num_cregs;
        }
        if (num_bregs > program.breg_count) {
            QL_IOUT("increasing program breg count from " << program.breg_count << " to " << num_bregs);
            program.breg_count = num_bregs;
        }

        // Add the subcircuits one by one.
        for (const auto &sc : ar.root->subcircuits) {

            // Construct the kernel for this subcircuit. Note that kernel names
            // must be unique in OpenQL, but subcircuits don't need to be in
            // cQASM. Also, multiple cQASM files can be added to a single
            // program, so even if that would be a requirement, it wouldn't be
            // unique enough. So we add a number to them for uniquification.
            quantum_kernel kernel(
                sc->name + "_" + to_string(subcircuit_count++),
                platform,
                num_qubits,
                num_cregs,
                num_bregs
            );

            // Set the cycle numbers in the OpenQL circuit based on cQASM's
            // timing rules; that is, the instructions in each bundle start
            // simultaneously, the next bundle starts in the next cycle, and
            // the skip instruction can be used to advance time. The wait
            // instruction, conversely, only serves to guide the scheduler, and
            // thus does nothing here. Note that the cycle times start at one
            // because someone thought that was a good idea at the time. Note
            // also that the cycle times will certainly be invalid if any cQASM
            // gate converts to a gate decomposition rule rather than a
            // primitive gate.
            UInt cycle = 1;
            Bool cycles_might_be_valid = true;
            UInt num_gates = 0;
            for (const auto &bundle : sc->bundles) {

                // Handle skip instructions/bundles.
                if (bundle->items.size() == 1 && bundle->items.at(0)->name == "skip") {
                    const auto &ops = bundle->items.at(0)->operands;
                    QL_ASSERT(ops.size() == 1);
                    auto ci = ops.at(0)->as_const_int();
                    if (!ci) {
                        throw Exception("skip durations must be constant at " + location(*ops.at(0)));
                    }
                    if (ci->value < 1) {
                        throw Exception("skip durations must be positive at " + location(*ops.at(0)));
                    }
                    cycle += ci->value;
                    continue;
                }

                // Loop over the parallel instructions.
                for (const auto &insn : bundle->items) {
                    const auto &gcr = insn->instruction->get_annotation<GateConversionRule::Ptr>();

                    // Handle gate conditions.
                    cond_type_t cond = e_cond_type::cond_always;
                    Vec<UInt> cond_bregs;
                    if (auto ccb = insn->condition->as_const_bool()) {
                        if (ccb->value) {
                            cond = e_cond_type::cond_always;
                        } else {
                            cond = e_cond_type::cond_never;
                        }
                    } else if (auto fun = insn->condition->as_function()) {
                        Bool invert = false;
                        while (fun->name == "operator!") {
                            invert = !invert;
                            if (auto fun2 = fun->operands[0]->as_function()) {
                                fun = fun2;
                                continue;
                            }
                            cond_bregs.push_back(expect_condition_reg(fun->operands[0]));
                            if (invert) {
                                cond = e_cond_type::cond_not;
                            } else {
                                cond = e_cond_type::cond_unary;
                            }
                            fun = nullptr;
                            break;
                        }
                        if (fun) {
                            if (fun->name == "operator&&") {
                                if (invert) {
                                    cond = e_cond_type::cond_nand;
                                } else {
                                    cond = e_cond_type::cond_and;
                                }
                            } else if (fun->name == "operator||") {
                                if (invert) {
                                    cond = e_cond_type::cond_nor;
                                } else {
                                    cond = e_cond_type::cond_or;
                                }
                            } else if (fun->name == "operator^^") {
                                if (invert) {
                                    cond = e_cond_type::cond_nxor;
                                } else {
                                    cond = e_cond_type::cond_xor;
                                }
                            }
                            cond_bregs.push_back(expect_condition_reg(fun->operands[0]));
                            cond_bregs.push_back(expect_condition_reg(fun->operands[1]));
                        }
                    } else {
                        cond_bregs.push_back(expect_condition_reg(insn->condition));
                        cond = e_cond_type::cond_unary;
                    }

                    // Figure out if this instruction uses
                    // single-gate-multiple-qubit (SGMQ) notation.
                    UInt sgmq_count = 0;
                    for (const auto &op : insn->operands) {
                        UInt cur_sgmq_count;
                        if (const auto qr = op->as_qubit_refs()) {
                            cur_sgmq_count = qr->index.size();
                        } else if (const auto br = op->as_bit_refs()) {
                            cur_sgmq_count = br->index.size();
                        } else {
                            continue;
                        }
                        QL_ASSERT(cur_sgmq_count > 0);
                        if (sgmq_count) {
                            QL_ASSERT(cur_sgmq_count == sgmq_count);
                        }
                        sgmq_count = cur_sgmq_count;
                    }
                    if (!sgmq_count) {
                        sgmq_count = 1;
                    }

                    // Loop over the single-gate-multiple-qubit instances of the
                    // instruction and add an OpenQL gate for each, as OpenQL
                    // does not support this abstraction.
                    for (UInt sgmq_index = 0; sgmq_index < sgmq_count; sgmq_index++) {

                        // Determine qubit argument list.
                        utils::Vec<utils::UInt> qubits;
                        for (const auto &arg : gcr->ql_qubits) {
                            qubits.push_back(arg->get(insn->operands, sgmq_index));
                        }
                        if (gcr->ql_all_qubits) {
                            for (UInt qubit = 0; qubit < num_qubits; qubit++) {
                                qubits.push_back(qubit);
                            }
                        }

                        // Determine creg argument list.
                        utils::Vec<utils::UInt> cregs;
                        for (const auto &arg : gcr->ql_cregs) {
                            cregs.push_back(arg->get(insn->operands, sgmq_index));
                        }
                        if (gcr->ql_all_cregs) {
                            for (UInt creg = 0; creg < num_cregs; creg++) {
                                cregs.push_back(creg);
                            }
                        }

                        // Determine breg argument list.
                        utils::Vec<utils::UInt> bregs;
                        for (const auto &arg : gcr->ql_bregs) {
                            bregs.push_back(arg->get(insn->operands, sgmq_index));
                        }
                        if (gcr->ql_all_bregs) {
                            for (UInt breg = 0; breg < num_bregs; breg++) {
                                cregs.push_back(breg);
                            }
                        }

                        // Determine duration and angle.
                        utils::UInt duration = gcr->ql_duration->get(insn->operands, sgmq_index);
                        utils::Real angle = gcr->ql_angle->get(insn->operands, sgmq_index);

                        // Handle gates with implicit single-gate-multiple-qubit
                        // behavior.
                        UInt impl_sgmq_count = gcr->implicit_sgmq ? qubits.size() : 1;
                        for (UInt impl_sgmq_index = 0; impl_sgmq_index < impl_sgmq_count; impl_sgmq_index++) {
                            utils::Vec<utils::UInt> cur_qubits;
                            if (gcr->implicit_sgmq) {
                                cur_qubits = {qubits.at(impl_sgmq_index)};
                            } else {
                                cur_qubits = qubits;
                            }

                            // Add implicit bregs if needed.
                            auto cur_bregs = bregs;
                            if (gcr->implicit_breg) {
                                cur_bregs.insert(cur_bregs.cend(), cur_qubits.cbegin(), cur_qubits.cend());
                            }

                            // Add the gate to the kernel.
                            kernel.gate(gcr->ql_name, cur_qubits, cregs, duration, angle, bregs, cond, cond_bregs);

                            // If that added more than one gate, invalidate
                            // timing information.
                            if (kernel.c.size() > num_gates + 1) {
                                cycles_might_be_valid = false;
                            }

                            // Set timing information for the added gates.
                            while (num_gates < kernel.c.size()) {
                                kernel.c.at(num_gates++)->cycle = cycle;
                            }

                        }

                    }

                }

                // End of normal bundle; increment cycle.
                cycle++;
            }

            // Assume that the cycle times in the cQASM schedule are valid if
            // they pass sanity checks (the cQASM file may already have been
            // scheduled).
            if (cycles_might_be_valid) {
                QL_IOUT("cQASM schedule for kernel " << kernel.name << " *might* be valid");
                kernel.cycles_valid = cycles_might_be_valid;
            } else {
                QL_IOUT("cQASM schedule for kernel " << kernel.name << " is invalid; kernel needs to be (re)scheduled");
            }

            // Append the kernel to program.
            if (sc->iterations > 1) {
                program.add_for(kernel, sc->iterations);
            } else {
                program.add(kernel);
            }

        }

    }

public:

    /**
     * Constructs a reader.
     */
    ReaderImpl(
        const quantum_platform &platform,
        quantum_program &program
    ) :
        platform(platform),
        program(program),
        gateset(),
        subcircuit_count(0)
    {}

    /**
     * Load libqasm gateset and conversion rules to OpenQL gates from a JSON
     * object. Any existing gateset conversion rules are first deleted.
     *
     * The toplevel JSON object should be an array of objects, where each object
     * represents a libqasm gate (overload) and its conversion to OpenQL. The
     * expected structure of these objects is described in
     * GateConverter::from_json().
     */
    void load_gateset(const Json &json) {
        gateset.clear();
        if (!json.is_array()) {
            throw Exception("cQASM gateset JSON should be an array at the top level");
        }
        for (const auto &el : json) {
            gateset.push_back(GateConversionRule::from_json(el));
        }
    }

    /**
     * Parses a cQASM string using the gateset selected when the Reader is
     * constructed, converts the cQASM kernels to OpenQL kernels, and adds those
     * kernels to the selected OpenQL program.
     */
    void string2circuit(const utils::Str &cqasm_str) {
        handle_parse_result(build_analyzer().analyze_string(cqasm_str));
    }

    /**
     * Parses a cQASM file using the gateset selected when the Reader is
     * constructed, converts the cQASM kernels to OpenQL kernels, and adds those
     * kernels to the selected OpenQL program.
     */
    void file2circuit(const utils::Str &cqasm_fname) {
        handle_parse_result(build_analyzer().analyze(cqasm_fname));
    }

};

/**
 * Constructs a cQASM reader with the default cQASM gateset and conversion
 * rules. This is here for backward compatibility; new code should use a JSON
 * file for the gateset and conversion rules, or take the JSON from the platform
 * configuration file.
 */
Reader::Reader(
    const quantum_platform &platform,
    quantum_program &program
) : impl(platform, program) {}

/**
 * Constructs a cQASM reader with a custom gateset from a JSON structure. The
 * JSON structure should be an array of objects, where every object represents
 * a cQASM gate (overload) and the rules for converting it to OpenQL gate(s).
 * The expected structure of these objects is described in
 * GateConverter::from_json().
 */
Reader::Reader(
    const quantum_platform &platform,
    quantum_program &program,
    const Json &gateset
) : impl(platform, program) {
    impl->load_gateset(gateset);
}

/**
 * Constructs a cQASM reader with a custom gateset from a JSON file. The
 * structure of the JSON file should be an array of objects, where every object
 * represents a cQASM gate (overload) and the rules for converting it to OpenQL
 * gate(s). The expected structure of these objects is described in
 * GateConverter::from_json().
 */
Reader::Reader(
    const quantum_platform &platform,
    quantum_program &program,
    const Str &gateset_fname
) : impl(platform, program) {
    impl->load_gateset(load_json(gateset_fname));
}

/**
 * Parses a cQASM string using the gateset selected when the Reader is
 * constructed, converts the cQASM kernels to OpenQL kernels, and adds those
 * kernels to the selected OpenQL program.
 */
void Reader::string2circuit(const utils::Str &cqasm_str) {
    impl->string2circuit(cqasm_str);
}

/**
 * Parses a cQASM file using the gateset selected when the Reader is
 * constructed, converts the cQASM kernels to OpenQL kernels, and adds those
 * kernels to the selected OpenQL program.
 */
void Reader::file2circuit(const utils::Str &cqasm_fname) {
    impl->file2circuit(cqasm_fname);
}

} // namespace cqasm
} // namespace ql
