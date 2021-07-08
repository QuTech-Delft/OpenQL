/** \file
 * Provides the conversion from the old IR (still used for the API for backward
 * compatibility) to the new one.
 */

#include "ql/ir/old_to_new.h"

#include "ql/ir/ops.h"
#include "ql/ir/consistency.h"
#include "ql/rmgr/manager.h"

namespace ql {
namespace ir {

/**
 * Takes an instruction name from the JSON file, "sanitizes" it according to
 * the legacy platform loading rules, and splits it into the name and its
 * specialization/decomposition template parameters.
 */
static utils::List<utils::Str> parse_instruction_name(utils::Str name) {

    // Sanitize according to the legacy rules.
    name = utils::to_lower(name);
    static const std::regex TRIM("^(\\s+)|(\\s+)$");
    name = std::regex_replace(name, TRIM, "");
    static const std::regex SPACES("[\\s,]+");
    name = std::regex_replace(name, SPACES, " ");

    // Split on spaces.
    utils::UInt pos = 0;
    utils::List<utils::Str> template_params;
    while (true) {
        auto next = name.find_first_of(' ', pos);
        template_params.push_back(name.substr(pos, next - pos));
        if (next == utils::Str::npos) break;
        pos = next + 1;
    }

    return template_params;
}

/**
 * Parses a parameter from an instruction or decomposition key.
 */
static ExpressionRef parse_instruction_parameter(
    const Ref &ir,
    const utils::Str &param
) {
    if (std::regex_match(param, std::regex("[qbc][0-9]+"))) {
        auto name = param.substr(0, 1);
        auto index = utils::parse_uint(param.substr(1));

        // Map the first num_qubits bregs to the implicit qubit measurement
        // registers.
        auto implicit_breg = false;
        if (name == "breg") {
            auto num_qubits = get_num_qubits(ir);
            if (index < num_qubits) {
                implicit_breg = true;
                name = "q";
            } else {
                index -= num_qubits;
            }
        }
        auto obj = find_physical_object(ir, name);
        if (obj.empty()) {
            throw utils::Exception(
                "invalid specialization parameter \"" + param + "\": "
                "no register exists with that name"
            );
        }
        if (obj->shape.size() != 1) {
            throw utils::Exception(
                "invalid specialization parameter \"" + param + "\": "
                "register has invalid shape"
            );
        }
        if (index >= obj->shape[0]) {
            throw utils::Exception(
                "invalid specialization parameter \"" + param + "\": "
                "register index out of range"
            );
        }
        auto ref = make_reference(ir, obj, {index});
        if (implicit_breg) {
            ref->data_type = ir->platform->implicit_bit_type;
        }
        return ref;
    } else {
        auto obj = find_physical_object(ir, param.substr(0, 1));
        if (obj.empty()) {
            throw utils::Exception(
                "invalid specialization parameter \"" + param + "\": "
                "no register exists with that name"
            );
        }
        return make_reference(ir, obj);
    }
}

/**
 * Converts the old platform to the new IR structure.
 *
 * See convert_old_to_new(const compat::ProgramRef&) for details.
 */
Ref convert_old_to_new(const compat::PlatformRef &old) {
    Ref ir;
    ir.emplace();

    // Build the platform.
    ir->platform.emplace();
    ir->platform->name = old->name;

    // Add qubit type and main qubit register.
    auto qubit_type = add_type<QubitType>(ir, "qubit");
    ir->platform->qubits = add_physical_object(ir, utils::make<PhysicalObject>(
        "q", qubit_type, prim::UIntVec({old->qubit_count})
    ));

    // Add type for bregs and conditions.
    auto bit_type = add_type<BitType>(ir, "bit");
    ObjectLink bregs;
    if (old->breg_count > old->qubit_count) {
        bregs = add_physical_object(ir, utils::make<PhysicalObject>(
            "breg", bit_type, prim::UIntVec({old->breg_count - old->qubit_count})
        ));
    }
    ir->platform->implicit_bit_type = bit_type;
    ir->platform->default_bit_type = bit_type;

    // Add type and registers for cregs.
    auto int_type = add_type<IntType>(ir, "int", true, 32);
    if (old->creg_count) {
        add_physical_object(ir, utils::make<PhysicalObject>(
            "creg", int_type, prim::UIntVec({old->creg_count})
        ));
    }
    ir->platform->default_int_type = int_type;

    // Add type for angle operands.
    auto real_type = add_type<RealType>(ir, "real");

    // Add the instruction set. We load this from the JSON data rather than
    // trying to use instruction_map, because the latter has some pretty ****ed
    // up stuff going on in it to make the legacy decompositions work. We need
    // to order by number of template args first: generalizations must be done
    // first, otherwise the generalization will be inferred from the
    // specialization and any extra data for the generalization will be lost.
    struct UnparsedGateType {
        utils::Str name;
        utils::List<utils::Str> name_parts;
        utils::Json data;
        utils::Bool operator<(const UnparsedGateType &other) const {
            if (name_parts.size() < other.name_parts.size()) return true;
            if (name_parts.size() > other.name_parts.size()) return false;
            return name < other.name;
        }
    };
    utils::List<UnparsedGateType> unparsed_gate_types;
    for (
        auto it = old->get_instructions().begin();
        it != old->get_instructions().end();
        ++it
    ) {
        unparsed_gate_types.push_back({
            it.key(),
            parse_instruction_name(it.key()),
            *it
        });
    }
    unparsed_gate_types.sort();
    for (const UnparsedGateType &unparsed_gate_type : unparsed_gate_types) {
        try {

            // Create an instruction node for the incoming instruction.
            auto insn = utils::make<InstructionType>();
            insn->data = unparsed_gate_type.data;

            // The instruction name is in it.key(). However:
            //  - this may include specialization parameters; and
            //  - someone thought it'd be a good idea to let people write absolute
            //    garbage and then "sanitize" with some regexes. Ugh.
            auto template_params = unparsed_gate_type.name_parts;
            auto name = template_params.front();
            template_params.pop_front();
            insn->name = name;
            if (!std::regex_match(insn->name, IDENTIFIER_RE)) {
                throw utils::Exception(
                    "instruction name is not a valid identifier"
                );
            }

            // Determine the cQASM name.
            auto it2 = insn->data->find("cqasm_name");
            if (it2 == insn->data->end()) {
                insn->cqasm_name = insn->name;
            } else if (it2->is_string()) {
                insn->cqasm_name = it2->get<utils::Str>();
                if (!std::regex_match(insn->cqasm_name, IDENTIFIER_RE)) {
                    throw utils::Exception(
                        "cQASM name is not a valid identifier"
                    );
                }
            } else {
                throw utils::Exception(
                    "cqasm_name key must be a string if specified"
                );
            }

            // Parse the template parameters.
            utils::Any<Expression> template_operands;
            for (const auto &param : template_params) {
                template_operands.add(parse_instruction_parameter(ir, param));
            }

            // Determine the operand types. This is *hard*, because in legacy
            // platform descriptions this simply isn't specified. So, if the
            // parameters key is missing, we have to try to make an educated
            // guess about what they are. There's a list of instruction names
            // with hardcoded parameter sets, and for everything else we guess
            // that there are no parameters. If we then determine we guessed
            // wrong while scanning the instructions in the program, we add
            // overloads as needed.
            utils::Bool duplicate_with_breg_arg = false;
            it2 = insn->data->find("parameters");
            if (it2 != insn->data->end()) {

                // Read parameters from JSON.
                if (!it2->is_array()) {
                    throw utils::Exception(
                        "parameter list must be a JSON array if specified"
                    );
                }
                for (const auto &it3 : *it2) {
                    if (!it3.is_string()) {
                        throw utils::Exception("parameters must be strings");
                    }
                    auto param = it3.get<utils::Str>();
                    auto pos = param.find_first_of(':');
                    auto type_s = param.substr(0, pos);
                    auto type = find_type(ir, type_s);
                    if (type.empty()) {
                        throw utils::Exception("unknown parameter type " + type_s);
                    }
                    utils::Str mode_s = "W";
                    if (pos != utils::Str::npos) {
                        mode_s = param.substr(pos);
                    }
                    prim::OperandMode mode;
                    if (mode_s == "W") {
                        mode = prim::OperandMode::WRITE;
                    } else if (mode_s == "R") {
                        mode = prim::OperandMode::READ;
                        if (type->as_qubit_type()) {
                            throw utils::Exception(
                                "invalid parameter mode R for qubit type"
                            );
                        }
                    } else if (mode_s == "L") {
                        mode = prim::OperandMode::LITERAL;
                        if (type->as_qubit_type()) {
                            throw utils::Exception(
                                "invalid parameter mode L for qubit type"
                            );
                        }
                    } else if (mode_s == "X") {
                        mode = prim::OperandMode::COMMUTE_X;
                        if (type->as_classical_type()) {
                            throw utils::Exception(
                                "invalid parameter mode X for classical type"
                            );
                        }
                    } else if (mode_s == "Y") {
                        mode = prim::OperandMode::COMMUTE_Y;
                        if (type->as_classical_type()) {
                            throw utils::Exception(
                                "invalid parameter mode Y for classical type"
                            );
                        }
                    } else if (mode_s == "Z") {
                        mode = prim::OperandMode::COMMUTE_Z;
                        if (type->as_classical_type()) {
                            throw utils::Exception(
                                "invalid parameter mode Z for classical type"
                            );
                        }
                    } else if (mode_s == "M") {
                        mode = prim::OperandMode::MEASURE;
                        if (type->as_classical_type()) {
                            throw utils::Exception(
                                "invalid parameter mode M for classical type"
                            );
                        }
                    } else {
                        throw utils::Exception(
                            "invalid parameter mode " + mode_s + ": "
                            "must be W, R, L, X, Y, Z, or M"
                        );
                    }
                    insn->operand_types.emplace(mode, type);
                }

                // Check whether the operand list matches the specializations
                // specified in the old way.
                if (insn->template_operands.size() > insn->operand_types.size()) {
                    throw utils::Exception(
                        "need at least operands for the specialization parameters"
                    );
                } else {
                    for (utils::UInt i = 0; i < template_params.size(); i++) {
                        if (insn->operand_types[i]->data_type != get_type_of(insn->template_operands[i])) {
                            throw utils::Exception(
                                "specialization parameter operand type mismatch"
                            );
                        }
                    }
                }

            } else {

                // We have to infer the prototype somehow...
                if (std::regex_match(insn->name, std::regex("h|i|move_init|prep_?[xyz]"))) {

                    // Single-qubit gate that doesn't commute in any way we can
                    // represent.
                    insn->operand_types.emplace(prim::OperandMode::WRITE, qubit_type);

                } else if (insn->name == "rx") {

                    // Single-qubit X rotation gate.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_X, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::LITERAL, real_type);

                } else if (std::regex_match(insn->name, std::regex("(m|mr|r)?xm?[0-9]*"))) {

                    // Single-qubit gate that commutes on the X axis.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_X, qubit_type);

                } else if (insn->name == "ry") {

                    // Single-qubit Y rotation gate.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Y, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::LITERAL, real_type);

                } else if (std::regex_match(insn->name, std::regex("(m|mr|r)?ym?[0-9]*"))) {

                    // Single-qubit gate that commutes on the Y axis.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Y, qubit_type);

                } else if (insn->name == "rz") {

                    // Single-qubit Z rotation gate.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::LITERAL, real_type);

                } else if (insn->name == "crz" || insn->name == "cr") {

                    // Controlled Z rotation gate.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::LITERAL, real_type);

                } else if (insn->name == "crk") {

                    // Controlled Z rotation gate.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::LITERAL, int_type);

                } else if (std::regex_match(insn->name, std::regex("[st](dag)?|(m|mr|r)?zm?[0-9]*"))) {

                    // Single-qubit gate that commutes on the Z axis.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);

                } else if (std::regex_match(insn->name, std::regex("meas(ure)?(_?[xyz])?(_keep)?"))) {

                    // Measurements.
                    duplicate_with_breg_arg = true;
                    insn->operand_types.emplace(prim::OperandMode::MEASURE, qubit_type);

                } else if (std::regex_match(insn->name, std::regex("(teleport)?(move|swap)"))) {

                    // Swaps.
                    insn->operand_types.emplace(prim::OperandMode::WRITE, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::WRITE, qubit_type);

                } else if (insn->name == "cnot" || insn->name == "cx") {

                    // Controlled X.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_X, qubit_type);

                } else if (insn->name == "cz" || insn->name == "cphase") {

                    // Controlled phase.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);

                } else if (insn->name == "cz_park") {

                    // Parking cz (assume only one parked qubit at the end).
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::IGNORE, qubit_type);

                } else if (insn->name == "toffoli") {

                    // Toffoli gate.
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::OperandMode::COMMUTE_X, qubit_type);

                }

                // If the instruction is specialized, we need at least qubit
                // args for those template parameters. If our guess didn't give
                // us that, just assume the template parameters are the only
                // ones, and don't infer any commutation rules.
                auto invalid = false;
                if (template_operands.size() > insn->operand_types.size()) {
                    invalid = true;
                } else {
                    for (utils::UInt i = 0; i < template_operands.size(); i++) {
                        if (insn->operand_types[i]->data_type != get_type_of(template_operands[i])) {
                            invalid = true;
                        }
                    }
                }
                if (invalid) {
                    insn->operand_types.reset();
                    for (utils::UInt i = 0; i < template_operands.size(); i++) {
                        insn->operand_types.emplace(
                            prim::OperandMode::WRITE,
                            get_type_of(template_operands[i])
                        );
                    }
                }

            }

            // Determine the duration of the instruction.
            utils::UInt duration_divider = 1;
            it2 = insn->data->find("duration_cycles");
            if (it2 == insn->data->end()) {
                it2 = insn->data->find("duration");
                duration_divider = old->cycle_time;
            } else if (insn->data->find("duration_cycles") != insn->data->end()) {
                throw utils::Exception(
                    "both duration and duration_cycles are specified; "
                    "please specify one or the other"
                );
            }
            if (it2 == insn->data->end()) {
                insn->duration = 1;
            } else if (it2->is_number_float()) {
                auto val = it2->get<utils::Real>();
                if (val < 0) {
                    throw utils::Exception(
                        "found negative duration (or integer overflow occurred)"
                    );
                }
                insn->duration = utils::ceil(val / (utils::Real)duration_divider);
            } else if (it2->is_number_integer()) {
                auto val = it2->get<utils::Int>();
                if (val < 0) {
                    throw utils::Exception(
                        "found negative duration (or integer overflow occurred)"
                    );
                }
                insn->duration = utils::div_ceil((utils::UInt)val, duration_divider);
            } else {
                throw utils::Exception(
                    "duration(_cycles) must be a number when specified"
                );
            }

            // Now actually add the instruction type.
            add_instruction_type(ir, insn, template_operands);

            // If this is a legacy measurement instruction, also add a variant
            // with an explicit breg.
            if (duplicate_with_breg_arg) {
                insn = insn.clone();
                insn->operand_types[0]->mode = prim::OperandMode::WRITE;
                insn->operand_types.emplace(prim::OperandMode::WRITE, bit_type);
                add_instruction_type(ir, insn, template_operands);
            }

        } catch (utils::Exception &e) {
            e.add_context("in gate description for '" + unparsed_gate_type.name + "'");
            throw;
        }
    }

