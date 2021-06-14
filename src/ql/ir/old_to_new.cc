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
        auto obj = find_physical_object(ir, param.substr(0, 1));
        if (obj.empty()) {
            throw utils::Exception(
                "invalid specialization parameter \"" + param + "\": "
                "no register exists with that name"
            );
        }
        auto index = utils::parse_uint(param.substr(1));
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
        return make_reference(ir, obj, {index});
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
 * Converts the old IR (program and platform) to the new one.
 */
Ref convert_old_to_new(const compat::ProgramRef &old) {
    Ref ir;
    ir.emplace();

    // Build the platform.
    ir->platform.emplace();
    ir->platform->name = old->platform->name;

    // Add qubit type and main qubit register.
    auto qubit_type = add_type<QubitType>(ir, "qubit");
    ir->platform->qubits = add_physical_object(ir, utils::make<PhysicalObject>(
        "q", qubit_type, prim::UIntVec({old->platform->qubit_count})
    ));

    // Add type for bregs and conditions.
    auto bit_type = add_type<BitType>(ir, "bit");
    ObjectLink bregs;
    if (old->platform->breg_count) {
        bregs = add_physical_object(ir, utils::make<PhysicalObject>(
            "b", bit_type, prim::UIntVec({old->platform->breg_count})
        ));
    }
    ir->platform->implicit_bit_type = bit_type;
    ir->platform->default_bit_type = bit_type;

    // Add type for cregs.
    auto int_type = add_type<IntType>(ir, "int", true, 32);
    ObjectLink cregs;
    if (old->platform->creg_count) {
        cregs = add_physical_object(ir, utils::make<PhysicalObject>(
            "c", int_type, prim::UIntVec({old->platform->creg_count})
        ));
    }
    ir->platform->default_int_type = int_type;

    // Add type for angle operands.
    auto real_type = add_type<RealType>(ir, "real");

    // Add the instruction set. We load this from the JSON data rather than
    // trying to use instruction_map, because the latter has some pretty ****ed
    // up stuff going on in it to make the legacy decompositions work.
    for (
        auto it = old->platform->get_instructions().begin();
        it != old->platform->get_instructions().end();
        ++it
    ) {
        try {

            // Create an instruction node for the incoming instruction.
            auto insn = utils::make<InstructionType>();
            insn->data = *it;

            // The instruction name is in it.key(). However:
            //  - this may include specialization parameters; and
            //  - someone thought it'd be a good idea to let people write absolute
            //    garbage and then "sanitize" with some regexes. Ugh.
            auto template_params = parse_instruction_name(it.key());
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
                    prim::AccessMode mode;
                    if (mode_s == "W") {
                        mode = prim::AccessMode::WRITE;
                    } else if (mode_s == "R") {
                        mode = prim::AccessMode::READ;
                        if (type->as_qubit_type()) {
                            throw utils::Exception(
                                "invalid parameter mode R for qubit type"
                            );
                        }
                    } else if (mode_s == "L") {
                        mode = prim::AccessMode::LITERAL;
                        if (type->as_qubit_type()) {
                            throw utils::Exception(
                                "invalid parameter mode L for qubit type"
                            );
                        }
                    } else if (mode_s == "X") {
                        mode = prim::AccessMode::COMMUTE_X;
                        if (type->as_classical_type()) {
                            throw utils::Exception(
                                "invalid parameter mode X for classical type"
                            );
                        }
                    } else if (mode_s == "Y") {
                        mode = prim::AccessMode::COMMUTE_Y;
                        if (type->as_classical_type()) {
                            throw utils::Exception(
                                "invalid parameter mode Y for classical type"
                            );
                        }
                    } else if (mode_s == "Z") {
                        mode = prim::AccessMode::COMMUTE_Z;
                        if (type->as_classical_type()) {
                            throw utils::Exception(
                                "invalid parameter mode Z for classical type"
                            );
                        }
                    } else if (mode_s == "M") {
                        mode = prim::AccessMode::MEASURE;
                        if (type->as_classical_type()) {
                            throw utils::Exception(
                                "invalid parameter mode M for classical type"
                            );
                        }
                    } else {
                        throw utils::Exception(
                            "invalid parameter mode " + mode_s + ": "
                            "must be W, R, L, X, Y, or Z"
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
                    insn->operand_types.emplace(prim::AccessMode::WRITE, qubit_type);

                } else if (insn->name == "rx") {

                    // Single-qubit X rotation gate.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_X, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::LITERAL, real_type);

                } else if (std::regex_match(insn->name, std::regex("(m|mr|r)?xm?[0-9]*"))) {

                    // Single-qubit gate that commutes on the X axis.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_X, qubit_type);

                } else if (insn->name == "ry") {

                    // Single-qubit Y rotation gate.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Y, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::LITERAL, real_type);

                } else if (std::regex_match(insn->name, std::regex("(m|mr|r)?ym?[0-9]*"))) {

                    // Single-qubit gate that commutes on the Y axis.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Y, qubit_type);

                } else if (insn->name == "rz") {

                    // Single-qubit Z rotation gate.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::LITERAL, real_type);

                } else if (insn->name == "crz" || insn->name == "cr") {

                    // Controlled Z rotation gate.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::LITERAL, real_type);

                } else if (insn->name == "crk") {

                    // Controlled Z rotation gate.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::LITERAL, int_type);

                } else if (std::regex_match(insn->name, std::regex("[st](dag)?|(m|mr|r)?zm?[0-9]*"))) {

                    // Single-qubit gate that commutes on the Z axis.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);

                } else if (std::regex_match(insn->name, std::regex("meas(ure)?(_?[xyz])?(_keep)?"))) {

                    // Measurements.
                    duplicate_with_breg_arg = true;
                    insn->operand_types.emplace(prim::AccessMode::MEASURE, qubit_type);

                } else if (std::regex_match(insn->name, std::regex("(teleport)?(move|swap)"))) {

                    // Swaps.
                    insn->operand_types.emplace(prim::AccessMode::WRITE, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::WRITE, qubit_type);

                } else if (insn->name == "cnot" || insn->name == "cx") {

                    // Controlled X.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_X, qubit_type);

                } else if (insn->name == "cz" || insn->name == "cphase") {

                    // Controlled phase.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);

                } else if (insn->name == "cz_park") {

                    // Parking cz (assume only one parked qubit at the end).
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::WRITE, qubit_type);

                } else if (insn->name == "toffoli") {

                    // Toffoli gate.
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_Z, qubit_type);
                    insn->operand_types.emplace(prim::AccessMode::COMMUTE_X, qubit_type);

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
                            prim::AccessMode::WRITE,
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
                duration_divider = old->platform->cycle_time;
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
                insn->operand_types[0]->mode = prim::AccessMode::WRITE;
                insn->operand_types.emplace(prim::AccessMode::WRITE, bit_type);
                add_instruction_type(ir, insn, template_operands);
            }

        } catch (utils::Exception &e) {
            e.messages.push_front("in gate description for '" + it.key() + "'");
            throw;
        }
    }

    // Add legacy decompositions to the new system, for gates added by passes
    // (notably swap and relatives for the mapper).
    auto it = old->platform->platform_config.find("gate_decomposition");
    if (it != old->platform->platform_config.end()) {
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
                        prim::AccessMode::WRITE,
                        get_type_of(template_operand)
                    );
                }

                // Legacy decompositions only support qubit operands, and we
                // know exactly how many. So this is way easier than it was for
                // normal instructions.
                for (utils::UInt i = 0; i < parameter_count; i++) {
                    insn->operand_types.emplace(prim::AccessMode::WRITE, qubit_type);
                    decomp->parameters.emplace("", qubit_type, prim::UIntVec());
                }

                // Figure out the instructions that the to-be-decomposed
                // instruction maps to.
                auto sub_insns = it2.value();
                if (!sub_insns.is_array()) {
                    throw utils::Exception("decomposition must be an array");
                }

                // Make a trivial guess for the duration by just summing the
                // durations of the sub-instructions, which we'll do while
                // resolving them in the loop below. This is not a very
                // realistic mode, of course, since sub-instructions may end up
                // being executed in parallel.
                insn->duration = 0;

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
                            if (idx >= ir->platform->qubits->shape[0]) {
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
                    auto sub_insn_node = build_instruction(
                        ir,
                        sub_insn_name,
                        sub_insn_operands,
                        {},
                        true
                    );
                    decomp->expansion.add(sub_insn_node);

                    // Accumulate duration.
                    insn->duration += get_duration_of(sub_insn_node);

                }

                // Now actually add the decomposition rule.
                add_decomposition_rule(ir, insn, template_operands);

            } catch (utils::Exception &e) {
                e.messages.push_front(
                    "in legacy decomposition of '" + it2.key() + "'"
                );
                throw;
            }
        }
    }

    // Populate topology. This is a bit annoying because the old platform has
    // it contained in an Opt rather than a Ptr.
    utils::Ptr<com::Topology> top;
    top.unwrap() = old->platform->topology.unwrap();
    ir->platform->topology.populate(top.as_const());

    // Populate architecture.
    ir->platform->architecture.populate(old->platform->architecture);

    // Populate resources.
    rmgr::CRef resources;
    resources.emplace(rmgr::Manager::from_defaults(old->platform));
    ir->platform->resources.populate(resources);

    // Populate platform JSON data.
    ir->platform->data = old->platform->platform_config;

    //ir->dump_seq();
    check_consistency(ir);
    return ir;
}

} // namespace ir
} // namespace ql
