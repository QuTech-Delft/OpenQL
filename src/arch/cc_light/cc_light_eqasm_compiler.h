/**
 * @file   cc_light_eqasm_compiler.h
 * @date   08/2017
 * @author Imran Ashraf
 *         Nader Khammassi
 * @brief  cclighteqasm compiler implementation
 */

#pragma once

#include <utils.h>
#include <platform.h>
#include <kernel.h>
#include <gate.h>
#include <ir.h>
#include <eqasm_compiler.h>
#include <arch/cc_light/cc_light_eqasm.h>
#include <scheduler.h>
#include <mapper.h>
#include <clifford.h>
#include <latency_compensation.h>
#include <buffer_insertion.h>
#include <qsoverlay.h>

namespace ql {
namespace arch {

// eqasm code : set of cc_light_eqasm instructions
typedef std::vector<cc_light_eqasm_instr_t> eqasm_t;

typedef std::vector<size_t>        qubit_set_t;
typedef std::pair<size_t,size_t>   qubit_pair_t;
typedef std::vector<qubit_pair_t>  qubit_pair_set_t;

const size_t MAX_S_REG = 32;
const size_t MAX_T_REG = 64;

OPENQL_DECLSPEC extern size_t CurrSRegCount;
OPENQL_DECLSPEC extern size_t CurrTRegCount;

class Mask {
public:
    size_t regNo = 0;
    std::string regName;
    qubit_set_t squbits;
    qubit_pair_set_t dqubits;

    Mask() = default;
    explicit Mask(const qubit_set_t &qs);
    Mask(const std::string &rn, const qubit_set_t &qs);
    explicit Mask(const qubit_pair_set_t &qps);
};

class MaskManager {
private:
    std::map<size_t,Mask> SReg2Mask;
    std::map<qubit_set_t,Mask> QS2Mask;

    std::map<size_t,Mask> TReg2Mask;
    std::map<qubit_pair_set_t,Mask> QPS2Mask;

public:
    MaskManager();
    ~MaskManager();
    size_t getRegNo(qubit_set_t &qs);
    size_t getRegNo(qubit_pair_set_t &qps);
    std::string getRegName(qubit_set_t &qs);
    std::string getRegName(qubit_pair_set_t &qps);
    std::string getMaskInstructions();
};

class classical_cc : public gate {
public:
    cmat_t m;
    // int imm_value;
    classical_cc(const std::string &operation, const std::vector<size_t> &opers, int ivalue = 0);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

std::string classical_instruction2qisa(classical_cc *classical_in);

// FIXME HvS cc_light_instr is name of attribute in json file, in gate: arch_operation_name, here in instruction_map?
// FIXME HvS attribute of gate or just in json? Generalization to arch_operation_name is unnecessary
std::string get_cc_light_instruction_name(const std::string &id, const quantum_platform &platform);

std::string ir2qisa(quantum_kernel &kernel, const quantum_platform &platform, MaskManager &gMaskManager);

/**
 * cclight eqasm compiler
 */
class cc_light_eqasm_compiler : public eqasm_compiler {
public:

    cc_light_eqasm_program_t cc_light_eqasm_instructions;
    size_t total_exec_time = 0;

public:

    // FIXME: should be private
    static std::string get_qisa_prologue(const quantum_kernel &k);
    static std::string get_qisa_epilogue(const quantum_kernel &k);

    void ccl_decompose_pre_schedule(quantum_program *programp, const quantum_platform &platform, const std::string &passname);
    void ccl_decompose_post_schedule(quantum_program *programp, const quantum_platform &platform, const std::string &passname);
    static void ccl_decompose_post_schedule_bundles(ir::bundles_t &bundles_dst, const quantum_platform &platform);

