/**
 * @file   cc_light_eqasm_compiler.h
 * @date   08/2017
 * @author Imran Ashraf, Nader Khammassi
 * @brief  cclighteqasm compiler implementation
 */

#ifndef QL_CC_LIGHT_EQASM_COMPILER_H
#define QL_CC_LIGHT_EQASM_COMPILER_H

#include <ql/platform.h>
#include <ql/kernel.h>
#include <ql/ir.h>
#include <ql/eqasm_compiler.h>
#include <ql/arch/cc_light_eqasm.h>

#include <ql/arch/cc_light_scheduler.h>

// eqasm code : set of cc_light_eqasm instructions
typedef std::vector<ql::arch::cc_light_eqasm_instr_t> eqasm_t;

namespace ql
{
namespace arch
{
/**
 * cclight eqasm compiler
 */
class cc_light_eqasm_compiler : public eqasm_compiler
{
public:

    cc_light_eqasm_program_t cc_light_eqasm_instructions;
    size_t          num_qubits;
    size_t          ns_per_cycle;
    size_t          total_exec_time = 0;
    size_t          buffer_matrix[__operation_types_num__][__operation_types_num__];

#define __ns_to_cycle(t) ((size_t)t/(size_t)ns_per_cycle)

public:


    /*
     * compile qasm to cc_light_eqasm
     */
    void compile(std::string prog_name, ql::circuit& ckt, ql::quantum_platform& platform) throw (ql::exception)
    {
        IOUT("[-] compiling qasm code ...");
        if (ckt.empty())
        {
            EOUT("empty circuit, eqasm compilation aborted !");
            return;
        }
        IOUT("[-] loading circuit (" <<  ckt.size() << " gates)...");

        load_hw_settings(platform);

        generate_opcode_cs_files(platform);


        // schedule
        // ql::ir sched_ir = cc_light_schedule(ckt, platform, num_qubits);

        // schedule with platform resource constraints
        ql::ir::bundles_t bundles = cc_light_schedule_rc(ckt, platform, num_qubits);

        // write RC scheduled bundles with parallelism as simple QASM file
        std::stringstream sched_qasm;
        sched_qasm <<"qubits " << num_qubits << "\n\n"
                   << ".fused_kernels";
        string fname( ql::options::get("output_dir") + "/" + prog_name + "_scheduled_rc.qasm");
        IOUT("Writing Recourse-contraint scheduled CC-Light QASM to " << fname);
        sched_qasm << ql::ir::qasm(bundles);
        ql::utils::write_file(fname, sched_qasm.str());

        MaskManager mask_manager;
        // write scheduled bundles with parallelism in cc-light syntax
        WriteCCLightQisa(prog_name, platform, mask_manager, bundles);

        // write scheduled bundles with parallelism in cc-light syntax with time-stamps
        WriteCCLightQisaTimeStamped(prog_name, platform, mask_manager, bundles);




        // time analysis
        // total_exec_time = time_analysis();

        // compensate for latencies
        // compensate_latency();

        // reschedule
        // resechedule();

        // dump_instructions();

        // decompose meta-instructions
        // decompose_instructions();

        // reorder instructions
        // reorder_instructions();

        // insert waits

        emit_eqasm();
    }

    void compile(std::vector<quantum_kernel> kernels, ql::quantum_platform& platform)
    {
        DOUT("Compiling " << kernels.size() << " kernels to generate CCLight eQASM ... ");

        load_hw_settings(platform);
        generate_opcode_cs_files(platform);
        MaskManager mask_manager;

        std::stringstream ssqisa, sskernels_qisa;
        sskernels_qisa << "start:" << std::endl;
        for(auto &kernel : kernels)
        {
            IOUT("Compiling kernel: " << kernel.name);
            ql::circuit& ckt = kernel.c;
            if (! ckt.empty())
            {
                // schedule with platform resource constraints
                ql::ir::bundles_t bundles = cc_light_schedule_rc(ckt, platform, num_qubits);

                // std::cout << "QASM" << std::endl;
                // std::cout << ql::ir::qasm(bundles) << std::endl;
                sskernels_qisa << "\n" << kernel.name << ":" << std::endl;
                sskernels_qisa << bundles2qisa(bundles, platform, mask_manager);
            }
        }

        sskernels_qisa << "\n    br always, start" << "\n"
                  << "    nop \n"
                  << "    nop" << std::endl;

        ssqisa << mask_manager.getMaskInstructions() << sskernels_qisa.str();
        std::cout << ssqisa.str();

        DOUT("Compiling CCLight eQASM [Done]");
    }