    // Add legacy decompositions to the new system, for gates added by passes
    // (notably swap and relatives for the mapper).
    utils::List<std::function<void()>> todo;
    auto it = old->platform_config.find("gate_decomposition");
    if (it != old->platform_config.end()) {
        for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
            try {

                // Create an instruction node for the incoming instruction.
                auto insn = utils::make<InstructionType>();

                // Also create a decomposition node for it.
                auto decomp = utils::make<InstructionDecomposition>();
                decomp->name = "legacy";
                insn->decompositions.add(decomp);

                // Figure out the name and template parameters.
                auto template_params = parse_instruction_name(it2.key());
                insn->name = template_params.front();
                insn->cqasm_name = insn->name;
                template_params.pop_front();
                if (!std::regex_match(insn->name, IDENTIFIER_RE)) {
                    throw utils::Exception(
                        "instruction name is not a valid identifier"
                    );
                }

                // Parse the template parameters.
                utils::Any<Expression> template_operands;
                utils::UInt parameter_count = 0;
                for (const auto &param : template_params) {
                    if (utils::starts_with(param, "%")) {
                        if (param != "%" + utils::to_string(parameter_count)) {
                            throw utils::Exception(
                                "% parameters must start with 0 and be "
                                "consecutive"
                            );
                        }
                        parameter_count++;
                    } else if (parameter_count) {
                        throw utils::Exception(
                            "specialization parameters are not allowed "
                            "after % parameters"
                        );
                    } else {
                        template_operands.add(parse_instruction_parameter(ir, param));
                    }
                }

                // Add operands for the template operand types.
                for (const auto &template_operand : template_operands) {
                    insn->operand_types.emplace(
                        prim::OperandMode::WRITE,
                        get_type_of(template_operand)
                    );
                }

                // Legacy decompositions only support qubit operands, and we
                // know exactly how many. So this is way easier than it was for
                // normal instructions.
                for (utils::UInt i = 0; i < parameter_count; i++) {
                    insn->operand_types.emplace(prim::OperandMode::WRITE, qubit_type);
                    decomp->parameters.emplace("", qubit_type, prim::UIntVec());
                }

                // Figure out the instructions that the to-be-decomposed
                // instruction maps to.
                auto sub_insns = it2.value();
                if (!sub_insns.is_array()) {
                    throw utils::Exception("decomposition must be an array");
                }

                // Set the duration to a placeholder for now.
                insn->duration = utils::UMAX;

                // Now actually add the decomposition rule, even though we
                // haven't computed the expansion yet.
                auto insn_ref = add_decomposition_rule(ir, insn, template_operands);

                // We can only compute the expansion when all instruction types
                // have been created, and decompositions can create new
                // instruction types. Thus, we have to postpone this process.
                todo.push_back([ir, sub_insns, decomp, insn_ref]() {

                    // Make a trivial guess for the duration by just summing the
                    // durations of the sub-instructions, which we'll do while
                    // resolving them in the loop below. This is not a very
                    // realistic mode, of course, since sub-instructions may end up
                    // being executed in parallel.
                    utils::UInt duration = 0;

                    // Parse and add the sub-instructions.
                    for (const auto &sub_insn : sub_insns) {

                        // Parse the sub-instruction.
                        if (!sub_insn.is_string()) {
                            throw utils::Exception("sub-instructions must be strings");
                        }
                        auto sub_insn_params = parse_instruction_name(sub_insn.get<utils::Str>());
                        auto sub_insn_name = sub_insn_params.front();
                        sub_insn_params.pop_front();

                        // Build the operand list.
                        utils::Any<Expression> sub_insn_operands;
                        for (const auto &sub_insn_param : sub_insn_params) {
                            ObjectLink target;
                            utils::Vec<utils::UInt> indices;
                            if (utils::starts_with(sub_insn_param, "%")) {
                                auto idx = utils::parse_uint(sub_insn_param.substr(1));
                                if (idx >= decomp->parameters.size()) {
                                    throw utils::Exception(
                                        "gate decomposition parameter " +
                                        sub_insn_param + " is out of range"
                                    );
                                }
                                target = decomp->parameters[idx];
                            } else if (utils::starts_with(sub_insn_param, "q")) {
                                auto idx = utils::parse_uint(sub_insn_param.substr(1));
                                if (idx >= get_num_qubits(ir)) {
                                    throw utils::Exception(
                                        "gate decomposition parameter " +
                                        sub_insn_param + " is out of range"
                                    );
                                }
                                target = ir->platform->qubits;
                                indices.push_back(idx);
                            } else {
                                throw utils::Exception(
                                    "unknown kind of gate decomposition parameter " +
                                    sub_insn_param + "; must be q<idx> or %<idx>"
                                );
                            }
                            sub_insn_operands.add(make_reference(ir, target, indices));
                        }

                        // Build the instruction.
                        auto sub_insn_node = make_instruction(
                            ir,
                            sub_insn_name,
                            sub_insn_operands,
                            {},
                            false,
                            true
                        );
                        decomp->expansion.add(sub_insn_node);

                        // Accumulate duration.
                        duration += get_duration_of_instruction(sub_insn_node);

                    }

                    // Set the duration if it still has its placeholder value.
                    if (insn_ref->duration == utils::UMAX) {
                        insn_ref->duration = duration;
                    }

                });

            } catch (utils::Exception &e) {
                e.add_context(
                    "in legacy decomposition of '" + it2.key() + "'"
                );
                throw;
            }
        }
    }

    // Now that we have all the instruction types, compute the decomposition
    // expansions that we postponed.
    for (const auto &fn : todo) {
        fn();
    }

    // Populate the default function types.
    auto fn = add_function_type(ir, utils::make<FunctionType>("operator!"));
    fn->operand_types.emplace(prim::OperandMode::READ, bit_type);
    fn->return_type = bit_type;
    for (const auto &op : utils::Vec<utils::Str>({"&&", "||", "==", "!="})) {
        fn = add_function_type(ir, utils::make<FunctionType>("operator" + op));
        fn->operand_types.emplace(prim::OperandMode::READ, bit_type);
        fn->operand_types.emplace(prim::OperandMode::READ, bit_type);
        fn->return_type = bit_type;
    }
    fn = add_function_type(ir, utils::make<FunctionType>("operator~"));
    fn->operand_types.emplace(prim::OperandMode::READ, int_type);
    fn->return_type = int_type;
    for (const auto &op : utils::Vec<utils::Str>({"+", "-", "&", "|", "^"})) {
        fn = add_function_type(ir, utils::make<FunctionType>("operator" + op));
        fn->operand_types.emplace(prim::OperandMode::READ, int_type);
        fn->operand_types.emplace(prim::OperandMode::READ, int_type);
        fn->return_type = int_type;
    }
    for (const auto &op : utils::Vec<utils::Str>({"==", "!=", "<", ">", "<=", ">="})) {
        fn = add_function_type(ir, utils::make<FunctionType>("operator" + op));
        fn->operand_types.emplace(prim::OperandMode::READ, int_type);
        fn->operand_types.emplace(prim::OperandMode::READ, int_type);
        fn->return_type = bit_type;
    }
    fn = add_function_type(ir, utils::make<FunctionType>("int"));
    fn->operand_types.emplace(prim::OperandMode::READ, bit_type);
    fn->return_type = int_type;

    // Populate topology. This is a bit annoying because the old platform has
    // it contained in an Opt rather than a Ptr.
    utils::Ptr<com::Topology> top;
    top.unwrap() = old->topology.unwrap();
    ir->platform->topology.populate(top.as_const());

    // Populate architecture.
    ir->platform->architecture.populate(old->architecture);

    // Populate resources.
    rmgr::CRef resources;
    resources.emplace(rmgr::Manager::from_defaults(old, {}, ir));
    ir->platform->resources.populate(resources);

    // Populate platform JSON data.
    ir->platform->data = old->platform_config;

    // Attach the old platform structure as an annotation. This is used when
    // converting back to the old IR structure.
    ir->platform->set_annotation<compat::PlatformRef>(old);

    // Check the result.
    QL_DOUT("Result of old->new IR platform conversion:");
    QL_IF_LOG_DEBUG(ir->dump_seq());
    check_consistency(ir);

    return ir;
}

