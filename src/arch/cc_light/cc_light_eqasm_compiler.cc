/** \file
 * CC-light eQASM compiler implementation.
 */

#include "cc_light_eqasm_compiler.h"

#include "scheduler.h"
#include "mapper.h"
#include "clifford.h"
#include "latency_compensation.h"
#include "buffer_insertion.h"
#include "qsoverlay.h"
#include "utils/filesystem.h"

namespace ql {
namespace arch {

using namespace utils;

UInt CurrSRegCount;
UInt CurrTRegCount;

Mask::Mask(const qubit_set_t &qs) : squbits(qs) {
    if (CurrSRegCount < MAX_S_REG) {
        regNo = CurrSRegCount++;
        regName = "s" + to_string(regNo);
    } else {
        QL_COUT(" !!!! Handle cases requiring more registers");
    }
}

Mask::Mask(const Str &rn, const qubit_set_t &qs) : regName(rn), squbits(qs) {
    if (CurrSRegCount < MAX_S_REG) {
        regNo = CurrSRegCount++;
    } else {
        QL_COUT(" !!!! Handle cases requiring more registers");
    }
}

Mask::Mask(const qubit_pair_set_t &qps) : dqubits(qps) {
    if (CurrTRegCount < MAX_T_REG) {
        regNo = CurrTRegCount++;
        regName = "t" + to_string(regNo);
    } else {
        QL_COUT(" !!!! Handle cases requiring more registers");
    }
}

MaskManager::MaskManager() {
    // add pre-defined smis
    for (UInt i = 0; i < 7; ++i) {
        qubit_set_t qs;
        qs.push_back(i);
        Mask m(qs);
        QS2Mask.set(qs) = m;
        SReg2Mask.set(m.regNo) = m;
    }

    // add some common single qubit masks
    {
        qubit_set_t qs;
        for(auto i=0; i<7; i++) qs.push_back(i);
        Mask m(qs); // TODO add proper support for:  Mask m(qs, "all_qubits");
        QS2Mask.set(qs) = m;
        SReg2Mask.set(m.regNo) = m;
    }

    {
        qubit_set_t qs;
        qs.push_back(0); qs.push_back(1); qs.push_back(5); qs.push_back(6);
        Mask m(qs); // TODO add proper support for:  Mask m(qs, "data_qubits");
        QS2Mask.set(qs) = m;
        SReg2Mask.set(m.regNo) = m;
    }

    {
        qubit_set_t qs;
        qs.push_back(2); qs.push_back(3); qs.push_back(4);
        Mask m(qs); // TODO add proper support for:  Mask m(qs, "ancilla_qubits");
        QS2Mask.set(qs) = m;
        SReg2Mask.set(m.regNo) = m;
    }

    // qubit_pair_set_t pre_defined_edges = { {2,0}, {0,3}, {3,1}, {1,4}, {2,5}, {5,3}, {3,6}, {6,4},
    //                                {0,2}, {3,0}, {1,3}, {4,1}, {5,2}, {3,5}, {6,3}, {4,6} };
    // // add smit
    // for(auto & p : pre_defined_edges)
    // {
    //     qubit_pair_set_t qps;
    //     qps.push_back(p);
    //     Mask m(qps);
    //     QPS2Mask[qps] = m;
    //     TReg2Mask[m.regNo] = m;
    // }

}

MaskManager::~MaskManager() {
    CurrSRegCount=0;
    CurrTRegCount=0;
}

UInt MaskManager::getRegNo(qubit_set_t &qs) {
    // sort qubit operands to avoid variation in order
    sort(qs.begin(), qs.end());

    auto it = QS2Mask.find(qs);
    if (it == QS2Mask.end()) {
        Mask m(qs);
        QS2Mask.set(qs) = m;
        SReg2Mask.set(m.regNo) = m;
    }
    return QS2Mask.at(qs).regNo;
}

UInt MaskManager::getRegNo(qubit_pair_set_t &qps) {
    // sort qubit operands pair to avoid variation in order
    sort(qps.begin(), qps.end());

    auto it = QPS2Mask.find(qps);
    if (it == QPS2Mask.end()) {
        Mask m(qps);
        QPS2Mask.set(qps) = m;
        TReg2Mask.set(m.regNo) = m;
    }
    return QPS2Mask.at(qps).regNo;
}

Str MaskManager::getRegName(qubit_set_t &qs) {
    // sort qubit operands to avoid variation in order
    sort(qs.begin(), qs.end());

    auto it = QS2Mask.find(qs);
    if (it == QS2Mask.end()) {
        Mask m(qs);
        QS2Mask.set(qs) = m;
        SReg2Mask.set(m.regNo) = m;
    }
    return QS2Mask.at(qs).regName;
}

Str MaskManager::getRegName(qubit_pair_set_t &qps) {
    // sort qubit operands pair to avoid variation in order
    sort(qps.begin(), qps.end());

    auto it = QPS2Mask.find(qps);
    if (it == QPS2Mask.end()) {
        Mask m(qps);
        QPS2Mask.set(qps) = m;
        TReg2Mask.set(m.regNo) = m;
    }
    return QPS2Mask.at(qps).regName;
}

Str MaskManager::getMaskInstructions() {
    StrStrm ssmasks;
    for (UInt r = 0; r < CurrSRegCount; ++r) {
        auto &m = SReg2Mask.at(r);
        ssmasks << "smis " << m.regName << ", {";
        for (auto it = m.squbits.begin(); it != m.squbits.end(); ++it) {
            ssmasks << *it;
            if (std::next(it) != m.squbits.end()) {
                ssmasks << ", ";
            }
        }
        ssmasks << "} " << std::endl;
    }

    for (UInt r = 0; r < CurrTRegCount; ++r) {
        auto &m = TReg2Mask.at(r);
        ssmasks << "smit " << m.regName << ", {";
        for (auto it = m.dqubits.begin(); it != m.dqubits.end(); ++it) {
            ssmasks << "(" << it->first << ", " << it->second << ")";
            if (std::next(it) != m.dqubits.end()) {
                ssmasks << ", ";
            }
        }
        ssmasks << "} " << std::endl;
    }

    return ssmasks.str();
}

classical_cc::classical_cc(
    const Str &operation,
    const Vec<UInt> &opers,
    Int ivalue
) {
    QL_DOUT("Classical_cc constructor for operation " << operation);
    QL_DOUT("... operands:");
    for (auto o : opers) QL_DOUT ("...... " << o << " ");
    QL_DOUT("...... ivalue= " << ivalue);

    // DOUT("adding classical_cc " << operation);
    name = to_lower(operation);
    duration = 20;
    creg_operands=opers;
    Int sz = creg_operands.size();
    if (
        (
            (name == "add") || (name == "sub")
            || (name == "and") || (name == "or") || (name == "xor")
        ) && (sz == 3)
        ) {
        QL_DOUT("Adding 3 operand operation: " << name);
    } else if (((name == "not") || (name == "cmp")) && (sz == 2)) {
        QL_DOUT("Adding 2 operand operation: " << name);
    } else if ((name == "fmr")  && (sz == 2)) {
        creg_operands= { opers[0] };
        operands= { opers[1] };
        QL_DOUT("Adding 2 operand fmr operation: " << name);
    } else if (
        (
            (name == "ldi") ||
            (name == "fbr_eq") || (name == "fbr_ne") || (name == "fbr_lt") ||
            (name == "fbr_gt") || (name == "fbr_le") || (name == "fbr_ge")
        ) && (sz == 1)
        ) {
        if (name == "ldi") {
            QL_DOUT("... setting int_operand of classical_cc gate for operation " << name << " to " << ivalue);
            int_operand = ivalue;
        }
        QL_DOUT("Adding 1 operand operation: " << name);
    } else if ((name == "nop") && (sz == 0)) {
        QL_DOUT("Adding 0 operand operation: " << name);
    } else {
        QL_EOUT("Unknown cclight classical operation '" << name << "' with '" << sz << "' operands!");
        throw Exception("Unknown cclight classical operation'" + name + "' with'" + to_string(sz) + "' operands!", false);
    }
    QL_DOUT("adding classical_cc [DONE]");
}

instruction_t classical_cc::qasm() const {
    Str iopers;
    Int sz = creg_operands.size();
    for (Int i = 0; i < sz; ++i) {
        if (i == sz - 1) {
            iopers += " r" + to_string(creg_operands[i]);
        } else {
            iopers += " r" + to_string(creg_operands[i]) + ",";
        }
    }

    if (name == "ldi") {
        iopers += ", " + to_string(int_operand);
        return "ldi" + iopers;
    } else if (name == "fmr") {
        return name + " r" + to_string(creg_operands[0]) +
               ", q" + to_string(operands[0]);
    } else {
        return name + iopers;
    }
}


gate_type_t classical_cc::type() const {
    return __classical_gate__;
}

cmat_t classical_cc::mat() const {
    return m;
}

Str classical_instruction2qisa(classical_cc *classical_ins) {
    StrStrm ssclassical;
    auto &iname = classical_ins->name;
    auto &iopers = classical_ins->creg_operands;
    Int iopers_count = iopers.size();

    if (
        (iname == "add") || (iname == "sub") ||
        (iname == "and") || (iname == "or") || (iname == "not") ||
        (iname == "xor") ||
        (iname == "ldi") || (iname == "nop") || (iname == "cmp")
    ) {
        ssclassical << iname;
        for (Int i = 0; i < iopers_count; ++i) {
            if (i == iopers_count - 1) {
                ssclassical << " r" << iopers[i];
            } else {
                ssclassical << " r" << iopers[i] << ",";
            }
        }
        if (iname == "ldi") {
            ssclassical << ", " + to_string(classical_ins->int_operand);
        }
    } else if (iname == "fmr") {
        ssclassical << "fmr r" << iopers[0] << ", q" << classical_ins->operands[0];
    } else if (iname == "fbr_eq") {
        ssclassical << "fbr " << "EQ, r" << iopers[0];
    } else if (iname == "fbr_ne") {
        ssclassical << "fbr " << "NE, r" << iopers[0];
    } else if (iname == "fbr_lt") {
        ssclassical << "fbr " << "LT, r" << iopers[0];
    } else if (iname == "fbr_gt") {
        ssclassical << "fbr " << "GT, r" << iopers[0];
    } else if (iname == "fbr_le") {
        ssclassical << "fbr " << "LE, r" << iopers[0];
    } else if (iname == "fbr_ge") {
        ssclassical << "fbr " << "GE, r" << iopers[0];
    } else {
        QL_EOUT("Unknown CClight classical operation '" << iname << "' with '" << iopers_count << "' operands!");
        throw Exception("Unknown classical operation'" + iname + "' with'" + to_string(iopers_count) + "' operands!", false);
    }

    return ssclassical.str();
}

// FIXME HvS cc_light_instr is name of attribute in json file, in gate: arch_operation_name, here in instruction_map?
// FIXME HvS attribute of gate or just in json? Generalization to arch_operation_name is unnecessary
Str get_cc_light_instruction_name(
    const Str &id,
    const quantum_platform &platform
) {
    Str cc_light_instr_name;
    auto it = platform.instruction_map.find(id);
    if (it != platform.instruction_map.end()) {
        custom_gate* g = it->second;
        cc_light_instr_name = g->arch_operation_name;
        if (cc_light_instr_name.empty()) {
            QL_FATAL("cc_light_instr not defined for instruction: " << id << " !");
        }
        // DOUT("cc_light_instr name: " << cc_light_instr_name);
    } else {
        QL_FATAL("custom instruction not found for : " << id << " !");
    }
    return cc_light_instr_name;
}

Str ir2qisa(
    quantum_kernel &kernel,
    const quantum_platform &platform,
    MaskManager &gMaskManager
) {
    QL_IOUT("Generating CC-Light QISA");

    ir::bundles_t bundles1;
    QL_ASSERT(kernel.cycles_valid);
    bundles1 = ir::bundler(kernel.c, platform.cycle_time);

    QL_IOUT("Combining parallel sections...");
    // combine parallel instructions of same type from different sections into a single section
    // this prepares for SIMD; each section will be a SIMD; of a quantum SIMD all operands are combined in a mask
    ir::DebugBundles("Before combining parallel sections", bundles1);
    for (ir::bundle_t &abundle : bundles1) {
        auto secIt1 = abundle.parallel_sections.begin();

        for (; secIt1 != abundle.parallel_sections.end(); ++secIt1) {
            for (auto secIt2 = std::next(secIt1); secIt2 != abundle.parallel_sections.end(); ++secIt2) {
                auto insIt1 = secIt1->begin();
                auto insIt2 = secIt2->begin();
                if (insIt1 != secIt1->end() && insIt2 != secIt2->end()) {
                    auto id1 = (*insIt1)->name;
                    auto id2 = (*insIt2)->name;
                    auto itype1 = (*insIt1)->type();
                    auto itype2 = (*insIt2)->type();
                    if (itype1 == __classical_gate__ || itype2 == __classical_gate__) {
                        QL_DOUT("Not splicing " << id1 << " and " << id2);
                        continue;
                    }

                    auto n1 = get_cc_light_instruction_name(id1, platform);
                    auto n2 = get_cc_light_instruction_name(id2, platform);
                    if (n1 == n2) {
                        QL_DOUT("Splicing " << id1 << "/" << n1 << " and " << id2 << "/" << n2);
                        (*secIt1).splice(insIt1, (*secIt2));
                    } else {
                        QL_DOUT("Not splicing " << id1 << "/" << n1 << " and " << id2 << "/" << n2);
                    }
                }
            }
        }
    }
    ir::DebugBundles("After combining", bundles1);
    QL_IOUT("Removing empty sections...");
    // remove empty sections
    ir::bundles_t bundles2;
    for (ir::bundle_t &abundle1 : bundles1) {
        ir::bundle_t abundle2;
        abundle2.start_cycle = abundle1.start_cycle;
        abundle2.duration_in_cycles = abundle1.duration_in_cycles;
        for (auto &sec : abundle1.parallel_sections) {
            if (!sec.empty()) {
                abundle2.parallel_sections.push_back(sec);
            }
        }
        bundles2.push_back(abundle2);
    }
    bundles1.clear();
    ir::DebugBundles("After removing empty sections", bundles2);

    // sort sections to get consistent output across multiple runs. The output
    // is correct even without this sorting. Sorting is important to test the similarity
    // of generated qisa against golden qisa files. For example, without sorting
    // any of the following can be generated, which is correct but there will be
    // differences reported by file_compare used for testing:
    // x s0 | y s1
    // OR
    // y s1 | x s0
    // However, with sorting it will always generate:
    // x s0 | y s1
    //
    QL_IOUT("Sorting sections alphabetically according to instruction name ...");
    for (ir::bundle_t &abundle : bundles2) {
        // sorts instructions alphabetically
        abundle.parallel_sections.sort(
            [](const ir::section_t &sec1, const ir::section_t &sec2) -> Bool {
                auto i1 = sec1.begin();
                auto iname1 = (*(i1))->name;
                auto i2 = sec2.begin();
                auto iname2 = (*(i2))->name;
                return iname2 < iname1;
            }
        );
    }

    // And now generate qisa
    // each section of a bundle will become a SIMD (all operations in a section are the same, see above)
    // for the operands of the SIMD, a mask will be used
    //
    // kernel prologue (start label) and epilogue are generated by the caller or ir2qisa
    StrStrm ssqisa;   // output qisa in here
    UInt curr_cycle = 0; // first instruction should be with pre-interval 1, 'bs 1' FIXME HvS start in cycle 0
    for (ir::bundle_t &abundle : bundles2) {
        Str iname;
        StrStrm sspre, ssinst;
        auto bcycle = abundle.start_cycle;
        auto delta = bcycle - curr_cycle;
        Bool classical_bundle=false;
        if (delta < 8) {
            sspre << "    " << delta << "    ";
        } else {
            sspre << "    qwait " << delta - 1 << std::endl
                  << "    1    ";
        }

        for (auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt) {
            qubit_set_t squbits;
            qubit_pair_set_t dqubits;
            auto firstInsIt = secIt->begin();
            iname = (*(firstInsIt))->name;
            auto itype = (*(firstInsIt))->type();

            if (itype == __classical_gate__) {
                classical_bundle = true;
                ssinst << classical_instruction2qisa( (classical_cc *)(*firstInsIt) );
            } else {
                QL_DOUT("get cclight instr name for : " << iname);
                Str cc_light_instr_name = get_cc_light_instruction_name(iname, platform);
                auto nOperands = ((*firstInsIt)->operands).size();
                if (itype == __nop_gate__) {
                    ssinst << cc_light_instr_name;
                } else {
                    for (auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt) {
                        if (nOperands == 1) {
                            auto &op = (*insIt)->operands[0];
                            squbits.push_back(op);
                        } else if (nOperands == 2) {
                            auto &op1 = (*insIt)->operands[0];
                            auto &op2 = (*insIt)->operands[1];
                            dqubits.push_back(qubit_pair_t(op1, op2));
                        } else {
                            throw Exception("Error : only 1 and 2 operand instructions are supported by cc light masks !", false);
                        }
                    }

                    Str rname;
                    if (nOperands == 1) {
                        rname = gMaskManager.getRegName(squbits);
                    } else if (nOperands == 2) {
                        rname = gMaskManager.getRegName(dqubits);
                    } else {
                        throw Exception("Error : only 1 and 2 operand instructions are supported by cc light masks !", false);
                    }

                    ssinst << cc_light_instr_name << " " << rname;
                }
            }

            if (std::next(secIt) != abundle.parallel_sections.end()) {
                ssinst << " | ";
            }
        }
        if (classical_bundle) {
            if (iname == "fmr") {
                // based on cclight requirements (section 4.7 eqasm manual),
                // two extra instructions need to be added between meas and fmr
                if (delta > 2) {
                    ssqisa << "    qwait " << 1 << std::endl;
                    ssqisa << "    qwait " << delta-1 << std::endl;
                } else {
                    ssqisa << "    qwait " << 1 << std::endl;
                    ssqisa << "    qwait " << 1 << std::endl;
                }
            } else {
                if (delta > 1) {
                    ssqisa << "    qwait " << delta << std::endl;
                }
            }
            ssqisa << "    " << ssinst.str() << std::endl;
        } else {
            // FIXME HvS next addition could be an option, assuming # comment convention of qisa
            // ssqisa << sspre.str() << ssinst.str() << "\t\t# @" << bcycle << std::endl;
            ssqisa << sspre.str() << ssinst.str() << std::endl;
        }
        curr_cycle+=delta;
    }

    auto & lastBundle = bundles2.back();
    Int lbduration = lastBundle.duration_in_cycles;
    if (lbduration > 1) {
        ssqisa << "    qwait " << lbduration << std::endl;
    }

    QL_IOUT("Generating CC-Light QISA [Done]");
    return ssqisa.str();
}

Str cc_light_eqasm_compiler::get_qisa_prologue(const quantum_kernel &k) {
    StrStrm ss;

    if (k.type == kernel_type_t::IF_START) {
#if 0
        // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition->inv_operation_name
               <<" r" << (k.br_condition->operands[0])->id <<", r" << (k.br_condition->operands[1])->id
               << ", " << k.name << "_end" << std::endl;
#else
        ss  <<"    cmp r" << (k.br_condition->operands[0])->as_creg().id
            <<", r" << (k.br_condition->operands[1])->as_creg().id << std::endl;
        ss  <<"    nop" << std::endl;
        ss  <<"    br " << k.br_condition->inv_operation_name << ", "
            << k.name << "_end" << std::endl;
#endif

    }

    if (k.type == kernel_type_t::ELSE_START) {
#if 0
        // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition->operation_name <<" r" << (k.br_condition->operands[0])->id
               <<", r" << (k.br_condition->operands[1])->id << ", " << k.name << "_end" << std::endl;
#else
        ss  <<"    cmp r" << (k.br_condition->operands[0])->as_creg().id
            <<", r" << (k.br_condition->operands[1])->as_creg().id << std::endl;
        ss  <<"    nop" << std::endl;
        ss  <<"    br " << k.br_condition->operation_name << ", "
            << k.name << "_end" << std::endl;
#endif
    }

    if (k.type == kernel_type_t::FOR_START) {
        // for now r29, r30, r31 are used as temporaries
        ss << "    ldi r29" <<", " << k.iterations << std::endl;
        ss << "    ldi r30" <<", " << 1 << std::endl;
        ss << "    ldi r31" <<", " << 0 << std::endl;
    }

    return ss.str();
}

Str cc_light_eqasm_compiler::get_qisa_epilogue(const quantum_kernel &k) {
    StrStrm ss;

    if (k.type == kernel_type_t::DO_WHILE_END) {
#if 0
        // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition->operation_name <<" r" << (k.br_condition->operands[0])->id
               <<", r" << (k.br_condition->operands[1])->id << ", " << k.name << "_start" << std::endl;
#else
        ss  <<"    cmp r" << (k.br_condition->operands[0])->as_creg().id
            <<", r" << (k.br_condition->operands[1])->as_creg().id << std::endl;
        ss  <<"    nop" << std::endl;
        ss  <<"    br " << k.br_condition->operation_name << ", "
            << k.name << "_start" << std::endl;
#endif
    }

    if (k.type == kernel_type_t::FOR_END) {
        Str kname(k.name);
        std::replace( kname.begin(), kname.end(), '_', ' ');
        std::istringstream iss(kname);
        Vec<Str> tokens{ std::istream_iterator<Str>{iss},
                                         std::istream_iterator<Str>{} };

        // for now r29, r30, r31 are used
        ss << "    add r31, r31, r30" << std::endl;
#if 0
        // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    blt r31, r29, " << tokens[0] << std::endl;
#else
        ss  <<"    cmp r31, r29" << std::endl;
        ss  <<"    nop" << std::endl;
        ss  <<"    br lt, " << tokens[0] << std::endl;
#endif
    }

    return ss.str();
}

void cc_light_eqasm_compiler::ccl_decompose_pre_schedule(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    for (auto &kernel : programp->kernels) {
        ccl_decompose_pre_schedule_kernel(kernel, platform);
    }

    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

void cc_light_eqasm_compiler::ccl_decompose_post_schedule(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    for (auto &kernel : programp->kernels) {
        QL_IOUT("Decomposing meta-instructions kernel after post-scheduling: " << kernel.name);
        if (!kernel.c.empty()) {
            QL_ASSERT(kernel.cycles_valid);
            ir::bundles_t bundles = ir::bundler(kernel.c, platform.cycle_time);
            ccl_decompose_post_schedule_bundles(bundles, platform);
            kernel.c = ir::circuiter(bundles);
            QL_ASSERT(kernel.cycles_valid);
        }
    }
    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

void cc_light_eqasm_compiler::ccl_decompose_post_schedule_bundles(
    ir::bundles_t &bundles_dst,
    const quantum_platform &platform
) {
    auto bundles_src = bundles_dst;

    QL_IOUT("Post scheduling decomposition ...");
    if (options::get("cz_mode") == "auto") {
        QL_DOUT("Automatically expanding cz to cz_park ...");

        typedef Pair<UInt,UInt> qubits_pair_t;
        Map<qubits_pair_t, UInt> qubitpair2edge; // map: pair of qubits to edge (from grid configuration)
        Map<UInt, Vec<UInt>> edge_detunes_qubits; // map: edge to vector of qubits that edge detunes (resource desc.)

        // initialize qubitpair2edge map from json description; this is a constant map
        // QL_DOUT("... reading edge definitions from topology");
        if (platform.topology.count("edges") <= 0) {
            QL_FATAL("topology[\"edges\"] not defined in configuration file");
        }
        for (auto &anedge : platform.topology["edges"]) {
            // QL_DOUT("... reading edge definitions from topology src");
            UInt s = anedge["src"];
            // QL_DOUT("... reading edge definitions from topology dst");
            UInt d = anedge["dst"];
            // QL_DOUT("... reading edge definitions from topology id");
            UInt e = anedge["id"];

            qubits_pair_t aqpair(s,d);
            // QL_DOUT("... reading edge definitions from topology find aqpair(s,d) : " << s << " " << d);
            auto it = qubitpair2edge.find(aqpair);
            if (it != qubitpair2edge.end()) {
                QL_FATAL("re-defining edge " << s << "->" << d << " !");
            } else {
                // QL_DOUT("... reading edge definitions from topology setting pair2edge to e: " << e);
                qubitpair2edge.set(aqpair) = e;
            }
        }

        // initialize edge_detunes_qubits map from json description; this is a constant map
        // QL_DOUT("... initializing internal edge_detunes_qubits map from detuned_qubits resource");
        auto &constraints = platform.resources["detuned_qubits"]["connection_map"];
        for (auto it = constraints.begin(); it != constraints.end(); ++it) {
            UInt edgeNo = stoi(it.key());
            auto &detuned_qubits = it.value();
            for (auto &q : detuned_qubits) {
                edge_detunes_qubits.set(edgeNo).push_back(q);
            }
        }

        // QL_DOUT("... reading bundles to find candidate two-qubit flux gates");
        for (
            auto bundles_src_it = bundles_src.begin(), bundles_dst_it = bundles_dst.begin();
            bundles_src_it != bundles_src.end() && bundles_dst_it != bundles_dst.end();
            ++bundles_src_it, ++bundles_dst_it
        ) {
            for (
                auto sec_src_it = bundles_src_it->parallel_sections.begin();
                sec_src_it != bundles_src_it->parallel_sections.end();
                ++sec_src_it
            ) {
                for (
                    auto ins_src_it = sec_src_it->begin();
                    ins_src_it != sec_src_it->end();
                    ++ins_src_it
                ) {
                    auto gp = *ins_src_it;
                    // QL_DOUT("... checking gate: " << gp->qasm());
                    Str id = gp->name;
                    Str operation_type{};
                    UInt nOperands = gp->operands.size();
                    if (nOperands == 2) {
                        // QL_DOUT("... is two-qubit gate: " << gp->qasm());
                        auto it = platform.instruction_map.find(id);
                        if (it != platform.instruction_map.end()) {
                            if (platform.instruction_settings[id].count("type") > 0) {
                                operation_type = platform.instruction_settings[id]["type"].get<Str>();
                            }
                        } else {
                            QL_FATAL("custom instruction not found for : " << id << " !");
                        }

                        // QL_DOUT("... operation_type= " << operation_type);
                        UInt q0 = gp->operands[0];
                        UInt q1 = gp->operands[1];
                        Bool is_flux_2_qubit = operation_type == "flux";
                        if (is_flux_2_qubit) {
                            // QL_DOUT("Post scheduling decomposition: found 2 qubit flux gate: " << gp->qasm());
                            qubits_pair_t aqpair(q0, q1);
                            auto it = qubitpair2edge.find(aqpair);
                            if (it != qubitpair2edge.end()) {
                                auto edge_no = it->second;
                                QL_DOUT("checking parked qubits for edge: " << edge_no << ":");
                                Vec<UInt> parked_operands = {};
                                for (auto &q : edge_detunes_qubits.get(edge_no)) {
                                    // QL_DOUT("found parked qubit q" << q);
                                    parked_operands.push_back(q);
                                }
                                if (parked_operands.size() != 0) {
                                    // QL_DOUT("adding parked qubits " << parked_operands << " to gate and adding _park behind its name");
                                    for (auto &q : parked_operands) {
                                        gp->operands.push_back(q);
                                    }
                                    UInt p = id.find(" ");
                                    if (p != Str::npos) {
                                        id = id.substr(0,p);
                                    }
                                    id.append("_park");
                                    gp->name = id;
                                    QL_DOUT("Post scheduling decomposition, added parked qubits: " << gp->qasm());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    QL_IOUT("Post scheduling decomposition [Done]");
}

void cc_light_eqasm_compiler::map(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname,
    Str *mapStatistics
) {
    auto mapopt = options::get("mapper");
    if (mapopt == "no") {
        QL_IOUT("Not mapping kernels");
        return;
    }

    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    mapper::Mapper mapper;  // virgin mapper creation; for role of Init functions, see comment at top of mapper.h
    mapper.Init(&platform); // platform specifies number of real qubits, i.e. locations for virtual qubits

    auto rf = ReportFile(programp, "out", passname);

    UInt total_swaps = 0;        // for reporting, data is mapper specific
    UInt total_moves = 0;        // for reporting, data is mapper specific
    Real total_timetaken = 0.0;  // total over kernels of time taken by mapper
    for (auto &kernel : programp->kernels) {
        QL_IOUT("Mapping kernel: " << kernel.name);

        // compute timetaken, start interval timer here
        Real timetaken = 0.0;
        using namespace std::chrono;
        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        mapper.Map(kernel);
        // kernel.qubit_count starts off as number of virtual qubits, i.e. highest indexed qubit minus 1
        // kernel.qubit_count is updated by Map to highest index of real qubits used minus -1
        programp->qubit_count = platform.qubit_number;
        // program.qubit_count is updated to platform.qubit_number

        // computing timetaken, stop interval timer
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<Real> time_span = t2 - t1;
        timetaken = time_span.count();

        StrStrm ss;
        report_kernel_statistics(ss, kernel, platform, "# ");
        ss << "# ----- swaps added: " << mapper.nswapsadded << std::endl;
        ss << "# ----- of which moves added: " << mapper.nmovesadded << std::endl;
        ss << "# ----- virt2real map before mapper:" << mapper.v2r_in << std::endl;
        ss << "# ----- virt2real map after initial placement:" << mapper.v2r_ip << std::endl;
        ss << "# ----- virt2real map after mapper:" << mapper.v2r_out << std::endl;
        ss << "# ----- realqubit states before mapper:" << mapper.rs_in << std::endl;
        ss << "# ----- realqubit states after mapper:" << mapper.rs_out << std::endl;
        ss << "# ----- time taken: " << timetaken << std::endl;
        rf << ss.str();

        total_swaps += mapper.nswapsadded;
        total_moves += mapper.nmovesadded;
        total_timetaken += timetaken;

        *mapStatistics += ss.str();
    }
    StrStrm ss;
    report_totals_statistics(ss, programp->kernels, platform, "# ");
    ss << "# Total no. of swaps: " << total_swaps << std::endl;
    ss << "# Total no. of moves of swaps: " << total_moves << std::endl;
    ss << "# Total time taken: " << total_timetaken << std::endl;
    rf << ss.str();

    report_qasm(programp, platform, "out", passname);

    // add total statistics
    *mapStatistics += ss.str();
}

void cc_light_eqasm_compiler::ccl_prep_code_generation(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    const Json &instruction_settings = platform.instruction_settings;
    for (const Json &i : instruction_settings) {
        if (i.count("cc_light_instr") <= 0) {
            QL_FATAL("cc_light_instr not found for " << i);
        }
    }
}

// unified entry for quantumsim script writing
// will be moved to dqcsim eventually, which must read cqasm with cycle information; is that sufficient?
void cc_light_eqasm_compiler::write_quantumsim_script(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    // for backward compatibility, use passname to distinguish between calls from different places
    Bool compiled;
    Str suffix;
    if (passname == "write_quantumsim_script_unmapped") {
        compiled = false;
        suffix = "";
    } else if (passname == "write_quantumsim_script_mapped") {
        compiled = true;
        suffix = "mapped";
    } else {
        QL_FATAL("Write_quantumsim_script: unknown passname: " << passname);
    }

    // dqcsim must take over
    if (options::get("quantumsim") == "yes") {
        write_quantumsim_program(programp, platform.qubit_number, platform, suffix);
    } else if (options::get("quantumsim") == "qsoverlay") {
        write_qsoverlay_program(programp, platform.qubit_number, platform, suffix, platform.cycle_time, compiled);
    }

    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

/**
 * program-level compilation of qasm to cc_light_eqasm
 */
void cc_light_eqasm_compiler::compile(
    const Str &prog_name,
    circuit &ckt,
    const quantum_platform &platform
) {
    QL_FATAL("cc_light_eqasm_compiler::compile interface with circuit not supported");
}

// kernel level compilation
void cc_light_eqasm_compiler::compile(quantum_program *programp, const quantum_platform &platform) {
    //std::cout << " ============= DEBUG PRINT FOR DEBUG(1): In cc_light BACKEND COMPILER " << std::endl;
    QL_DOUT("Compiling " << programp->kernels.size() << " kernels to generate CCLight eQASM ... ");

    // overall timing should be done by the pass manager
    // can be deleted here when so
    //
    // each pass can also have a local timer;
    // can also be done by pass manager in parallel to skip option
    //
    // compute timetaken, start interval timer here
    Real    total_timetaken = 0.0;
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    // see comment with definition
    // could also be in back-end constructor, or even be deleted
    ccl_prep_code_generation(programp, platform, "ccl_prep_code_generation");

    // decompose_pre_schedule pass
    // is very much concerned with generation of classical code
    ccl_decompose_pre_schedule(programp, platform, "ccl_decompose_pre_schedule");

    // this call could also have been at end of back-end-independent passes
    write_quantumsim_script(programp, platform, "write_quantumsim_script_unmapped");

    clifford_optimize(programp, platform, "clifford_premapper");

    // map function definition must be moved to src/mapper.h and src/mapper.cc
    // splitting src/mapper.h into src/mapper.h and src/mapper.cc is intricate
    // because mapper shares ddg code with scheduler
    // this implies that those latter interfaces must be made public in scheduler.h before splitting
    // scheduler.h and mapper.h
    Str emptystring = "";
    map(programp, platform, "mapper", &emptystring);

    clifford_optimize(programp, platform, "clifford_postmapper");

    rcschedule(programp, platform, "rcscheduler");

    latency_compensation(programp, platform, "ccl_latency_compensation");

    insert_buffer_delays(programp, platform, "ccl_insert_buffer_delays");

    // decompose meta-instructions after scheduling
    ccl_decompose_post_schedule(programp, platform, "ccl_decompose_post_schedule");

    // just before code generation, emit quantumsim script to best match target architecture
    write_quantumsim_script(programp, platform, "write_quantumsim_script_mapped");

    // and now for real
    if (options::get("generate_code") == "yes") {
        qisa_code_generation(programp, platform, "qisa_code_generation");
    }

    // timing to be moved to pass manager
    // computing timetaken, stop interval timer
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<Real> time_span = t2 - t1;
    total_timetaken = time_span.count();

    // reporting to be moved to write_statistics pass
    // report totals over all kernels, over all eqasm passes contributing to mapping
    auto rf = ReportFile(programp, "out", "cc_light_compiler");
    for (const auto &k : programp->kernels) {
        rf.write_kernel_statistics(k, platform, "# ");
    }
    rf.write_totals_statistics(programp->kernels, platform, "# ");
    rf << "# Total time taken: " << total_timetaken << "\n";
    report_qasm(programp, platform, "out", "cc_light_compiler");

    QL_DOUT("Compiling CCLight eQASM [Done]");
}

/**
 * decompose
 */
// decompose meta-instructions
void cc_light_eqasm_compiler::ccl_decompose_pre_schedule_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform
) {
    QL_IOUT("Decomposing kernel: " << kernel.name);
    if (kernel.c.empty()) {
        return;
    }
    circuit decomp_ckt;	// collect result circuit in here and before return swap with kernel.c

    QL_DOUT("decomposing instructions...");
    for (auto ins : kernel.c) {
        auto iname = to_lower(ins->name);
        QL_DOUT("decomposing instruction " << iname << "...");
        auto & icopers = ins->creg_operands;
        auto & iqopers = ins->operands;
        Int icopers_count = icopers.size();
        Int iqopers_count = iqopers.size();
        QL_DOUT("decomposing instruction " << iname << " operands=" << to_string(iqopers) << " creg_operands=" << to_string(icopers));
        auto itype = ins->type();
        if (itype == __classical_gate__) {
            QL_DOUT("    classical instruction: " << ins->qasm());

            if (
                (iname == "add") || (iname == "sub") ||
                (iname == "and") || (iname == "or") || (iname == "xor") ||
                (iname == "not") || (iname == "nop")
            ) {
                // decomp_ckt.push_back(ins);
                decomp_ckt.push_back(new classical_cc(iname, icopers));
                QL_DOUT("    classical instruction decomposed: " << decomp_ckt.back()->qasm());
            } else if (
                (iname == "eq") || (iname == "ne") || (iname == "lt") ||
                (iname == "gt") || (iname == "le") || (iname == "ge")
            ) {
                decomp_ckt.push_back(new classical_cc("cmp", {icopers[1], icopers[2]}));
                QL_DOUT("    classical instruction decomposed: " << decomp_ckt.back()->qasm());
                decomp_ckt.push_back(new classical_cc("nop", {}));
                QL_DOUT("                                      " << decomp_ckt.back()->qasm());
                decomp_ckt.push_back(new classical_cc("fbr_"+iname, {icopers[0]}));
                QL_DOUT("                                      " << decomp_ckt.back()->qasm());
            } else if (iname == "mov") {
                // r28 is used as temp, TODO use creg properly to create temporary
                decomp_ckt.push_back(new classical_cc("ldi", {28}, 0));
                QL_DOUT("    classical instruction decomposed: " << decomp_ckt.back()->qasm());
                decomp_ckt.push_back(new classical_cc("add", {icopers[0], icopers[1], 28}));
                QL_DOUT("                                      " << decomp_ckt.back()->qasm());
            } else if (iname == "ldi") {
                // auto imval = ((classical_cc*)ins)->int_operand;
                auto imval = ((classical*)ins)->int_operand;
                QL_DOUT("    classical instruction decomposed: imval=" << imval);
                decomp_ckt.push_back(new classical_cc("ldi", {icopers[0]}, imval));
                QL_DOUT("    classical instruction decomposed: " << decomp_ckt.back()->qasm());
            } else {
                QL_EOUT("Unknown decomposition of classical operation '" << iname << "' with '" << icopers_count << "' operands!");
                throw Exception("Unknown classical operation '" + iname + "' with'" + to_string(icopers_count) + "' operands!", false);
            }
        } else {
            if (iname == "wait") {
                QL_DOUT("    wait instruction ");
                decomp_ckt.push_back(ins);
            } else {
                const Json &instruction_settings = platform.instruction_settings;
                Str operation_type;
                if (instruction_settings.find(iname) != instruction_settings.end()) {
                    operation_type = instruction_settings[iname]["type"].get<Str>();
                } else {
                    QL_EOUT("instruction settings not found for '" << iname << "' with '" << iqopers_count << "' operands!");
                    throw Exception("instruction settings not found for '" + iname + "' with'" + to_string(iqopers_count) + "' operands!", false);
                }
                Bool is_measure = (operation_type == "readout");
                if (is_measure) {
                    // insert measure
                    QL_DOUT("    readout instruction ");
                    auto qop = iqopers[0];
                    decomp_ckt.push_back(ins);
                    if (itype == gate_type_t::__custom_gate__) {
                        auto &coperands = ins->creg_operands;
                        if (!coperands.empty()) {
                            auto cop = coperands[0];
                            decomp_ckt.push_back(new classical_cc("fmr", {cop, qop}));
                        } else {
                            // WOUT("Unknown classical operand for measure/readout operation: '" << iname <<
                            //     ". This will soon be depricated in favour of measure instruction with fmr" <<
                            //     " to store measurement outcome to classical register.");
                        }
                    } else {
                        QL_EOUT("Unknown decomposition of measure/readout operation: '" << iname << "!");
                        throw Exception("Unknown decomposition of measure/readout operation '" + iname + "'!", false);
                    }
                } else {
                    QL_DOUT("    quantum instruction ");
                    decomp_ckt.push_back(ins);
                }
            }
        }
    }
    kernel.c = decomp_ckt;;

    QL_DOUT("decomposing instructions...[Done]");
}

// qisa_code_generation pass
// generates qisa from IR
void cc_light_eqasm_compiler::qisa_code_generation(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    (void)passname;
    MaskManager mask_manager;
    StrStrm ssqisa, sskernels_qisa;
    sskernels_qisa << "start:" << std::endl;
    for (auto &kernel : programp->kernels) {
        sskernels_qisa << std::endl << kernel.name << ":" << std::endl;
        sskernels_qisa << get_qisa_prologue(kernel);
        if (!kernel.c.empty()) {
            sskernels_qisa << ir2qisa(kernel, platform, mask_manager);
        }
        sskernels_qisa << get_qisa_epilogue(kernel);
    }
    sskernels_qisa << std::endl
                   << "    br always, start" << std::endl
                   << "    nop " << std::endl
                   << "    nop" << std::endl;
    ssqisa << mask_manager.getMaskInstructions() << sskernels_qisa.str();
    // std::cout << ssqisa.str();

    // write cc-light qisa file
    Str unique_name = programp->unique_name;
    Str qisafname(options::get("output_dir") + "/" + unique_name + ".qisa");
    QL_IOUT("Writing CC-Light QISA to " << qisafname);
    ssqisa << std::endl;
    OutFile(qisafname).write(ssqisa.str());
    // end qisa_generation pass
}

// write cc_light scheduled bundles for quantumsim
// when cc_light independent, it should be extracted and put in src/quantumsim.h
void cc_light_eqasm_compiler::write_quantumsim_program(
    quantum_program *programp,
    UInt num_qubits,
    const quantum_platform &platform,
    const Str &suffix
) {
    QL_IOUT("Writing scheduled Quantumsim program");
    Str qfname( options::get("output_dir") + "/" + "quantumsim_" + programp->unique_name + "_" + suffix + ".py");
    QL_DOUT("Writing scheduled Quantumsim program to " << qfname);
    QL_IOUT("Writing scheduled Quantumsim program to " << qfname);
    OutFile fout(qfname);

    fout << "# Quantumsim program generated OpenQL\n"
         << "# Please modify at your will to obtain extra information from Quantumsim\n\n";

    fout << "import numpy as np\n"
         << "from quantumsim.circuit import Circuit\n"
         << "from quantumsim.circuit import uniform_noisy_sampler\n"
         << "from quantumsim.circuit import ButterflyGate\n"
         << "\n";

    fout << "from quantumsim.circuit import IdlingGate as i\n"
         << "from quantumsim.circuit import RotateY as ry\n"
         << "from quantumsim.circuit import RotateX as rx\n"
         << "from quantumsim.circuit import RotateZ as rz\n"
         << "from quantumsim.circuit import Hadamard as h\n"
         << "from quantumsim.circuit import NoisyCPhase as cz\n"
         << "from quantumsim.circuit import CNOT as cnot\n"
         << "from quantumsim.circuit import Swap as swap\n"
         << "from quantumsim.circuit import CPhaseRotation as cr\n"
         << "from quantumsim.circuit import ConditionalGate as ConditionalGate\n"
         << "from quantumsim.circuit import RotateEuler as RotateEuler\n"
         << "from quantumsim.circuit import ResetGate as ResetGate\n"
         << "from quantumsim.circuit import Measurement as measure\n"
         << "import quantumsim.sparsedm as sparsedm\n"
         << "\n"
         << "# print('GPU is used:', sparsedm.using_gpu)\n"
         << "\n"
         << "\n"
         << "def t(q, time):\n"
         << "    return RotateEuler(q, time=time, theta=0, phi=np.pi/4, lamda=0)\n"
         << "\n"
         << "def tdag(q, time):\n"
         << "    return RotateEuler(q, time=time, theta=0, phi=-np.pi/4, lamda=0)\n"
         << "\n"
         << "def measure_z(q, time, sampler):\n"
         << "    return measure(q, time, sampler)\n"
         << "\n"
         << "def z(q, time):\n"
         << "    return rz(q, time, angle=np.pi)\n"
         << "\n"
         << "def x(q, time, dephasing_axis, dephasing_angle):\n"
         << "    return rx(q, time, angle=np.pi, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"
         << "\n"
         << "def y(q, time, dephasing_axis, dephasing_angle):\n"
         << "    return ry(q, time, angle=np.pi, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"
         << "\n"
         << "def x90(q, time, dephasing_axis, dephasing_angle):\n"
         << "    return rx(q, time, angle=np.pi/2, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"
         << "\n"
         << "def y90(q, time, dephasing_axis, dephasing_angle):\n"
         << "    return ry(q, time, angle=np.pi/2, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"
         << "\n"
         << "def xm90(q, time, dephasing_axis, dephasing_angle):\n"
         << "    return rx(q, time, angle=-np.pi/2, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"
         << "\n"
         << "def ym90(q, time, dephasing_axis, dephasing_angle):\n"
         << "    return ry(q, time, angle=-np.pi/2, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"
         << "\n"
         << "def x45(q, time):\n"
         << "    return rx(q, time, angle=np.pi/4)\n"
         << "\n"
         << "def xm45(q, time):\n"
         << "    return rx(q, time, angle=-np.pi/4)\n"
         << "\n"
         //<< "def cz(q, time, dephase_var):\n"
         //<< "    return cphase(q, time, dephase_var=dephase_var)\n"
         << "\n"
         << "def prepz(q, time):\n"
         << "    return ResetGate(q, time, state=0)\n\n"
         << "\n";

    fout << "\n" << "# create a circuit\n";
    fout << "def circuit_generated(t1=np.inf, t2=np.inf, dephasing_axis=None, dephasing_angle=None, dephase_var=0, readout_error=0.0) :\n";
    fout << "    c = Circuit(title=\"" << programp->unique_name << "\")\n";

    QL_DOUT("Adding qubits to Quantumsim program");
    fout << "\n    # add qubits\n";
    Json config;
    try {
        config = load_json(platform.configuration_file_name);
    } catch (Json::exception e) {
        throw Exception("[x] error : quantumsim_compiler::load() :  failed to load the hardware config file : malformed json file ! : \n    " +
                        Str(e.what()), false);
    }

    // load qubit attributes
    Json qubit_attributes = config["qubit_attributes"];
    if (qubit_attributes.is_null()) {
        QL_EOUT("qubit_attributes is not specified in the hardware config file !");
        throw Exception("[x] error: quantumsim_compiler: qubit_attributes is not specified in the hardware config file !", false);
    }
    Json relaxation_times = qubit_attributes["relaxation_times"];
    if (relaxation_times.is_null()) {
        QL_EOUT("relaxation_times is not specified in the hardware config file !");
        throw Exception("[x] error: quantumsim_compiler: relaxation_times is not specified in the hardware config file !", false);
    }
    UInt count = platform.hardware_settings["qubit_number"];

    // want to ignore unused qubits below
    QL_ASSERT(programp->kernels.size() <= 1);
    Vec<UInt> check_usecount;
    check_usecount.resize(count, 0);

    for (auto &gp : programp->kernels.front().c) {
        switch (gp->type()) {
            case __classical_gate__:
            case __wait_gate__:
                break;
            default:    // quantum gate
                for (auto v: gp->operands) {
                    check_usecount[v]++;
                }
                break;
        }
    }

    for (auto it = relaxation_times.begin(); it != relaxation_times.end(); ++it) {
        UInt q = stoi(it.key());
        if (q >= count) {
            QL_EOUT("qubit_attribute.relaxation_time.qubit number is not in qubits available in the platform");
            throw Exception("[x] error: qubit_attribute.relaxation_time.qubit number is not in qubits available in the platform", false);
        }
        if (check_usecount[q] == 0) {
            QL_DOUT("... qubit " << q << " is not used; skipping it");
            continue;
        }
        auto & rt = it.value();
        if (rt.size() < 2) {
            QL_EOUT("each qubit must have at least two relaxation times");
            throw Exception("[x] error: each qubit must have at least two relaxation times", false);
        }
        // fout << "    c.add_qubit(\"q" << q <<"\", " << rt[0] << ", " << rt[1] << ")\n";
        fout << "    c.add_qubit(\"q" << q << "\", t1=t1, t2=t2)\n";
    }

    QL_DOUT("Adding Gates to Quantumsim program");
    {
        // global writes
        StrStrm ssqs;
        ssqs << std::endl << "    sampler = uniform_noisy_sampler(readout_error=readout_error, seed=42)" << std::endl;
        ssqs << std::endl << "    # add gates" << std::endl;
        fout << ssqs.str();
    }
    for (auto &kernel : programp->kernels) {
        QL_DOUT("... adding gates, a new kernel");
        QL_ASSERT(kernel.cycles_valid);
        ir::bundles_t bundles = ir::bundler(kernel.c, platform.cycle_time);

        if (bundles.empty()) {
            QL_IOUT("No bundles for adding gates");
        } else {
            for (ir::bundle_t &abundle : bundles) {
                QL_DOUT("... adding gates, a new bundle");
                auto bcycle = abundle.start_cycle;

                StrStrm ssqs;
                for(auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt) {
                    QL_DOUT("... adding gates, a new section in a bundle");
                    for (auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt) {
                        auto & iname = (*insIt)->name;
                        auto & operands = (*insIt)->operands;
                        auto duration = (*insIt)->duration;     // duration in nano-seconds
                        // UInt operation_duration = ceil(static_cast<Real>(duration) / platform.cycle_time);
                        if (iname == "measure") {
                            QL_DOUT("... adding gates, a measure");
                            auto op = operands.back();
                            ssqs << "    c.add_qubit(\"m" << op << "\")" << std::endl;
                            ssqs << "    c.add_gate("
                                 << "ButterflyGate("
                                 << "\"q" << op <<"\", "
                                 << "time=" << ((bcycle-1)*platform.cycle_time) << ", "
                                 << "p_exc=0,"
                                 << "p_dec= 0.005)"
                                 << ")" << std::endl;
                            ssqs << "    c.add_measurement("
                                 << "\"q" << op << "\", "
                                 << "time=" << ((bcycle - 1)*platform.cycle_time) + (duration/4) << ", "
                                 << "output_bit=\"m" << op << "\", "
                                 << "sampler=sampler"
                                 << ")" << std::endl;
                            ssqs << "    c.add_gate("
                                 << "ButterflyGate("
                                 << "\"q" << op << "\", "
                                 << "time=" << ((bcycle - 1)*platform.cycle_time) + duration/2 << ", "
                                 << "p_exc=0,"
                                 << "p_dec= 0.015)"
                                 << ")" << std::endl;

                        } else if (
                            iname == "y90" || iname == "ym90" || iname == "y" || iname == "x" ||
                            iname == "x90" || iname == "xm90"
                        ) {
                            QL_DOUT("... adding gates, another gate");
                            ssqs <<  "    c.add_gate("<< iname << "(" ;
                            UInt noperands = operands.size();
                            if (noperands > 0) {
                                for (auto opit = operands.begin(); opit != operands.end()-1; opit++) {
                                    ssqs << "\"q" << *opit <<"\", ";
                                }
                                ssqs << "\"q" << operands.back()<<"\"";
                            }
                            ssqs << ", time=" << ((bcycle - 1)*platform.cycle_time) + (duration/2) << ", dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle))" << std::endl;
                        } else if (iname == "cz") {
                            QL_DOUT("... adding gates, another gate");
                            ssqs <<  "    c.add_gate("<< iname << "(" ;
                            UInt noperands = operands.size();
                            if (noperands > 0) {
                                for (auto opit = operands.begin(); opit != operands.end()-1; opit++) {
                                    ssqs << "\"q" << *opit <<"\", ";
                                }
                                ssqs << "\"q" << operands.back()<<"\"";
                            }
                            ssqs << ", time=" << ((bcycle - 1)*platform.cycle_time) + (duration/2) << ", dephase_var=dephase_var))" << std::endl;
                        } else {
                            QL_DOUT("... adding gates, another gate");
                            ssqs <<  "    c.add_gate("<< iname << "(" ;
                            UInt noperands = operands.size();
                            if (noperands > 0) {
                                for (auto opit = operands.begin(); opit != operands.end()-1; opit++) {
                                    ssqs << "\"q" << *opit <<"\", ";
                                }
                                ssqs << "\"q" << operands.back()<<"\"";
                            }
                            ssqs << ", time=" << ((bcycle - 1)*platform.cycle_time) + (duration/2) << "))" << std::endl;
                        }
                    }
                }
                fout << ssqs.str();
            }
            fout << "    return c";
            fout << "    \n\n";
            report_kernel_statistics(fout.unwrap(), kernel, platform, "    # ");
        }
    }
    report_string(fout.unwrap(), "    \n");
    report_string(fout.unwrap(), "    # Program-wide statistics:\n");
    report_totals_statistics(fout.unwrap(), programp->kernels, platform, "    # ");
    fout << "    return c";

    fout.close();
    QL_IOUT("Writing scheduled Quantumsim program [Done]");
}

} // namespace arch
} // namespace ql