    /**
     * display instruction and start time
     */
    void dump_instructions()
    {
        println("[d] instructions dump:");
        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
        {
            size_t t = instr->start;
            std::cout << t << " : " << instr->code() << std::endl;
        }
    }


    /**
     * decompose
     */
    void decompose_instructions(bool verbose=false)
    {
        /*
        IOUT("decomposing instructions...");
        cc_light_eqasm_program_t decomposed;
        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
        {
        cc_light_eqasm_program_t dec = instr->decompose();
          for (cc_light_eqasm_instruction * i : dec)
             decomposed.push_back(i);
            }
            cc_light_eqasm_instructions.swap(decomposed);
            */
    }


    /**
     * reorder instructions
     */
    void reorder_instructions()
    {
        // IOUT("reodering instructions...");
        // std::sort(cc_light_eqasm_instructions.begin(),cc_light_eqasm_instructions.end(), cc_light_eqasm_comparator);
    }

    /**
     * time analysis
     */
    size_t time_analysis(bool verbose=false)
    {
        IOUT("time analysis...");
        // update start time : find biggest latency
        size_t max_latency = 0;
        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
        {
            size_t l = instr->latency;
            max_latency = (l > max_latency ? l : max_latency);
        }
        // set refrence time to max latency (avoid negative reference)
        size_t time = max_latency; // 0;
        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
        {
            // println(time << ":");
            // println(instr->code());
            // instr->start = time;
            instr->set_start(time);
            time        += instr->duration; //+1;
        }
        return time;
    }



    /**
     * compensate for latencies
     */
    void compensate_latency(bool verbose=false)
    {
        IOUT("latency compensation...");
        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
            instr->compensate_latency();
    }

    /**
     * optimize
     */
    void resechedule(bool verbose=false)
    {
        IOUT("instruction rescheduling...");
        IOUT("resource dependency analysis...");
        IOUT("buffer insertion...");
#if 0
        std::vector<size_t>           hw_res_av(__trigger_width__+__awg_number__,0);
        std::vector<size_t>           qu_res_av(num_qubits,0);
        std::vector<operation_type_t> hw_res_op(__trigger_width__+__awg_number__,__none__);
        std::vector<operation_type_t> qu_res_op(num_qubits,__none__);

        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
        {
            resources_t      hw_res  = instr->used_resources;
            qubit_set_t      qu_res  = instr->used_qubits;
            operation_type_t type    = instr->get_operation_type();
            size_t latest_hw = 0;
            size_t buf_hw    = 0;
            size_t latest_qu = 0;
            size_t buf_qu    = 0;
            // hardware dependency
            for (size_t r=0; r<hw_res.size(); ++r)
            {
                if (hw_res.test(r))
                {
                    size_t rbuf = buffer_size(hw_res_op[r],type);
                    buf_hw      = ((rbuf > buf_hw) ? rbuf : buf_hw);
                    latest_hw   = (hw_res_av[r] > latest_hw ? hw_res_av[r] : latest_hw);
                }
            }

            // qubit dependency
            for (size_t q : qu_res) // qubits used by the instr
            {
                size_t rbuf  = buffer_size(qu_res_op[q],type);
                buf_qu       = ((rbuf > buf_qu) ? rbuf : buf_qu);
                latest_qu    = (qu_res_av[q] > latest_qu ? qu_res_av[q] : latest_qu);
            }

            // println("latest_hw: " << latest_hw);
            // println("latest_qu: " << latest_qu);

            size_t latest = std::max(latest_hw,latest_qu);
            size_t buf    = std::max(buf_hw,buf_qu);

            //if (buf)
            // println("[!] inserting buffer...");

            instr->start = latest+buf;
            // update latest hw record
            for (size_t r=0; r<hw_res.size(); ++r)
            {
                if (hw_res.test(r))
                {
                    hw_res_av[r] = (instr->start+instr->duration);
                    hw_res_op[r] = (type);
                }
            }
            // update latest hw record
            for (size_t q : qu_res) // qubits used by the instr
            {
                qu_res_av[q] = (instr->start+instr->duration);
                qu_res_op[q] = (type);
            }
        }
#endif
    }