/**
 * Converts a classical operand to an expression.
 */
static ExpressionRef convert_operand(
    const Ref &ir,
    const compat::ClassicalOperand &op
) {
    switch (op.type()) {
        case compat::ClassicalOperandType::VALUE:
            return make_int_lit(
                ir,
                op.as_value().value
            );

        case compat::ClassicalOperandType::REGISTER:
            return make_reference(
                ir,
                find_physical_object(ir, "creg"),
                {op.as_register().id}
            );

        default:
            throw utils::Exception("unknown operand type");

    }
}

/**
 * Converts the RHS of a classical gate to an expression.
 */
static ExpressionRef convert_classical(
    const Ref &ir,
    const utils::Str &name,
    const utils::Any<Expression> &operands,
    utils::Bool is_condition = false
) {
    // Construct the expression.
    ExpressionRef expr;
    if (name == "ldi" || name == "mov") {

        // Handle moves.
        if (operands.size() != 1) {
            throw utils::Exception(
                "incorrect number of arguments for ldi/mov instruction"
            );
        }
        expr = operands[0];

    } else {

        // Convert function name.
        static const utils::Map<utils::Str, utils::Str> OP_TO_FN{
            {"add", "operator+"},
            {"sub", "operator-"},
            {"and", "operator&"},
            {"or", "operator|"},
            {"xor", "operator^"},
            {"eq", "operator=="},
            {"ne", "operator!="},
            {"lt", "operator<"},
            {"gt", "operator>"},
            {"le", "operator<="},
            {"ge", "operator>="},
            {"not", "operator~"}
        };
        auto it = OP_TO_FN.find(name);
        if (it == OP_TO_FN.end()) {
            throw utils::Exception("unknown operation type '" + name + "'");
        }
        const auto &function_name = it->second;

        // Build the function call.
        expr = make_function_call(ir, function_name, operands);

    }

    // Check and if necessary convert the return type.
    if (is_condition) {
        QL_ASSERT(get_type_of(expr) == ir->platform->default_bit_type);
    } else {
        if (get_type_of(expr)->as_bit_type()) {
            expr = make_function_call(ir, "int", {expr});
        }
        QL_ASSERT(get_type_of(expr) == ir->platform->default_int_type);
    }

    return expr;
}

