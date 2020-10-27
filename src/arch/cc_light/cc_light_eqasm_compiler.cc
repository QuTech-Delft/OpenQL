#include "cc_light_eqasm_compiler.h"

namespace ql {
namespace arch {

size_t CurrSRegCount;
size_t CurrTRegCount;

Mask::Mask(const qubit_set_t &qs) : squbits(qs) {
    if (CurrSRegCount < MAX_S_REG) {
        regNo = CurrSRegCount++;
        regName = "s" + std::to_string(regNo);
    } else {
        COUT(" !!!! Handle cases requiring more registers");
    }
}

Mask::Mask(const std::string &rn, const qubit_set_t &qs) : regName(rn), squbits(qs) {
    if (CurrSRegCount < MAX_S_REG) {
        regNo = CurrSRegCount++;
    } else {
        COUT(" !!!! Handle cases requiring more registers");
    }
}

Mask::Mask(const qubit_pair_set_t &qps) : dqubits(qps) {
    if (CurrTRegCount < MAX_T_REG) {
        regNo = CurrTRegCount++;
        regName = "t" + std::to_string(regNo);
    } else {
        COUT(" !!!! Handle cases requiring more registers");
    }
}

MaskManager::MaskManager() {
    // add pre-defined smis
    for (size_t i = 0; i < 7; ++i) {
        qubit_set_t qs;
        qs.push_back(i);
        Mask m(qs);
        QS2Mask[qs] = m;
        SReg2Mask[m.regNo] = m;
    }

    // add some common single qubit masks
    {
        qubit_set_t qs;
        for(auto i=0; i<7; i++) qs.push_back(i);
        Mask m(qs); // TODO add proper support for:  Mask m(qs, "all_qubits");
        QS2Mask[qs] = m;
        SReg2Mask[m.regNo] = m;
    }

    {
        qubit_set_t qs;
        qs.push_back(0); qs.push_back(1); qs.push_back(5); qs.push_back(6);
        Mask m(qs); // TODO add proper support for:  Mask m(qs, "data_qubits");
        QS2Mask[qs] = m;
        SReg2Mask[m.regNo] = m;
    }

    {
        qubit_set_t qs;
        qs.push_back(2); qs.push_back(3); qs.push_back(4);
        Mask m(qs); // TODO add proper support for:  Mask m(qs, "ancilla_qubits");
        QS2Mask[qs] = m;
        SReg2Mask[m.regNo] = m;
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

size_t MaskManager::getRegNo(qubit_set_t &qs) {
    // sort qubit operands to avoid variation in order
    sort(qs.begin(), qs.end());

    auto it = QS2Mask.find(qs);
    if (it == QS2Mask.end()) {
        Mask m(qs);
        QS2Mask[qs] = m;
        SReg2Mask[m.regNo] = m;
    }
    return QS2Mask[qs].regNo;
}

size_t MaskManager::getRegNo(qubit_pair_set_t &qps) {
    // sort qubit operands pair to avoid variation in order
    sort(qps.begin(), qps.end(), utils::sort_pair_helper);

    auto it = QPS2Mask.find(qps);
    if (it == QPS2Mask.end()) {
        Mask m(qps);
        QPS2Mask[qps] = m;
        TReg2Mask[m.regNo] = m;
    }
    return QPS2Mask[qps].regNo;
}

std::string MaskManager::getRegName(qubit_set_t &qs) {
    // sort qubit operands to avoid variation in order
    sort(qs.begin(), qs.end());

    auto it = QS2Mask.find(qs);
    if (it == QS2Mask.end()) {
        Mask m(qs);
        QS2Mask[qs] = m;
        SReg2Mask[m.regNo] = m;
    }
    return QS2Mask[qs].regName;
}

std::string MaskManager::getRegName(qubit_pair_set_t &qps) {
    // sort qubit operands pair to avoid variation in order
    sort(qps.begin(), qps.end(), utils::sort_pair_helper);

    auto it = QPS2Mask.find(qps);
    if (it == QPS2Mask.end()) {
        Mask m(qps);
        QPS2Mask[qps] = m;
        TReg2Mask[m.regNo] = m;
    }
    return QPS2Mask[qps].regName;
}

std::string MaskManager::getMaskInstructions() {
    std::stringstream ssmasks;
    for (size_t r = 0; r < CurrSRegCount; ++r) {
        auto & m = SReg2Mask[r];
        ssmasks << "smis " << m.regName << ", {";
        for (auto it = m.squbits.begin(); it != m.squbits.end(); ++it) {
            ssmasks << *it;
            if (std::next(it) != m.squbits.end()) {
                ssmasks << ", ";
            }
        }
        ssmasks << "} " << std::endl;
    }

    for (size_t r = 0; r < CurrTRegCount; ++r) {
        auto & m = TReg2Mask[r];
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
    const std::string &operation,
    const std::vector<size_t> &opers,
    int ivalue
) {
    DOUT("Classical_cc constructor for operation " << operation);
    DOUT("... operands:");
    for (auto o : opers) DOUT ("...... " << o << " ");
    DOUT("...... ivalue= " << ivalue);

    // DOUT("adding classical_cc " << operation);
    name = utils::to_lower(operation);
    duration = 20;
    creg_operands=opers;
    int sz = creg_operands.size();
    if (
        (
            (name == "add") || (name == "sub")
            || (name == "and") || (name == "or") || (name == "xor")
        ) && (sz == 3)
        ) {
        DOUT("Adding 3 operand operation: " << name);
    } else if (((name == "not") || (name == "cmp")) && (sz == 2)) {
        DOUT("Adding 2 operand operation: " << name);
    } else if ((name == "fmr")  && (sz == 2)) {
        creg_operands= { opers[0] };
        operands= { opers[1] };
        DOUT("Adding 2 operand fmr operation: " << name);
    } else if (
        (
            (name == "ldi") ||
            (name == "fbr_eq") || (name == "fbr_ne") || (name == "fbr_lt") ||
            (name == "fbr_gt") || (name == "fbr_le") || (name == "fbr_ge")
        ) && (sz == 1)
        ) {
        if (name == "ldi") {
            DOUT("... setting int_operand of classical_cc gate for operation " << name << " to " << ivalue);
            int_operand = ivalue;
        }
        DOUT("Adding 1 operand operation: " << name);
    } else if ((name == "nop") && (sz == 0)) {
        DOUT("Adding 0 operand operation: " << name);
    } else {
        EOUT("Unknown cclight classical operation '" << name << "' with '" << sz << "' operands!");
        throw exception("Unknown cclight classical operation'"+name+"' with'"+std::to_string(sz)+"' operands!", false);
    }
    DOUT("adding classical_cc [DONE]");
}

instruction_t classical_cc::qasm() const {
    std::string iopers;
    int sz = creg_operands.size();
    for (int i = 0; i < sz; ++i) {
        if (i == sz - 1) {
            iopers += " r" + std::to_string(creg_operands[i]);
        } else {
            iopers += " r" + std::to_string(creg_operands[i]) + ",";
        }
    }

    if (name == "ldi") {
        iopers += ", " + std::to_string(int_operand);
        return "ldi" + iopers;
    } else if (name == "fmr") {
        return name + " r" + std::to_string(creg_operands[0]) +
               ", q" + std::to_string(operands[0]);
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

std::string classical_instruction2qisa(classical_cc *classical_ins) {
    std::stringstream ssclassical;
    auto &iname = classical_ins->name;
    auto &iopers = classical_ins->creg_operands;
    int iopers_count = iopers.size();

    if (
        (iname == "add") || (iname == "sub") ||
        (iname == "and") || (iname == "or") || (iname == "not") ||
        (iname == "xor") ||
        (iname == "ldi") || (iname == "nop") || (iname == "cmp")
    ) {
        ssclassical << iname;
        for (int i = 0; i < iopers_count; ++i) {
            if (i == iopers_count - 1) {
                ssclassical << " r" << iopers[i];
            } else {
                ssclassical << " r" << iopers[i] << ",";
            }
        }
        if (iname == "ldi") {
            ssclassical << ", " + std::to_string(classical_ins->int_operand);
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
        EOUT("Unknown CClight classical operation '" << iname << "' with '" << iopers_count << "' operands!");
        throw exception("Unknown classical operation'" + iname + "' with'" + std::to_string(iopers_count) + "' operands!", false);
    }

    return ssclassical.str();
}

// FIXME HvS cc_light_instr is name of attribute in json file, in gate: arch_operation_name, here in instruction_map?
// FIXME HvS attribute of gate or just in json? Generalization to arch_operation_name is unnecessary
std::string get_cc_light_instruction_name(
    const std::string &id,
    const quantum_platform &platform
) {
    std::string cc_light_instr_name;
    auto it = platform.instruction_map.find(id);
    if (it != platform.instruction_map.end()) {
        custom_gate* g = it->second;
        cc_light_instr_name = g->arch_operation_name;
        if (cc_light_instr_name.empty()) {
            FATAL("cc_light_instr not defined for instruction: " << id << " !");
        }
        // DOUT("cc_light_instr name: " << cc_light_instr_name);
    } else {
        FATAL("custom instruction not found for : " << id << " !");
    }
    return cc_light_instr_name;
}

std::string ir2qisa(
    quantum_kernel &kernel,
    const quantum_platform &platform,
    MaskManager &gMaskManager
) {
    IOUT("Generating CC-Light QISA");

    ir::bundles_t bundles1;
    ASSERT(kernel.cycles_valid);
    bundles1 = ir::bundler(kernel.c, platform.cycle_time);

    IOUT("Combining parallel sections...");
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
                        DOUT("Not splicing " << id1 << " and " << id2);
                        continue;
                    }

                    auto n1 = get_cc_light_instruction_name(id1, platform);
                    auto n2 = get_cc_light_instruction_name(id2, platform);
                    if (n1 == n2) {
                        DOUT("Splicing " << id1 << "/" << n1 << " and " << id2 << "/" << n2);
                        (*secIt1).splice(insIt1, (*secIt2));
                    } else {
                        DOUT("Not splicing " << id1 << "/" << n1 << " and " << id2 << "/" << n2);
                    }
                }
            }
        }
    }
    ir::DebugBundles("After combining", bundles1);
    IOUT("Removing empty sections...");
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
    IOUT("Sorting sections alphabetically according to instruction name ...");
    for (ir::bundle_t &abundle : bundles2) {
        // sorts instructions alphabetically
        abundle.parallel_sections.sort(
            [](const ir::section_t &sec1, const ir::section_t &sec2) -> bool {
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
    std::stringstream ssqisa;   // output qisa in here
    size_t curr_cycle = 0; // first instruction should be with pre-interval 1, 'bs 1' FIXME HvS start in cycle 0
    for (ir::bundle_t &abundle : bundles2) {
        std::string iname;
        std::stringstream sspre, ssinst;
        auto bcycle = abundle.start_cycle;
        auto delta = bcycle - curr_cycle;
        bool classical_bundle=false;
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
                DOUT("get cclight instr name for : " << iname);
                std::string cc_light_instr_name = get_cc_light_instruction_name(iname, platform);
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
                            throw exception("Error : only 1 and 2 operand instructions are supported by cc light masks !",false);
                        }
                    }

                    std::string rname;
                    if (nOperands == 1) {
                        rname = gMaskManager.getRegName(squbits);
                    } else if (nOperands == 2) {
                        rname = gMaskManager.getRegName(dqubits);
                    } else {
                        throw exception("Error : only 1 and 2 operand instructions are supported by cc light masks !",false);
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
    int lbduration = lastBundle.duration_in_cycles;
    if (lbduration > 1) {
        ssqisa << "    qwait " << lbduration << std::endl;
    }

    IOUT("Generating CC-Light QISA [Done]");
    return ssqisa.str();
}

std::string cc_light_eqasm_compiler::get_qisa_prologue(const quantum_kernel &k) {
    std::stringstream ss;

    if (k.type == kernel_type_t::IF_START) {
#if 0
        // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition.inv_operation_name
               <<" r" << (k.br_condition.operands[0])->id <<", r" << (k.br_condition.operands[1])->id
               << ", " << k.name << "_end" << std::endl;
#else
        ss  <<"    cmp r" << (k.br_condition.operands[0])->id
            <<", r" << (k.br_condition.operands[1])->id << std::endl;
        ss  <<"    nop" << std::endl;
        ss  <<"    br " << k.br_condition.inv_operation_name << ", "
            << k.name << "_end" << std::endl;
#endif

    }

    if (k.type == kernel_type_t::ELSE_START) {
#if 0
        // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition.operation_name <<" r" << (k.br_condition.operands[0])->id
               <<", r" << (k.br_condition.operands[1])->id << ", " << k.name << "_end" << std::endl;
#else
        ss  <<"    cmp r" << (k.br_condition.operands[0])->id
            <<", r" << (k.br_condition.operands[1])->id << std::endl;
        ss  <<"    nop" << std::endl;
        ss  <<"    br " << k.br_condition.operation_name << ", "
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

std::string cc_light_eqasm_compiler::get_qisa_epilogue(const quantum_kernel &k) {
    std::stringstream ss;

    if (k.type == kernel_type_t::DO_WHILE_END) {
#if 0
        // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition.operation_name <<" r" << (k.br_condition.operands[0])->id
               <<", r" << (k.br_condition.operands[1])->id << ", " << k.name << "_start" << std::endl;
#else
        ss  <<"    cmp r" << (k.br_condition.operands[0])->id
            <<", r" << (k.br_condition.operands[1])->id << std::endl;
        ss  <<"    nop" << std::endl;
        ss  <<"    br " << k.br_condition.operation_name << ", "
            << k.name << "_start" << std::endl;
#endif
    }

    if (k.type == kernel_type_t::FOR_END) {
        std::string kname(k.name);
        std::replace( kname.begin(), kname.end(), '_', ' ');
        std::istringstream iss(kname);
        std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                         std::istream_iterator<std::string>{} };

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
    const std::string &passname
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
    const std::string &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    for (auto &kernel : programp->kernels) {
        IOUT("Decomposing meta-instructions kernel after post-scheduling: " << kernel.name);
        if (!kernel.c.empty()) {
            ASSERT(kernel.cycles_valid);
            ir::bundles_t bundles = ir::bundler(kernel.c, platform.cycle_time);
            ccl_decompose_post_schedule_bundles(bundles, platform);
            kernel.c = ir::circuiter(bundles);
            ASSERT(kernel.cycles_valid);
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

    IOUT("Post scheduling decomposition ...");
    if (options::get("cz_mode") == "auto") {
        IOUT("decompose cz to cz+sqf...");

        typedef std::pair<size_t,size_t> qubits_pair_t;
        std::map< qubits_pair_t, size_t > qubitpair2edge;           // map: pair of qubits to edge (from grid configuration)
        std::map<size_t, std::vector<size_t> > edge_detunes_qubits; // map: edge to vector of qubits that edge detunes (resource desc.)

        // initialize qubitpair2edge map from json description; this is a constant map
        for (auto &anedge : platform.topology["edges"]) {
            size_t s = anedge["src"];
            size_t d = anedge["dst"];
            size_t e = anedge["id"];

            qubits_pair_t aqpair(s,d);
            auto it = qubitpair2edge.find(aqpair);
            if (it != qubitpair2edge.end()) {
                FATAL("re-defining edge " << s <<"->" << d << " !");
            } else {
                qubitpair2edge[aqpair] = e;
            }
        }

        // initialize edge_detunes_qubits map from json description; this is a constant map
        auto & constraints = platform.resources["detuned_qubits"]["connection_map"];
        for (auto it = constraints.begin(); it != constraints.end(); ++it) {
            size_t edgeNo = stoi( it.key() );
            auto & detuned_qubits = it.value();
            for (auto &q : detuned_qubits) {
                edge_detunes_qubits[edgeNo].push_back(q);
            }
        }

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
                    std::string id = (*ins_src_it)->name;
                    std::string operation_type{};
                    size_t nOperands = ((*ins_src_it)->operands).size();
                    if (nOperands == 2) {
                        auto it = platform.instruction_map.find(id);
                        if (it != platform.instruction_map.end()) {
                            if (platform.instruction_settings[id].count("type") > 0) {
                                operation_type = platform.instruction_settings[id]["type"].get<std::string>();
                            }
                        } else {
                            FATAL("custom instruction not found for : " << id << " !");
                        }

                        bool is_flux_2_qubit = operation_type == "flux";
                        if (is_flux_2_qubit) {
                            auto &q0 = (*ins_src_it)->operands[0];
                            auto &q1 = (*ins_src_it)->operands[1];
                            DOUT("found 2 qubit flux gate on " << q0 << " and " << q1);
                            qubits_pair_t aqpair(q0, q1);
                            auto it = qubitpair2edge.find(aqpair);
                            if (it != qubitpair2edge.end()) {
                                auto edge_no = qubitpair2edge[aqpair];
                                DOUT("add the following sqf gates for edge: " << edge_no << ":");
                                for (auto &q : edge_detunes_qubits[edge_no]) {
                                    DOUT("sqf q" << q);
                                    custom_gate* g = new custom_gate("sqf q"+std::to_string(q));
                                    g->operands.push_back(q);

                                    ir::section_t asec;
                                    asec.push_back(g);
                                    bundles_dst_it->parallel_sections.push_back(asec);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    IOUT("Post scheduling decomposition [Done]");
}


} // namespace arch
} // namespace ql