    /**
     * buffer size
     */
    size_t buffer_size(operation_type_t t1, operation_type_t t2)
    {
        return buffer_matrix[t1][t2];
    }

    /**
     * dump traces
     */
    void write_traces(std::string file_name="")
    {
#if 0
        ql::arch::channels_t channels;
        if (cc_light_eqasm_instructions.empty())
        {
            println("[!] warning : empty cc_light_eqasm code : not traces to dump !");
            return;
        }

        for (size_t i=0; i<__trigger_width__; i++)
        {
            std::string ch = "TRIG_"+std::to_string(i);
            channels.push_back(ch);
        }

        for (size_t i=0; i<__awg_number__; i++)
        {
            std::string ch = "AWG_"+std::to_string(i);
            channels.push_back(ch);
        }

        ql::arch::time_diagram diagram(channels,total_exec_time,4);

        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
        {
            instruction_traces_t trs = instr->trace();
            for (instruction_trace_t t : trs)
                diagram.add_trace(t);
        }

        diagram.dump(ql::options::get("output_dir") + "/trace.dat");
#endif
    }


private:

    void load_hw_settings(ql::quantum_platform& platform)
    {
        std::string params[] = { "qubit_number", "cycle_time", "mw_mw_buffer", "mw_flux_buffer", "mw_readout_buffer", "flux_mw_buffer",
                                 "flux_flux_buffer", "flux_readout_buffer", "readout_mw_buffer", "readout_flux_buffer", "readout_readout_buffer"
                               };
        size_t p = 0;

        DOUT("Loading hardware settings ...");
        try
        {
            num_qubits                                      = platform.hardware_settings[params[p++]];
            ns_per_cycle                                    = platform.hardware_settings[params[p++]];

            buffer_matrix[__rf__][__rf__]                   = __ns_to_cycle(platform.hardware_settings[params[p++]]);
            buffer_matrix[__rf__][__flux__]                 = __ns_to_cycle(platform.hardware_settings[params[p++]]);
            buffer_matrix[__rf__][__measurement__]          = __ns_to_cycle(platform.hardware_settings[params[p++]]);
            buffer_matrix[__flux__][__rf__]                 = __ns_to_cycle(platform.hardware_settings[params[p++]]);
            buffer_matrix[__flux__][__flux__]               = __ns_to_cycle(platform.hardware_settings[params[p++]]);
            buffer_matrix[__flux__][__measurement__]        = __ns_to_cycle(platform.hardware_settings[params[p++]]);
            buffer_matrix[__measurement__][__rf__]          = __ns_to_cycle(platform.hardware_settings[params[p++]]);
            buffer_matrix[__measurement__][__flux__]        = __ns_to_cycle(platform.hardware_settings[params[p++]]);
            buffer_matrix[__measurement__][__measurement__] = __ns_to_cycle(platform.hardware_settings[params[p++]]);
        }
        catch (json::exception e)
        {
            throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter '"+params[p-1]+"'\n\t"+ std::string(e.what()),false);
        }
    }