    void map(quantum_program* programp, const ql::quantum_platform& platform, std::string passname, std::string* mapStatistics)
    {
        auto mapopt = ql::options::get("mapper");
        if (mapopt == "no" )
        {
            IOUT("Not mapping kernels");
            return;
        }

        ql::report_statistics(programp, platform, "in", passname, "# ");
        ql::report_qasm(programp, platform, "in", passname);

        Mapper mapper;  // virgin mapper creation; for role of Init functions, see comment at top of mapper.h
        mapper.Init(&platform); // platform specifies number of real qubits, i.e. locations for virtual qubits

        std::ofstream   ofs;
        ofs = ql::report_open(programp, "out", passname);

        size_t  total_swaps = 0;        // for reporting, data is mapper specific
        size_t  total_moves = 0;        // for reporting, data is mapper specific
        double  total_timetaken = 0.0;  // total over kernels of time taken by mapper
        for(auto &kernel : programp->kernels)
        {
            IOUT("Mapping kernel: " << kernel.name);

            // compute timetaken, start interval timer here
            double    timetaken = 0.0;
            using namespace std::chrono;
            high_resolution_clock::time_point t1 = high_resolution_clock::now();

            mapper.Map(kernel);
            // kernel.qubit_count starts off as number of virtual qubits, i.e. highest indexed qubit minus 1
            // kernel.qubit_count is updated by Map to highest index of real qubits used minus -1
            programp->qubit_count = platform.qubit_number;
            // program.qubit_count is updated to platform.qubit_number

            // computing timetaken, stop interval timer
            high_resolution_clock::time_point t2 = high_resolution_clock::now();
            duration<double> time_span = t2 - t1;
            timetaken = time_span.count();

            ql::report_kernel_statistics(ofs, kernel, platform, "# ");
            std::stringstream ss;
            ss << "# ----- swaps added: " << mapper.nswapsadded << "\n";
            ss << "# ----- of which moves added: " << mapper.nmovesadded << "\n";
            ss << "# ----- virt2real map before mapper:" << ql::utils::to_string(mapper.v2r_in) << "\n";
            ss << "# ----- virt2real map after initial placement:" << ql::utils::to_string(mapper.v2r_ip) << "\n";
            ss << "# ----- virt2real map after mapper:" << ql::utils::to_string(mapper.v2r_out) << "\n";
            ss << "# ----- realqubit states before mapper:" << ql::utils::to_string(mapper.rs_in) << "\n";
            ss << "# ----- realqubit states after mapper:" << ql::utils::to_string(mapper.rs_out) << "\n";
            ss << "# ----- time taken: " << timetaken << "\n";
            ql::report_string(ofs, ss.str());

            total_swaps += mapper.nswapsadded;
            total_moves += mapper.nmovesadded;
            total_timetaken += timetaken;

            ql::get_kernel_statistics(mapStatistics, kernel, platform, "# ");
            *mapStatistics += ss.str();
        }
        ql::report_totals_statistics(ofs, programp->kernels, platform, "# ");
        std::stringstream ss;
        ss << "# Total no. of swaps: " << total_swaps << "\n";
        ss << "# Total no. of moves of swaps: " << total_moves << "\n";
        ss << "# Total time taken: " << total_timetaken << "\n";
        ql::report_string(ofs, ss.str());
        ql::report_close(ofs);

        ql::report_qasm(programp, platform, "out", passname);


        // add total statistics
        ql::get_totals_statistics(mapStatistics, programp->kernels, platform, "# ");
        *mapStatistics += ss.str();
    }

    // cc_light_instr is needed by some cc_light backend passes and by cc_light resource_management:
    // - each bundle section will only have gates with the same cc_light_instr name; prepares for SIMD/SOMQ
    // - in resource management with VSMs, gates with same cc_light_instr can use same QWG in parallel
    // arch_operation_name is attempt to generalize this but is only in custom gate;
    //   so using default gates in a context where arch_operation_name is needed, would crash (e.g. wait gate)
    // it depends on that a primitive gate is one-to-one with a qisa instruction;
    //   this is something done by design now but perhaps not future-proof, e.g. towards an other backend for e.g. spin qubits
    //
    // FIXME HvS this mess must be cleaned up; so I didn't touch it further
    //
    // perhaps can be replaced by semantic definition (e.g. x90 :=: ( type=ROTATION axis=X angle=90 ) )
    // and check on equality of these instead
    // but what if there are two x90s, with different physical attributes (e.g. different amplitudes?)? Does this happen?