/**
 * Converts a classical operation to an expression.
 */
static ExpressionRef convert_operation(
    const Ref &ir,
    const compat::ClassicalOperation &op,
    utils::Bool is_condition = false,
    utils::Bool invert = false
) {

    // Figure out the operation name.
    auto name = op.operation_name;
    if (is_condition) {
        if (op.operation_type != compat::ClassicalOperationType::RELATIONAL) {
            throw utils::Exception(
                "attempt to use non-relational operation as condition"
            );
        }
        if (invert) {
            name = op.inv_operation_name;
        }
    }

    // Convert operands.
    utils::Any<Expression> expressions;
    for (const auto &operand : op.operands) {
        expressions.add(convert_operand(ir, *operand));
    }

    // Construct the expression.
    return convert_classical(ir, name, expressions, is_condition);

}

/**
 * Converts an old-IR gate to a new-IR instruction.
 */
static InstructionRef convert_gate(
    const Ref &ir,
    const compat::ProgramRef &old,
    const compat::GateRef &gate
) {

    // Gate names are a lie for RX90, MRX90, RX180, and Y variants of those
    // default gates, in that the name reported by the class differs from the
    // name with which the gate can be constructed. We have to convert that
    // here, or "information" is lost in the conversion process, meaning we
    // can't convert back to old one-to-one. For example, if the user adds
    // "ry90" and that gate doesn't exist in the platform, they'll get an RY90
    // class gate, which reports the name "y90". If we would just use that name
    // here and then convert back, they would instead get the "y90" from the
    // platform, because you can't even add a default gate by that name. Yes,
    // this actually happened in a test case. To handle the case correctly,
    // we'll use the name of the gate that the user would have to add (ry90) for
    // the real name, and use gate->name (y90) for the cQASM name, assuming
    // we'll need to add a new gate definition.
    auto name = parse_instruction_name(gate->name).front();
    auto cqasm_name = name;
    switch (gate->type()) {
        case compat::GateType::RX90:  name = "rx90";  break;
        case compat::GateType::MRX90: name = "mrx90"; break;
        case compat::GateType::RX180: name = "rx180"; break;
        case compat::GateType::RY90:  name = "ry90";  break;
        case compat::GateType::MRY90: name = "mry90"; break;
        case compat::GateType::RY180: name = "ry180"; break;
        default: void();
    }

    try {
        
        // Convert the gate's qubit operands.
        utils::Any<Expression> qubit_operands;
        for (auto idx : gate->operands) {
            qubit_operands.add(make_qubit_ref(ir, idx));
        }
        
        // Convert the gate's creg operands.
        utils::Any<Expression> creg_operands;
        if (!gate->creg_operands.empty()) {
            auto creg_object = find_physical_object(ir, "creg");
            QL_ASSERT(!creg_object.empty());
            for (auto idx : gate->creg_operands) {
                creg_operands.add(make_reference(ir, creg_object, {idx}));
            }
        }
        
        // Convert the gate's breg operands and condition. The first num_qubits
        // bits are mapped to the implicit bits associated with the qubits; only
        // beyond that is the b register used.
        utils::Any<Expression> breg_operands;
        ExpressionRef condition;
        auto breg_object = find_physical_object(ir, "breg");
        auto num_qubits = get_num_qubits(ir);

        // Convert breg operands.
        for (auto idx : gate->breg_operands) {
            if (idx < num_qubits) {
                breg_operands.add(make_bit_ref(ir, idx));
            } else {
                QL_ASSERT(!breg_object.empty());
                breg_operands.add(make_reference(ir, breg_object, {idx - num_qubits}));
            }
        }

        // Convert condition.
        utils::Any<Expression> cond_operands;
        for (auto idx : gate->cond_operands) {
            if (idx < num_qubits) {
                cond_operands.add(make_bit_ref(ir, idx));
            } else {
                QL_ASSERT(!breg_object.empty());
                cond_operands.add(make_reference(ir, breg_object, {idx - num_qubits}));
            }
        }
        switch (gate->condition) {
            case compat::ConditionType::ALWAYS:
                // Leave empty; condition will be inferred by
                // make_instruction(). Conversely, filling it out here would
                // throw an exception for instruction types that cannot be
                // made conditional.
                break;
            case compat::ConditionType::NEVER:
                condition = make_bit_lit(ir, false);
                break;
            case compat::ConditionType::UNARY:
                condition = cond_operands[0];
                break;
            case compat::ConditionType::NOT:
                condition = make_function_call(
                    ir, "operator!",
                    {cond_operands[0]}
                );
                break;
            case compat::ConditionType::AND:
                condition = make_function_call(
                    ir, "operator&&",
                    {cond_operands[0], cond_operands[1]}
                );
                break;
            case compat::ConditionType::NAND:
                condition = make_function_call(
                    ir, "operator!",
                    {make_function_call(
                        ir, "operator&&",
                        {cond_operands[0], cond_operands[1]}
                    )}
                );
                break;
            case compat::ConditionType::OR:
                condition = make_function_call(
                    ir, "operator||",
                    {cond_operands[0], cond_operands[1]}
                );
                break;
            case compat::ConditionType::NOR:
                condition = make_function_call(
                    ir, "operator!",
                    {make_function_call(
                        ir, "operator||",
                        {cond_operands[0], cond_operands[1]}
                    )}
                );
                break;
            case compat::ConditionType::XOR:
                condition = make_function_call(
                    ir, "operator!=",
                    {cond_operands[0], cond_operands[1]}
                );
                break;
            case compat::ConditionType::NXOR:
                condition = make_function_call(
                    ir, "operator==",
                    {cond_operands[0], cond_operands[1]}
                );
                break;
        }

        // Use a somewhat arbitrary set of rules to convert from the old gate
        // types to the new ones, considering that the semantics of gate->type()
        // and name were rather arbitrary to begin with. Start with special
        // instructions.
        if (name == "wait" || name == "barrier" || gate->type() == compat::GateType::WAIT) {
            auto duration = utils::div_ceil(gate->duration, old->platform->cycle_time);
            utils::Any<Expression> operands;
            operands.add(make_uint_lit(ir, duration));
            operands.extend(qubit_operands);
            operands.extend(creg_operands);
            operands.extend(breg_operands);
            return make_instruction(ir, "wait", operands, condition);
        }

        // Handle the classical gates from the remnants of CC-light.
        if (gate->type() == compat::GateType::CLASSICAL && (
            name == "mov" || name == "not" ||
            name == "and" || name == "or"  || name == "xor" ||
            name == "add" || name == "sub" ||
            name == "eq"  || name == "ne"  ||
            name == "lt"  || name == "gt"  ||
            name == "le"  || name == "ge"
        )) {
            auto lhs = creg_operands.front();
            creg_operands.remove(0);
            return make_set_instruction(
                ir,
                lhs,
                convert_classical(ir, name, creg_operands),
                condition
            );
        }
        if (gate->type() == compat::GateType::CLASSICAL && name == "ldi") {
            return make_set_instruction(
                ir,
                creg_operands[0],
                make_int_lit(ir, gate->int_operand),
                condition
            );
        }

        // See if we can find a custom instruction that matches closely enough.
        utils::Any<Expression> operands;
        operands.extend(qubit_operands);
        operands.extend(creg_operands);
        operands.extend(breg_operands);
        auto real_type = find_type(ir, "real");
        if (
            name == "rx" || name == "ry" || name == "rz" ||
            name == "crz" || name == "cr"
        ) {
            QL_ASSERT(!real_type.empty());
            operands.emplace<RealLiteral>(gate->angle, real_type);
        } else if (name == "crk") {
            operands.add(make_int_lit(ir, gate->int_operand));
        }
        auto insn = make_instruction(ir, name, operands, condition, true, true);
        if (!insn.empty()) {
            return insn;
        }

        // No instruction type exists yet... probably a default gate. So try to
        // infer an instruction type for it.
        operands.reset();
        auto ityp = utils::make<InstructionType>(name, cqasm_name);
        ityp->duration = utils::div_ceil(gate->duration, old->platform->cycle_time);
        auto qubit_type = ir->platform->qubits->data_type;
        switch (gate->type()) {
            case compat::GateType::IDENTITY:
            case compat::GateType::PREP_Z:
                for (const auto &qubit : qubit_operands) {
                    ityp->operand_types.emplace(prim::OperandMode::WRITE, qubit_type);
                    operands.add(qubit);
                }
                break;

            case compat::GateType::HADAMARD:
                ityp->operand_types.emplace(prim::OperandMode::WRITE, qubit_type);
                operands.add(qubit_operands[0]);
                break;

            case compat::GateType::PAULI_X:
            case compat::GateType::RX90:
            case compat::GateType::MRX90:
            case compat::GateType::RX180:
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_X, qubit_type);
                operands.add(qubit_operands[0]);
                break;

            case compat::GateType::PAULI_Y:
            case compat::GateType::RY90:
            case compat::GateType::MRY90:
            case compat::GateType::RY180:
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Y, qubit_type);
                operands.add(qubit_operands[0]);
                break;

            case compat::GateType::PAULI_Z:
            case compat::GateType::PHASE:
            case compat::GateType::PHASE_DAG:
            case compat::GateType::T:
            case compat::GateType::T_DAG:
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                operands.add(qubit_operands[0]);
                break;

            case compat::GateType::RX:
            case compat::GateType::RY:
            case compat::GateType::RZ:
                QL_ASSERT(!real_type.empty());
                if (gate->type() == compat::GateType::RX) {
                    ityp->operand_types.emplace(prim::OperandMode::COMMUTE_X, qubit_type);
                } else if (gate->type() == compat::GateType::RY) {
                    ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Y, qubit_type);
                } else {
                    ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                }
                operands.add(qubit_operands[0]);
                ityp->operand_types.emplace(prim::OperandMode::LITERAL, real_type);
                operands.emplace<RealLiteral>(gate->angle, real_type);
                break;

            case compat::GateType::CNOT:
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                operands.add(qubit_operands[0]);
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_X, qubit_type);
                operands.add(qubit_operands[1]);
                break;

            case compat::GateType::CPHASE:
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                operands.add(qubit_operands[0]);
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                operands.add(qubit_operands[1]);
                break;

            case compat::GateType::SWAP:
                ityp->operand_types.emplace(prim::OperandMode::WRITE, qubit_type);
                operands.add(qubit_operands[0]);
                ityp->operand_types.emplace(prim::OperandMode::WRITE, qubit_type);
                operands.add(qubit_operands[1]);
                break;

            case compat::GateType::TOFFOLI:
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                operands.add(qubit_operands[0]);
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_Z, qubit_type);
                operands.add(qubit_operands[1]);
                ityp->operand_types.emplace(prim::OperandMode::COMMUTE_X, qubit_type);
                operands.add(qubit_operands[2]);
                break;

            case compat::GateType::MEASURE:
            case compat::GateType::DISPLAY:
            case compat::GateType::DISPLAY_BINARY:
                for (const auto &qubit : qubit_operands) {
                    ityp->operand_types.emplace(prim::OperandMode::MEASURE, qubit_type);
                    operands.add(qubit);
                }
                break;

            case compat::GateType::NOP:
            case compat::GateType::CUSTOM:
            case compat::GateType::CLASSICAL:
                if (name != "nop") {
                    throw utils::Exception(
                        "unknown gate '" + name + "' with type " +
                        utils::to_string(gate->type())
                    );
                }
                break;

            case compat::GateType::COMPOSITE:
            case compat::GateType::DUMMY:
            case compat::GateType::WAIT:
                throw utils::Exception("unexpected gate type: " + utils::to_string(gate->type()));
        }

        // Add the inferred instruction type.
        add_instruction_type(ir, ityp);

        // Make an instruction with it. Note that this could be done more
        // efficiently: the instruction type doesn't really need to be resolved
        // since the function above already returns it. However, code reuse is
        // also worth something, and execution only gets here for the first gate
        // of a particular type.
        return make_instruction(ir, name, operands, condition);

    } catch (utils::Exception &e) {
        e.add_context("while converting gate " + name);
        throw;
    }
}