    void generate_opcode_cs_files(ql::quantum_platform& platform)
    {
        DOUT("Generating opcode file ...");
        json& instruction_settings       = platform.instruction_settings;

        std::stringstream opcode_ss;

        opcode_ss << "# Classic instructions (single instruction format)\n";
        opcode_ss << "def_opcode[\"nop\"]      = 0x00\n";
        opcode_ss << "def_opcode[\"br\"]       = 0x01\n";
        opcode_ss << "def_opcode[\"stop\"]     = 0x08\n";
        opcode_ss << "def_opcode[\"cmp\"]      = 0x0d\n";
        opcode_ss << "def_opcode[\"ldi\"]      = 0x16\n";
        opcode_ss << "def_opcode[\"ldui\"]     = 0x17\n";
        opcode_ss << "def_opcode[\"or\"]       = 0x18\n";
        opcode_ss << "def_opcode[\"xor\"]      = 0x19\n";
        opcode_ss << "def_opcode[\"and\"]      = 0x1a\n";
        opcode_ss << "def_opcode[\"not\"]      = 0x1b\n";
        opcode_ss << "def_opcode[\"add\"]      = 0x1e\n";
        opcode_ss << "def_opcode[\"sub\"]      = 0x1f\n";
        opcode_ss << "# quantum-classical mixed instructions (single instruction format)\n";
        opcode_ss << "def_opcode[\"fbr\"]      = 0x14\n";
        opcode_ss << "def_opcode[\"fmr\"]      = 0x15\n";
        opcode_ss << "# quantum instructions (single instruction format)\n";
        opcode_ss << "def_opcode[\"smis\"]     = 0x20\n";
        opcode_ss << "def_opcode[\"smit\"]     = 0x28\n";
        opcode_ss << "def_opcode[\"qwait\"]    = 0x30\n";
        opcode_ss << "def_opcode[\"qwaitr\"]   = 0x38\n";
        opcode_ss << "# quantum instructions (double instruction format)\n";
        opcode_ss << "# no arguments\n";
        opcode_ss << "def_q_arg_none[\"qnop\"] = 0x00\n";

        DOUT("Generating control store file ...");
        std::stringstream control_store;

        control_store << "         Condition  OpTypeLeft  CW_Left  OpTypeRight  CW_Right\n";
        control_store << "     0:      0          0          0          0           0    \n";

        std::map<std::string,size_t> instr_name_2_opcode;
        std::set<size_t> opcode_set;
        size_t opcode=0;
        for (json & i : instruction_settings)
        {
            std::string instr_name;
            if (i["cc_light_instr"].is_null())
            {
                std::cout << i << std::endl;                
                COUT("SHOULD NOT BE NULL, FIX IT Later"); // TODO Fix it why null entry is picked up
                continue;
            }
            else
            {
                instr_name = i["cc_light_instr"];
            }

            if (i["cc_light_opcode"].is_null())
                throw ql::exception("[x] error : ql::eqasm_compiler::compile() : missing opcode for instruction '"+instr_name,false);
            else
                opcode = i["cc_light_opcode"];


            auto mapit = instr_name_2_opcode.find(instr_name);
            if( mapit != instr_name_2_opcode.end() )
            {
                // found
                if( opcode != mapit->second )
                    throw ql::exception("[x] error : ql::eqasm_compiler::compile() : multiple opcodes for instruction '"+instr_name,false);
            }
            else
            {
                // not found
                instr_name_2_opcode[instr_name] = opcode;
            }

            if (i["cc_light_instr_type"] == "single_qubit_gate")
            {
                if (opcode_set.find(opcode) != opcode_set.end())
                    continue;

                // opcode range check
                if (i["type"] == "readout")
                {
                    if (opcode < 0x4 || opcode > 0x7)
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : invalid opcode for measure instruction '"+instr_name+"' : should be in [0x04..0x07] range : current opcode: "+std::to_string(opcode),false);
                }
                else if (opcode < 1 || opcode > 127)
                {
                    throw ql::exception("[x] error : ql::eqasm_compiler::compile() : invalid opcode for single qubit gate instruction '"+instr_name+"' : should be in [1..127] range : current opcode: "+std::to_string(opcode),false);
                }
                opcode_set.insert(opcode);
                size_t condition  = (i["cc_light_cond"].is_null() ? 0 : i["cc_light_cond"].get<size_t>());
                if (i["cc_light_instr"].is_null())
                    throw ql::exception("[x] error : ql::eqasm_compiler::compile() : 'cc_light_instr' attribute missing in gate definition (opcode: "+std::to_string(opcode),false);
                opcode_ss << "def_q_arg_st[" << i["cc_light_instr"] << "]\t= " << std::showbase << std::hex << opcode << "\n";
                auto optype     = (i["type"] == "mw" ? 1 : (i["type"] == "flux" ? 2 : ((i["type"] == "readout" ? 3 : 0))));
                auto codeword   = i["cc_light_codeword"];
                control_store << "     " << i["cc_light_opcode"] << ":     " << condition << "          " << optype << "          " << codeword << "          0          0\n";
            }
            else if (i["cc_light_instr_type"] == "two_qubit_gate")
            {
                size_t opcode     = i["cc_light_opcode"];
                if (opcode_set.find(opcode) != opcode_set.end())
                    continue;
                if (opcode < 127 || opcode > 255)
                    throw ql::exception("[x] error : ql::eqasm_compiler::compile() : invalid opcode for two qubits gate instruction '"+instr_name+"' : should be in [128..255] range : current opcode: "+std::to_string(opcode),false);
                opcode_set.insert(opcode);
                // size_t condition  = 0;
                size_t condition  = (i["cc_light_cond"].is_null() ? 0 : i["cc_light_cond"].get<size_t>());
                if (i["cc_light_instr"].is_null())
                    throw ql::exception("[x] error : ql::eqasm_compiler::compile() : 'cc_light_instr' attribute missing in gate definition (opcode: "+std::to_string(opcode),false);
                // opcode_ss << "def_opcode[" << i["cc_light_instr"] << "]\t= " << opcode << "\n";
                opcode_ss << "def_q_arg_tt[" << i["cc_light_instr"] << "]\t= " << std::showbase << std::hex << opcode << "\n";
                auto optype     = (i["type"] == "mw" ? 1 : (i["type"] == "flux" ? 2 : ((i["type"] == "readout" ? 3 : 0))));
                auto codeword_l = i["cc_light_left_codeword"];
                auto codeword_r = i["cc_light_right_codeword"];
                control_store << "     " << i["cc_light_opcode"] << ":     " << condition << "          " << optype << "          " << codeword_l << "          " << optype << "          " << codeword_r << "\n";
            }
            else
                throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : invalid 'cc_light_instr_type' for instruction !",false);
            // println("\n" << control_store.str());
            // println("\n" << opcode_ss.str());
        }

        std::string cs_filename = ql::options::get("output_dir") + "/cs.txt";
        IOUT("writing control store file to '" << cs_filename << "' ...");
        ql::utils::write_file(cs_filename, control_store.str());

        std::string im_filename = ql::options::get("output_dir") + "/qisa_opcodes.qmap";
        IOUT("writing qisa instruction file to '" << im_filename << "' ...");        
        ql::utils::write_file(im_filename, opcode_ss.str());
    }