    void ccl_prep_code_generation(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        const json& instruction_settings = platform.instruction_settings;
        for(const json & i : instruction_settings)
        {
            if(i.count("cc_light_instr") <= 0)
            {
                FATAL("cc_light_instr not found for " << i);
            }
        }
    }

    // unified entry for quantumsim script writing
    // will be moved to dqcsim eventually, which must read cqasm with cycle information; is that sufficient?
    void write_quantumsim_script(quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        ql::report_statistics(programp, platform, "in", passname, "# ");
        ql::report_qasm(programp, platform, "in", passname);

        // for backward compatibility, use passname to distinguish between calls from different places
        bool compiled;
        std::string suffix;
        if (passname == "write_quantumsim_script_unmapped")
        {
            compiled = false;
            suffix = "";
        }
        else if (passname == "write_quantumsim_script_mapped")
        {
            compiled = true;
            suffix = "mapped";
        }
        else
        {
            FATAL("Write_quantumsim_script: unknown passname: " << passname);
        }

        // dqcsim must take over
        if (ql::options::get("quantumsim") == "yes")
            write_quantumsim_program(programp, platform.qubit_number, platform, suffix);
        else if (ql::options::get("quantumsim") == "qsoverlay")
            write_qsoverlay_program(programp, platform.qubit_number, platform, suffix, platform.cycle_time, compiled);

        ql::report_statistics(programp, platform, "out", passname, "# ");
        ql::report_qasm(programp, platform, "out", passname);
    }

    /*
     * program-level compilation of qasm to cc_light_eqasm
     */
    void compile(std::string prog_name, ql::circuit& ckt, ql::quantum_platform& platform)
    {
        FATAL("cc_light_eqasm_compiler::compile interface with circuit not supported");
    }

    // kernel level compilation
    void compile(quantum_program* programp, const ql::quantum_platform& platform)
    {
//std::cout << " ============= DEBUG PRINT FOR DEBUG(1): In cc_light BACKEND COMPILER \n";
        DOUT("Compiling " << programp->kernels.size() << " kernels to generate CCLight eQASM ... ");

        // overall timing should be done by the pass manager
        // can be deleted here when so
        //
        // each pass can also have a local timer;
        // can also be done by pass manager in parallel to skip option
        //
        // compute timetaken, start interval timer here
        double    total_timetaken = 0.0;
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

        ql::clifford_optimize(programp, platform, "clifford_premapper");

        // map function definition must be moved to src/mapper.h and src/mapper.cc
        // splitting src/mapper.h into src/mapper.h and src/mapper.cc is intricate
        // because mapper shares ddg code with scheduler
        // this implies that those latter interfaces must be made public in scheduler.h before splitting
        // scheduler.h and mapper.h
        std::string emptystring = "";
        map(programp, platform, "mapper", &emptystring);

        ql::clifford_optimize(programp, platform, "clifford_postmapper");

        ql::rcschedule(programp, platform, "rcscheduler");

        ql::latency_compensation(programp, platform, "ccl_latency_compensation");

        ql::insert_buffer_delays(programp, platform, "ccl_insert_buffer_delays");

        // decompose meta-instructions after scheduling
        ccl_decompose_post_schedule(programp, platform, "ccl_decompose_post_schedule");

        // just before code generation, emit quantumsim script to best match target architecture
        write_quantumsim_script(programp, platform, "write_quantumsim_script_mapped");

        // and now for real
        qisa_code_generation(programp, platform, "qisa_code_generation");

        // timing to be moved to pass manager
        // computing timetaken, stop interval timer
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = t2 - t1;
        total_timetaken = time_span.count();

        // reporting to be moved to write_statistics pass
        // report totals over all kernels, over all eqasm passes contributing to mapping
        std::ofstream   ofs;
        ofs = ql::report_open(programp, "out", "cc_light_compiler");
        for (auto& k : programp->kernels) { ql::report_kernel_statistics(ofs, k, platform, "# "); }
        ql::report_totals_statistics(ofs, programp->kernels, platform, "# ");
        std::stringstream ss;
        ss << "# Total time taken: " << total_timetaken << "\n";
        ql::report_string(ofs, ss.str());
        ql::report_close(ofs);
        ql::report_qasm(programp, platform, "out", "cc_light_compiler");

        DOUT("Compiling CCLight eQASM [Done]");
    }