/**
 * Converts a block of old-IR kernels to a single new-IR subblock, starting the
 * scanning process from idx onward. idx is incremented to the index of the
 * next kernel.
 */
static utils::Str convert_kernels(
    const Ref &ir,
    const compat::ProgramRef &old,
    utils::UInt &idx,
    const utils::One<BlockBase> block
) {
    utils::Str name;
    try {

        // Figure out where we're at in terms of cycles in this block, and
        // how this compares to the cycle numbers in the old-IR kernel.
        utils::UInt cycle_offset = get_duration_of_block(block);
        utils::UInt cycle = cycle_offset;

        auto type = old->kernels[idx]->type;
        switch (type) {
            case compat::KernelType::STATIC: {

                // Convert gates to instructions.
                for (const auto &gate : old->kernels[idx]->gates) {

                    // Convert the gate.
                    auto instruction = convert_gate(ir, old, gate);

                    // Figure out its cycle number.
                    if (old->kernels[idx]->cycles_valid) {
                        if (gate->cycle != compat::MAX_CYCLE) {
                            auto gate_cycle = gate->cycle;
                            if (gate_cycle >= compat::FIRST_CYCLE) {
                                gate_cycle -= compat::FIRST_CYCLE;
                            } else {
                                gate_cycle = 0;
                            }
                            cycle = utils::max(cycle, gate_cycle + cycle_offset);
                        }
                        instruction->cycle = cycle;
                    } else {
                        instruction->cycle = cycle++;
                    }

                    // Add to block.
                    block->statements.add(instruction);

                }

                // Copy annotations from the kernel to the block, but make sure
                // that we don't override our own annotations!
                old->kernels[idx]->erase_annotation<KernelName>();
                old->kernels[idx]->erase_annotation<KernelCyclesValid>();
                block->copy_annotations(*old->kernels[idx]);

                // Save kernel name.
                name = old->kernels[idx]->name;
                if (!block->has_annotation<KernelName>()) {
                    block->set_annotation<KernelName>({name});
                }

                // Save cycle validity.
                if (!block->has_annotation<KernelCyclesValid>()) {
                    block->set_annotation<KernelCyclesValid>({
                        old->kernels[idx]->cycles_valid
                    });
                } else {
                    block->get_annotation<KernelCyclesValid>().valid &=
                        old->kernels[idx]->cycles_valid;
                }

                // Advance to the next kernel.
                idx++;
                break;
            }
            case compat::KernelType::FOR_START: {

                // Load the iteration count.
                auto iteration_count = old->kernels[idx]->iteration_count;

                // Seek past the start marker.
                idx++;

                // Handle the body by calling ourselves until we reach a FOR_END.
                auto sub_block = utils::make<SubBlock>();
                do {
                    auto new_name = convert_kernels(ir, old, idx, sub_block);
                    if (name.empty()) name = new_name;
                } while (old->kernels[idx]->type != compat::KernelType::FOR_END);

                // Skip past the FOR_END.
                idx++;

                // Create a static for loop and add it to the block.
                block->statements.emplace<StaticLoop>(
                    make_reference(ir, make_temporary(ir, ir->platform->default_int_type)),
                    make_uint_lit(ir, iteration_count - 1),
                    make_uint_lit(ir, (utils::UInt) 0),
                    sub_block,
                    cycle
                );

                break;
            }
            case compat::KernelType::DO_WHILE_START: {

                // Load the condition.
                const auto &condition = *old->kernels[idx]->br_condition;

                // Seek past the start marker.
                idx++;

                // Handle the body by calling ourselves until we reach a DO_WHILE_END.
                auto sub_block = utils::make<SubBlock>();
                do {
                    auto new_name = convert_kernels(ir, old, idx, sub_block);
                    if (name.empty()) name = new_name;
                } while (old->kernels[idx]->type != compat::KernelType::DO_WHILE_END);

                // Skip past the DO_WHILE_END.
                idx++;

                // Create a repeat-until loop and add it to the block. Since the
                // original is a do-while rather than a repeat-until, the condition
                // is inverted using `operator!(bit) -> bit`.
                block->statements.emplace<RepeatUntilLoop>(
                    convert_operation(ir, condition, true, true),
                    sub_block,
                    cycle
                );

                break;
            }
            case compat::KernelType::IF_START: {

                // Load the condition.
                const auto &condition = *old->kernels[idx]->br_condition;

                // Seek past the start marker.
                idx++;

                // Handle the body by calling ourselves until we reach an IF_END.
                auto if_block = utils::make<SubBlock>();
                do {
                    auto new_name = convert_kernels(ir, old, idx, if_block);
                    if (name.empty()) name = new_name;
                } while (old->kernels[idx]->type != compat::KernelType::IF_END);

                // Skip past the IF_END.
                idx++;

                // Handle the else block, if any.
                utils::One<SubBlock> else_block;
                if (idx < old->kernels.size() && old->kernels[idx]->type == compat::KernelType::ELSE_START) {

                    // Seek past the start marker.
                    idx++;

                    // Handle the body by calling ourselves until we reach an
                    // ELSE_END.
                    else_block.emplace();
                    do {
                        auto new_name = convert_kernels(ir, old, idx, else_block);
                        if (name.empty()) name = new_name;
                    } while (old->kernels[idx]->type != compat::KernelType::ELSE_END);

                    // Skip past the ELSE_END.
                    idx++;

                }

                // Create an if-else statement and add it to the block.
                block->statements.emplace<IfElse>(
                    utils::Many<IfElseBranch>({utils::make<IfElseBranch>(
                        convert_operation(ir, condition, true),
                        if_block
                    )}),
                    else_block,
                    cycle
                );

                break;
            }
            default:
                throw utils::Exception(
                    "unexpected kernel type for kernel with index " +
                    utils::to_string(idx)
                );

        }
    } catch (utils::Exception &e) {
        if (idx < old->kernels.size()) {
            e.add_context(
                "while converting kernel " + utils::to_string(idx) +
                " ('" + old->kernels[idx]->name + "')"
            );
        } else {
            e.add_context(
                "after converting last kernel (corrupt control-flow structure?)"
            );
        }
        throw;
    }
    return name;
}