    /**
     * emit qasm code
     */
    void emit_eqasm(bool verbose=false)
    {
        IOUT("emitting eqasm...");
        eqasm_code.clear();
        // eqasm_code.push_back("wait 1");       // add wait 1 at the begining
        // eqasm_code.push_back("mov r14, 0");   // 0: infinite loop
        // eqasm_code.push_back("start:");       // label
        size_t t = 0;
        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
        {
            size_t start = instr->start;
            size_t dt = start-t;
            if (dt)
            {
                // eqasm_code.push_back("wait "+std::to_string(dt));
                // t = start;
            }
            eqasm_code.push_back(instr->code());
        }
        // eqasm_code.push_back("wait "+std::to_string(cc_light_eqasm_instructions.back()->duration));
        // eqasm_code.push_back("beq r14, r14 start");  // loop
        IOUT("emitting eqasm code done.");
    }

    /**
     * process
     */
    void process_single_qubit_gate(std::string instr_name, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
    {
        cc_light_single_qubit_gate * instr = new cc_light_single_qubit_gate(instr_name,single_qubit_mask(qubits[0]));
        cc_light_eqasm_instructions.push_back(instr);
    }

    /**
     * return operation type
     */
    operation_type_t operation_type(std::string type)
    {
        if (type == "mw")
            return __rf__;
        else if (type == "flux")
            return __flux__;
        else if (type == "readout")
            return __measurement__;
        else
            return __unknown_operation__;
    }
};
}
}

#endif // QL_CC_LIGHT_EQASM_COMPILER_H