    /**
     * decompose
     */
    // decompose meta-instructions
    void ccl_decompose_pre_schedule_kernel(ql::quantum_kernel& kernel, const ql::quantum_platform & platform)
    {
        IOUT("Decomposing kernel: " << kernel.name);
        if (kernel.c.empty())
        {
            return;
        }
        ql::circuit decomp_ckt;	// collect result circuit in here and before return swap with kernel.c

        DOUT("decomposing instructions...");
        for( auto ins : kernel.c )
        {
            auto iname = utils::to_lower(ins->name);
            DOUT("decomposing instruction " << iname << "...");
            auto & icopers = ins->creg_operands;
            auto & iqopers = ins->operands;
            int icopers_count = icopers.size();
            int iqopers_count = iqopers.size();
            DOUT("decomposing instruction " << iname << " operands=" << ql::utils::to_string(iqopers) << " creg_operands=" << ql::utils::to_string(icopers));
            auto itype = ins->type();
            if(__classical_gate__ == itype)
            {
                DOUT("    classical instruction: " << ins->qasm());

                if( (iname == "add") || (iname == "sub") ||
                    (iname == "and") || (iname == "or") || (iname == "xor") ||
                    (iname == "not") || (iname == "nop")
                    )
                {
                    // decomp_ckt.push_back(ins);
                    decomp_ckt.push_back(new ql::arch::classical_cc(iname, icopers));
                    DOUT("    classical instruction decomposed: " << decomp_ckt.back()->qasm());
                }
                else if( (iname == "eq") || (iname == "ne") || (iname == "lt") ||
                         (iname == "gt") || (iname == "le") || (iname == "ge")
                    )
                {
                    decomp_ckt.push_back(new ql::arch::classical_cc("cmp", {icopers[1], icopers[2]}));
                    DOUT("    classical instruction decomposed: " << decomp_ckt.back()->qasm());
                    decomp_ckt.push_back(new ql::arch::classical_cc("nop", {}));
                    DOUT("                                      " << decomp_ckt.back()->qasm());
                    decomp_ckt.push_back(new ql::arch::classical_cc("fbr_"+iname, {icopers[0]}));
                    DOUT("                                      " << decomp_ckt.back()->qasm());
                }
                else if(iname == "mov")
                {
                    // r28 is used as temp, TODO use creg properly to create temporary
                    decomp_ckt.push_back(new ql::arch::classical_cc("ldi", {28}, 0));
                    DOUT("    classical instruction decomposed: " << decomp_ckt.back()->qasm());
                    decomp_ckt.push_back(new ql::arch::classical_cc("add", {icopers[0], icopers[1], 28}));
                    DOUT("                                      " << decomp_ckt.back()->qasm());
                }
                else if(iname == "ldi")
                {
                    // auto imval = ((classical_cc*)ins)->int_operand;
                    auto imval = ((classical*)ins)->int_operand;
                    DOUT("    classical instruction decomposed: imval=" << imval);
                    decomp_ckt.push_back(new ql::arch::classical_cc("ldi", {icopers[0]}, imval));
                    DOUT("    classical instruction decomposed: " << decomp_ckt.back()->qasm());
                }
                else
                {
                    EOUT("Unknown decomposition of classical operation '" << iname << "' with '" << icopers_count << "' operands!");
                    throw ql::exception("Unknown classical operation '"+iname+"' with'"+std::to_string(icopers_count)+"' operands!", false);
                }
            }
            else
            {
                if(iname == "wait")
                {
                    DOUT("    wait instruction ");
                    decomp_ckt.push_back(ins);
                }
                else
                {
                    const json& instruction_settings = platform.instruction_settings;
                    std::string operation_type;
                    if (instruction_settings.find(iname) != instruction_settings.end())
                    {
                        operation_type = instruction_settings[iname]["type"].get<std::string>();
                    }
                    else
                    {
                        EOUT("instruction settings not found for '" << iname << "' with '" << iqopers_count << "' operands!");
                        throw ql::exception("instruction settings not found for '"+iname+"' with'"+std::to_string(iqopers_count)+"' operands!", false);
                    }
                    bool is_measure = (operation_type == "readout");
                    if(is_measure)
                    {
                        // insert measure
                        DOUT("    readout instruction ");
                        auto qop = iqopers[0];
                        decomp_ckt.push_back(ins);
                        if( ql::gate_type_t::__custom_gate__ == itype )
                        {
                            auto & coperands = ins->creg_operands;
                            if(!coperands.empty())
                            {
                                auto cop = coperands[0];
                                decomp_ckt.push_back(new ql::arch::classical_cc("fmr", {cop, qop}));
                            }
                            else
                            {
                                // WOUT("Unknown classical operand for measure/readout operation: '" << iname <<
                                //     ". This will soon be depricated in favour of measure instruction with fmr" <<
                                //     " to store measurement outcome to classical register.");
                            }
                        }
                        else
                        {
                            EOUT("Unknown decomposition of measure/readout operation: '" << iname << "!");
                            throw ql::exception("Unknown decomposition of measure/readout operation '"+iname+"'!", false);
                        }
                    }
                    else
                    {
                        DOUT("    quantum instruction ");
                        decomp_ckt.push_back(ins);
                    }
                }
            }
        }
        kernel.c = decomp_ckt;;

        DOUT("decomposing instructions...[Done]");
    }