/**
 * Converts the old IR (program and platform) to the new one.
 *
 * Refer to the header file for details.
 */
Ref convert_old_to_new(const compat::ProgramRef &old) {

    // Build the platform.
    auto ir = convert_old_to_new(old->platform);

    // If there are no kernels in the old program, don't create a program node
    // at all.
    if (old->kernels.empty()) {
        return ir;
    }

    // Build a program node and copy the metadata.
    ir->program.emplace();
    ir->program->name = old->name;
    ir->program->unique_name = old->unique_name;
    ir->program->copy_annotations(*old);
    ir->program->set_annotation<ObjectUsage>({
        old->qubit_count,
        old->creg_count,
        old->breg_count
    });

    // Convert the kernels.
    utils::Set<utils::Str> names;
    for (utils::UInt idx = 0; idx < old->kernels.size(); ) {

        // Convert the next block of kernels.
        auto block = utils::make<Block>();
        auto name = convert_kernels(ir, old, idx, block);

        // Sanitize and uniquify the kernel name.
        name = std::regex_replace(name, std::regex("[^a-zA-Z0-9_]"), "_");
        if (!std::regex_match(name, IDENTIFIER_RE)) name = "_" + name;
        auto unique_name = name;
        utils::UInt unique_idx = 1;
        while (!names.insert(unique_name).second) {
            unique_name = name + "_" + utils::to_string(unique_idx++);
        }
        QL_ASSERT(std::regex_match(unique_name, IDENTIFIER_RE));
        block->name = unique_name;

        // Link the previous block to this one.
        if (ir->program->blocks.empty()) {
            ir->program->entry_point = block;
        } else {
            ir->program->blocks.back()->next = block;
        }

        // Add the block.
        ir->program->blocks.add(block);

    }

    // Check the result.
    QL_DOUT("Result of old->new IR program conversion:");
    QL_IF_LOG_DEBUG(ir->dump_seq());
    check_consistency(ir);

    return ir;
}

} // namespace ir
} // namespace ql