    // qisa_code_generation pass
    // generates qisa from IR
    void qisa_code_generation(quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        MaskManager mask_manager;
        std::stringstream ssqisa, sskernels_qisa;
        sskernels_qisa << "start:" << std::endl;
        for(auto &kernel : programp->kernels)
        {
            sskernels_qisa << "\n" << kernel.name << ":" << std::endl;
            sskernels_qisa << get_qisa_prologue(kernel);
            if (! kernel.c.empty())
            {
                sskernels_qisa << ir2qisa(kernel, platform, mask_manager);
            }
            sskernels_qisa << get_qisa_epilogue(kernel);
        }
        sskernels_qisa << "\n    br always, start" << "\n"
                       << "    nop \n"
                       << "    nop" << std::endl;
        ssqisa << mask_manager.getMaskInstructions() << sskernels_qisa.str();
        // std::cout << ssqisa.str();

        // write cc-light qisa file
        std::ofstream fout;
        std::string unique_name = programp->unique_name;
        std::string qisafname( ql::options::get("output_dir") + "/" + unique_name + ".qisa");
        IOUT("Writing CC-Light QISA to " << qisafname);
        fout.open( qisafname, std::ios::binary);
        if ( fout.fail() )
        {
            EOUT("opening file " << qisafname << std::endl
                                 << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }
        fout << ssqisa.str() << std::endl;
        fout.close();
        // end qisa_generation pass
    }

private:
    // write cc_light scheduled bundles for quantumsim
    // when cc_light independent, it should be extracted and put in src/quantumsim.h
    void write_quantumsim_program( quantum_program* programp, size_t num_qubits, const ql::quantum_platform & platform, std::string suffix)
    {
        IOUT("Writing scheduled Quantumsim program");
        std::ofstream fout;
        std::string qfname( ql::options::get("output_dir") + "/" + "quantumsim_" + programp->unique_name + "_" + suffix + ".py");
        DOUT("Writing scheduled Quantumsim program to " << qfname);
        IOUT("Writing scheduled Quantumsim program to " << qfname);
        fout.open( qfname, std::ios::binary);
        if ( fout.fail() )
        {
            EOUT("opening file " << qfname << std::endl
                                 << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }

        fout << "# Quantumsim program generated OpenQL\n"
             << "# Please modify at your will to obtain extra information from Quantumsim\n\n";

        fout << "import numpy as np\n"
             << "from quantumsim.circuit import Circuit\n"
             << "from quantumsim.circuit import uniform_noisy_sampler\n"
             << "from quantumsim.circuit import ButterflyGate\n"
             << std::endl;

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
             << std::endl;

        fout << "\n# create a circuit\n";
        fout << "def circuit_generated(t1=np.inf, t2=np.inf, dephasing_axis=None, dephasing_angle=None, dephase_var=0, readout_error=0.0) :\n";
        fout << "    c = Circuit(title=\"" << programp->unique_name << "\")\n";

        DOUT("Adding qubits to Quantumsim program");
        fout << "\n    # add qubits\n";
        json config;
        try
        {
            config = load_json(platform.configuration_file_name);
        }
        catch (json::exception e)
        {
            throw ql::exception("[x] error : ql::quantumsim_compiler::load() :  failed to load the hardware config file : malformed json file ! : \n    "+
                                std::string(e.what()),false);
        }

        // load qubit attributes
        json qubit_attributes = config["qubit_attributes"];
        if (qubit_attributes.is_null())
        {
            EOUT("qubit_attributes is not specified in the hardware config file !");
            throw ql::exception("[x] error: quantumsim_compiler: qubit_attributes is not specified in the hardware config file !",false);
        }
        json relaxation_times = qubit_attributes["relaxation_times"];
        if (relaxation_times.is_null())
        {
            EOUT("relaxation_times is not specified in the hardware config file !");
            throw ql::exception("[x] error: quantumsim_compiler: relaxation_times is not specified in the hardware config file !",false);
        }
        size_t count =  platform.hardware_settings["qubit_number"];

        // want to ignore unused qubits below
        ASSERT (programp->kernels.size() <= 1);
        std::vector<size_t> check_usecount;
        check_usecount.resize(count, 0);

        for (auto & gp: programp->kernels.front().c)
        {
            switch(gp->type())
            {
                case __classical_gate__:
                case __wait_gate__:
                    break;
                default:    // quantum gate
                    for (auto v: gp->operands)
                    {
                        check_usecount[v]++;
                    }
                    break;
            }
        }

        for (json::iterator it = relaxation_times.begin(); it != relaxation_times.end(); ++it)
        {
            size_t q = stoi(it.key());
            if (q >= count)
            {
                EOUT("qubit_attribute.relaxation_time.qubit number is not in qubits available in the platform");
                throw ql::exception("[x] error: qubit_attribute.relaxation_time.qubit number is not in qubits available in the platform",false);
            }
            if (check_usecount[q] == 0)
            {
                DOUT("... qubit " << q << " is not used; skipping it");
                continue;
            }
            auto & rt = it.value();
            if (rt.size() < 2)
            {
                EOUT("each qubit must have at least two relaxation times");
                throw ql::exception("[x] error: each qubit must have at least two relaxation times",false);
            }
            // fout << "    c.add_qubit(\"q" << q <<"\", " << rt[0] << ", " << rt[1] << ")\n" ;
            fout << "    c.add_qubit(\"q" << q << "\", t1=t1, t2=t2)\n" ;
        }

        DOUT("Adding Gates to Quantumsim program");
        {
            // global writes
            std::stringstream ssqs;
            ssqs << "\n    sampler = uniform_noisy_sampler(readout_error=readout_error, seed=42)\n";
            ssqs << "\n    # add gates\n";
            fout << ssqs.str();
        }
        for(auto &kernel : programp->kernels)
        {
            DOUT("... adding gates, a new kernel");
            ASSERT(kernel.cycles_valid);
            ql::ir::bundles_t bundles = ql::ir::bundler(kernel.c, platform.cycle_time);

            if (bundles.empty())
            {
                IOUT("No bundles for adding gates");
            }
            else
            {
                for ( ql::ir::bundle_t & abundle : bundles)
                {
                    DOUT("... adding gates, a new bundle");
                    auto bcycle = abundle.start_cycle;

                    std::stringstream ssqs;
                    for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
                    {
                        DOUT("... adding gates, a new section in a bundle");
                        for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                        {
                            auto & iname = (*insIt)->name;
                            auto & operands = (*insIt)->operands;
                            auto duration = (*insIt)->duration;     // duration in nano-seconds
                            // size_t operation_duration = std::ceil( static_cast<float>(duration) / platform.cycle_time);
                            if( iname == "measure")
                            {
                                DOUT("... adding gates, a measure");
                                auto op = operands.back();
                                ssqs << "    c.add_qubit(\"m" << op << "\")\n";
                                ssqs << "    c.add_gate("
                                     << "ButterflyGate("
                                     << "\"q" << op <<"\", "
                                     << "time=" << ((bcycle-1)*platform.cycle_time) << ", "
                                     << "p_exc=0,"
                                     << "p_dec= 0.005)"
                                     << ")\n" ;
                                ssqs << "    c.add_measurement("
                                     << "\"q" << op << "\", "
                                     << "time=" << ((bcycle - 1)*platform.cycle_time) + (duration/4) << ", "
                                     << "output_bit=\"m" << op << "\", "
                                     << "sampler=sampler"
                                     << ")\n";
                                ssqs << "    c.add_gate("
                                     << "ButterflyGate("
                                     << "\"q" << op << "\", "
                                     << "time=" << ((bcycle - 1)*platform.cycle_time) + duration/2 << ", "
                                     << "p_exc=0,"
                                     << "p_dec= 0.015)"
                                     << ")\n";

                            }
                            else if( iname == "y90" || iname == "ym90" || iname == "y" || iname == "x" ||
                                     iname == "x90" || iname == "xm90")
                            {
                                DOUT("... adding gates, another gate");
                                ssqs <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssqs << "\"q" << *opit <<"\", ";
                                    ssqs << "\"q" << operands.back()<<"\"";
                                }
                                ssqs << ", time=" << ((bcycle - 1)*platform.cycle_time) + (duration/2) << ", dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle))" << std::endl;
                            }
                            else if( iname == "cz")
                            {
                                DOUT("... adding gates, another gate");
                                ssqs <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssqs << "\"q" << *opit <<"\", ";
                                    ssqs << "\"q" << operands.back()<<"\"";
                                }
                                ssqs << ", time=" << ((bcycle - 1)*platform.cycle_time) + (duration/2) << ", dephase_var=dephase_var))" << std::endl;
                            }
                            else
                            {
                                DOUT("... adding gates, another gate");
                                ssqs <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssqs << "\"q" << *opit <<"\", ";
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
                ql::report_kernel_statistics(fout, kernel, platform, "    # ");
            }
        }
        ql::report_string(fout, "    \n");
        ql::report_string(fout, "    # Program-wide statistics:\n");
        ql::report_totals_statistics(fout, programp->kernels, platform, "    # ");
        fout << "    return c";

        fout.close();
        IOUT("Writing scheduled Quantumsim program [Done]");
    }

};
} // namespace arch
} // namespace ql
