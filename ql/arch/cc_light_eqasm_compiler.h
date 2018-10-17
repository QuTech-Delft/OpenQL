/**
 * @file   cc_light_eqasm_compiler.h
 * @date   08/2017
 * @author Imran Ashraf
 *         Nader Khammassi
 * @brief  cclighteqasm compiler implementation
 */

#ifndef QL_CC_LIGHT_EQASM_COMPILER_H
#define QL_CC_LIGHT_EQASM_COMPILER_H

#include <ql/utils.h>
#include <ql/platform.h>
#include <ql/kernel.h>
#include <ql/gate.h>
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

typedef std::vector<size_t>        qubit_set_t;
typedef std::pair<size_t,size_t>   qubit_pair_t;
typedef std::vector<qubit_pair_t>  qubit_pair_set_t;

const size_t MAX_S_REG =32;
const size_t MAX_T_REG =64;

size_t CurrSRegCount=0;
size_t CurrTRegCount=0;

class Mask
{
public:
    size_t regNo;
    std::string regName;
    qubit_set_t squbits;
    qubit_pair_set_t dqubits;

    Mask() {}

    Mask(qubit_set_t & qs) : squbits(qs)
    {
        
        if(CurrSRegCount < MAX_S_REG)
        {
            regNo = CurrSRegCount++;
            regName = "s" + std::to_string(regNo);
        }
        else
        {
            COUT(" !!!! Handle cases requiring more registers");
        }
    }

    Mask(std::string rn, qubit_set_t & qs ) : regName(rn), squbits(qs)
    { 
        if(CurrSRegCount < MAX_S_REG)
        {
            regNo = CurrSRegCount++;
        }
        else
        {
            COUT(" !!!! Handle cases requiring more registers");
        }
    }

    Mask(qubit_pair_set_t & qps) : dqubits(qps) 
    { 
        if(CurrTRegCount < MAX_T_REG)
        {
            regNo = CurrTRegCount++;
            regName = "t" + std::to_string(regNo);
        }
        else
        {
            COUT(" !!!! Handle cases requiring more registers");
        }
    }

};

class MaskManager
{
private:
    std::map<size_t,Mask> SReg2Mask;
    std::map<qubit_set_t,Mask> QS2Mask;

    std::map<size_t,Mask> TReg2Mask;
    std::map<qubit_pair_set_t,Mask> QPS2Mask;

public:
    MaskManager()
    {
        // add pre-defined smis
        for(size_t i=0; i<7; ++i)
        {
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

    size_t getRegNo( qubit_set_t & qs )
    {
        // sort qubit operands to avoid variation in order
        sort(qs.begin(), qs.end());

        auto it = QS2Mask.find(qs);
        if( it == QS2Mask.end() )
        {
            Mask m(qs);
            QS2Mask[qs] = m;
            SReg2Mask[m.regNo] = m;
        }
        return QS2Mask[qs].regNo;
    }

    size_t getRegNo( qubit_pair_set_t & qps )
    {
        // sort qubit operands pair to avoid variation in order
        sort(qps.begin(), qps.end(), ql::utils::sort_pair_helper);

        auto it = QPS2Mask.find(qps);
        if( it == QPS2Mask.end() )
        {
            Mask m(qps);
            QPS2Mask[qps] = m;
            TReg2Mask[m.regNo] = m;
        }
        return QPS2Mask[qps].regNo;
    }

    std::string getRegName( qubit_set_t & qs )
    {
        // sort qubit operands to avoid variation in order
        sort(qs.begin(), qs.end());

        auto it = QS2Mask.find(qs);
        if( it == QS2Mask.end() )
        {
            Mask m(qs);
            QS2Mask[qs] = m;
            SReg2Mask[m.regNo] = m;
        }
        return QS2Mask[qs].regName;
    }

    std::string getRegName( qubit_pair_set_t & qps )
    {
        // sort qubit operands pair to avoid variation in order
        sort(qps.begin(), qps.end(), ql::utils::sort_pair_helper);

        auto it = QPS2Mask.find(qps);
        if( it == QPS2Mask.end() )
        {
            Mask m(qps);
            QPS2Mask[qps] = m;
            TReg2Mask[m.regNo] = m;
        }
        return QPS2Mask[qps].regName;
    }

    std::string getMaskInstructions()
    {
        std::stringstream ssmasks;
        for(size_t r=0; r<CurrSRegCount; ++r)
        {
            auto & m = SReg2Mask[r];
            ssmasks << "smis " << m.regName << ", {";
            for(auto it = m.squbits.begin(); it != m.squbits.end(); ++it)
            {
                ssmasks << *it;
                if( std::next(it) != m.squbits.end() )
                    ssmasks << ", ";
            }
            ssmasks << "} \n";
        }

        for(size_t r=0; r<CurrTRegCount; ++r)
        {
            auto & m = TReg2Mask[r];
            ssmasks << "smit " << m.regName << ", {";
            for(auto it = m.dqubits.begin(); it != m.dqubits.end(); ++it)
            {
                ssmasks << "(" << it->first << ", " << it->second << ")";
                if( std::next(it) != m.dqubits.end() )
                    ssmasks << ", ";
            }
            ssmasks << "} \n";
        }

        return ssmasks.str();
    }

    ~MaskManager()
    {
        CurrSRegCount=0;
        CurrTRegCount=0;
    }

};



class classical_cc : public gate
{
public:
    cmat_t m;
    int imm_value;
    classical_cc(std::string operation, std::vector<size_t> opers, int ivalue=0)
    {
        DOUT("adding classical_cc " << operation);
        str::lower_case(operation);
        name=operation;
        duration = 20;
        operands=opers;
        int sz = operands.size();
        if((   (name == "add") || (name == "sub") 
            || (name == "and") || (name == "or") || (name == "xor")
           ) && (sz == 3))
        {
            DOUT("Adding 3 operand operation: " << name);
        }
        else if(((name == "not") || (name == "fmr") || (name == "cmp")) && (sz == 2))
        {
            DOUT("Adding 2 operand operation: " << name);
        }
        else if( ( (name == "ldi") ||
            (name == "fbr_eq") || (name == "fbr_ne") || (name == "fbr_lt") ||
            (name == "fbr_gt") || (name == "fbr_le") || (name == "fbr_ge")
            ) && (sz == 1) )
        {
            if( (name == "ldi") )
            {
                imm_value = ivalue;
            }
            DOUT("Adding 1 operand operation: " << name);
        }
        else if( (name == "nop") && (sz == 0) )
        {
            DOUT("Adding 0 operand operation: " << name);
        }
        else
        {
            EOUT("Unknown cclight classical operation '" << name << "' with '" << sz << "' operands!");
            throw ql::exception("Unknown cclight classical operation'"+name+"' with'"+std::to_string(sz)+"' operands!", false);
        }
        DOUT("adding classical_cc [DONE]");
    }

    instruction_t qasm()
    {
        std::string iopers;
        int sz = operands.size();
        for(int i=0; i<sz; ++i)
        {
            if(i==sz-1)
                iopers += " r" + std::to_string(operands[i]);
            else
                iopers += " r" + std::to_string(operands[i]) + ",";
        }

        if(name == "ldi")
        {
            iopers += ", " + std::to_string(imm_value);
            return "ldi" + iopers;
        }
        else if(name == "fmr")
        {
            return name + " r" + std::to_string(operands[0]) +
                          ", q" + std::to_string(operands[1]);
        }
        else
            return name + iopers;
    }

    instruction_t micro_code()
    {
        return ql::dep_instruction_map["nop"];
    }

    gate_type_t type()
    {
        return __classical_gate__;
    }

    cmat_t mat()
    {
        return m;
    }

};

std::string classical_instruction2qisa(ql::arch::classical_cc* classical_ins)
{
    std::stringstream ssclassical;
    auto & iname =  classical_ins->name;
    auto & iopers = classical_ins->operands;
    int iopers_count = iopers.size();

    if(  (iname == "add") || (iname == "sub") ||
         (iname == "and") || (iname == "or") || (iname == "not") || (iname == "xor") ||
         (iname == "ldi") || (iname == "nop") || (iname == "cmp")
      )
    {
        ssclassical << iname;
        for(int i=0; i<iopers_count; ++i)
        {
            if(i==iopers_count-1)
                ssclassical << " r" <<  iopers[i];
            else
                ssclassical << " r" << iopers[i] << ",";
        }
        if(iname == "ldi")
        {
            ssclassical << ", " + std::to_string(classical_ins->imm_value);
        }
    }
    else if(iname == "fmr")
    {
        ssclassical << "fmr r" << iopers[0] << ", q" << iopers[1];
    }
    else if(iname == "fbr_eq")
    {
        ssclassical << "fbr " << "EQ, r" << iopers[0];
    }
    else if(iname == "fbr_ne")
    {
        ssclassical << "fbr " << "NE, r" << iopers[0];
    }
    else if(iname == "fbr_lt")
    {
        ssclassical << "fbr " << "LT, r" << iopers[0];
    }
    else if(iname == "fbr_gt")
    {
        ssclassical << "fbr " << "GT, r" << iopers[0];
    }
    else if(iname == "fbr_le")
    {
        ssclassical << "fbr " << "LE, r" << iopers[0];
    }
    else if(iname == "fbr_ge")
    {
        ssclassical << "fbr " << "GE, r" << iopers[0];
    }
    else
    {
        EOUT("Unknown CClight classical operation '" << iname << "' with '" << iopers_count << "' operands!");
        throw ql::exception("Unknown classical operation'"+iname+"' with'"+std::to_string(iopers_count)+"' operands!", false);
    }

    return ssclassical.str();
}

std::string bundles2qisa(ql::ir::bundles_t & bundles,
    ql::quantum_platform & platform, MaskManager & gMaskManager)
{
    IOUT("Generating CC-Light QISA");

    std::stringstream ssbundles, sspre, ssinst;
    size_t curr_cycle=0;

    for (ql::ir::bundle_t & abundle : bundles)
    {
        std::string iname;
        std::stringstream sspre, ssinst;
        auto bcycle = abundle.start_cycle;
        auto delta = bcycle - curr_cycle;
        bool classical_bundle=false;
        if(delta < 8)
            sspre << "    " << delta << "    ";
        else
            sspre << "    qwait " << delta-1 << "\n"
                  << "    1    ";

        for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
        {
            qubit_set_t squbits;
            qubit_pair_set_t dqubits;
            auto firstInsIt = secIt->begin();
            iname = (*(firstInsIt))->name;
            auto itype = (*(firstInsIt))->type();

            if(__classical_gate__ == itype)
            {
                classical_bundle = true;
                ssinst << classical_instruction2qisa( (ql::arch::classical_cc *)(*firstInsIt) );
            }
            else
            {
                auto id = iname;
                DOUT("get cclight instr name for : " << id);
                std::string cc_light_instr_name;
                auto it = platform.instruction_map.find(id);
                if (it != platform.instruction_map.end())
                {
                    custom_gate* g = it->second;
                    cc_light_instr_name = g->arch_operation_name;
                    if(cc_light_instr_name.empty())
                    {
                        EOUT("cc_light_instr not defined for instruction: " << id << " !");
                        throw ql::exception("Error : cc_light_instr not defined for instruction: "+id+" !",false);
                    }                    
                }
                else
                {
                    EOUT("custom instruction not found for : " << id << " !");
                    throw ql::exception("Error : custom instruction not found for : "+id+" !",false);
                }

                auto nOperands = ((*firstInsIt)->operands).size();
                if( itype == __nop_gate__ )
                {
                    ssinst << cc_light_instr_name;
                }
                else
                {
                    for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                    {
                        if( 1 == nOperands )
                        {
                            auto & op = (*insIt)->operands[0];
                            squbits.push_back(op);
                        }
                        else if( 2 == nOperands )
                        {
                            auto & op1 = (*insIt)->operands[0];
                            auto & op2 = (*insIt)->operands[1];
                            dqubits.push_back( qubit_pair_t(op1,op2) );
                        }
                        else
                        {
                            throw ql::exception("Error : only 1 and 2 operand instructions are supported by cc light masks !",false);
                        }
                    }

                    //TODO:ORDER sort squbits here
                    //TODO:ORDER sort dqubits here

                    std::string rname;
                    if( 1 == nOperands )
                    {
                        rname = gMaskManager.getRegName(squbits);
                    }
                    else if( 2 == nOperands )
                    {
                        rname = gMaskManager.getRegName(dqubits);
                    }
                    else
                    {
                        throw ql::exception("Error : only 1 and 2 operand instructions are supported by cc light masks !",false);
                    }

                    ssinst << cc_light_instr_name << " " << rname;
                }

                if( std::next(secIt) != abundle.parallel_sections.end() )
                {
                    ssinst << " | ";
                }
            }
        }
        if(classical_bundle)
        {
            if(iname == "fmr")
            {
                // based on cclight requirements (section 4.7 eqasm manual),
                // two extra instructions need to be added between meas and fmr
                if(delta > 2)
                {
                    ssbundles << "    qwait " << 1 << "\n";
                    ssbundles << "    qwait " << delta-1 << "\n";
                }                    
                else
                {
                    ssbundles << "    qwait " << 1 << "\n";
                    ssbundles << "    qwait " << 1 << "\n";
                }
            }
            else
            {
                if(delta > 1)
                    ssbundles << "    qwait " << delta << "\n";
            }
            ssbundles << "    " << ssinst.str() << "\n";
        }
        else
        {
            ssbundles << sspre.str() << ssinst.str() << "\n";
        }
        curr_cycle+=delta;        
    }

    auto & lastBundle = bundles.back();
    int lbduration = lastBundle.duration_in_cycles;
    if( lbduration>1 )
        ssbundles << "    qwait " << lbduration << "\n";

    IOUT("Generating CC-Light QISA [Done]");
    return ssbundles.str();
}

void WriteCCLightQisa(std::string prog_name, ql::quantum_platform & platform, MaskManager & gMaskManager,
    ql::ir::bundles_t & bundles)
{
    IOUT("Generating CC-Light QISA");

    ofstream fout;
    string qisafname( ql::options::get("output_dir") + "/" + prog_name + ".qisa");
    fout.open( qisafname, ios::binary);
    if ( fout.fail() )
    {
        EOUT("opening file " << qisafname << std::endl
                 << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
        return;
    }


    std::stringstream ssbundles;
    ssbundles << "start:" << "\n";
    ssbundles << bundles2qisa(bundles, platform, gMaskManager);
    ssbundles << "    br always, start" << "\n"
              << "    nop \n"
              << "    nop" << endl;


    IOUT("Writing CC-Light QISA to " << qisafname);
    fout << gMaskManager.getMaskInstructions() << endl << ssbundles.str() << endl;
    fout.close();
    IOUT("Generating CC-Light QISA [Done]");
}


void WriteCCLightQisaTimeStamped(std::string prog_name, ql::quantum_platform & platform, MaskManager & gMaskManager,
    ql::ir::bundles_t & bundles)
{
    IOUT("Generating Time-stamped CC-Light QISA");
    ofstream fout;
    string qisafname( ql::options::get("output_dir") + "/" + prog_name + ".tqisa");
    fout.open( qisafname, ios::binary);
    if ( fout.fail() )
    {
        EOUT("opening file " << qisafname << std::endl
                 << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
        return;
    }


    std::stringstream ssbundles;
    size_t curr_cycle=0; // first instruction should be with pre-interval 1, 'bs 1'
    ssbundles << "start:" << "\n";
    for (ql::ir::bundle_t & abundle : bundles)
    {
        auto bcycle = abundle.start_cycle;
        auto delta = bcycle - curr_cycle;

        if(delta < 8)
            ssbundles << std::setw(8) << curr_cycle << ":    bs " << delta << "    ";
        else
            ssbundles << std::setw(8) << curr_cycle << ":    qwait " << delta-1 << "\n"
                      << std::setw(8) << curr_cycle + (delta-1) << ":    bs 1    ";

        for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
        {
            qubit_set_t squbits;
            qubit_pair_set_t dqubits;
            auto firstInsIt = secIt->begin();

            auto id = (*(firstInsIt))->name;
            std::string cc_light_instr_name = get_cc_light_instruction_name(id, platform);
            auto itype = (*(firstInsIt))->type();
            auto nOperands = ((*firstInsIt)->operands).size();
            if( itype == __nop_gate__ )
            {
                ssbundles << cc_light_instr_name;
            }
            else
            {
                for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                {
                    if( 1 == nOperands )
                    {
                        auto & op = (*insIt)->operands[0];
                        squbits.push_back(op);
                    }
                    else if( 2 == nOperands )
                    {
                        auto & op1 = (*insIt)->operands[0];
                        auto & op2 = (*insIt)->operands[1];
                        dqubits.push_back( qubit_pair_t(op1,op2) );
                    }
                    else
                    {
                        throw ql::exception("Error : only 1 and 2 operand instructions are supported by cc light masks !",false);
                    }
                }
                std::string rname;
                if( 1 == nOperands )
                {
                    rname = gMaskManager.getRegName(squbits);
                }
                else if( 2 == nOperands )
                {
                    rname = gMaskManager.getRegName(dqubits);
                }
                else
                {
                    throw ql::exception("Error : only 1 and 2 operand instructions are supported by cc light masks !",false);
                }

                ssbundles << cc_light_instr_name << " " << rname;
            }

            if( std::next(secIt) != abundle.parallel_sections.end() )
            {
                ssbundles << " | ";
            }
        }
        curr_cycle+=delta;
        ssbundles << "\n";
    }

    auto & lastBundle = bundles.back();
    int lbduration = lastBundle.duration_in_cycles;
    if( lbduration>1 )
        ssbundles << std::setw(8) << curr_cycle   << ":    qwait " << lbduration << "\n";
    curr_cycle+=lbduration;
    ssbundles << std::setw(8) << curr_cycle++ << ":    br always, start" << "\n";
    ssbundles << std::setw(8) << curr_cycle++ << ":    nop \n";
    ssbundles << std::setw(8) << curr_cycle++ << ":    nop" << endl;

    IOUT("Writing Time-stamped CC-Light QISA to " << qisafname);
    fout << gMaskManager.getMaskInstructions() << endl << ssbundles.str() << endl;
    fout.close();

    IOUT("Generating Time-stamped CC-Light QISA [Done]");
}



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
     * program-level compilaation of qasm to cc_light_eqasm
     */
    void compile(std::string prog_name, ql::circuit& ckt, ql::quantum_platform& platform)
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

    std::string get_prologue(ql::quantum_kernel &k)
    {
        std::stringstream ss;

        if(k.type == kernel_type_t::IF_START)
        {
            #if 0
            // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition.inv_operation_name 
               <<" r" << (k.br_condition.operands[0])->id <<", r" << (k.br_condition.operands[1])->id
               << ", " << k.name << "_end\n";
            #else
            ss  <<"    cmp r" << (k.br_condition.operands[0])->id 
                <<", r" << (k.br_condition.operands[1])->id << '\n';
            ss  <<"    nop\n";
            ss  <<"    br " << k.br_condition.inv_operation_name << ", "
                << k.name << "_end\n";
            #endif

        }

        if(k.type == kernel_type_t::ELSE_START)
        {
            #if 0
            // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition.operation_name <<" r" << (k.br_condition.operands[0])->id
               <<", r" << (k.br_condition.operands[1])->id << ", " << k.name << "_end\n";
            #else
            ss  <<"    cmp r" << (k.br_condition.operands[0])->id 
                <<", r" << (k.br_condition.operands[1])->id << '\n';
            ss  <<"    nop\n";
            ss  <<"    br " << k.br_condition.operation_name << ", "
                << k.name << "_end\n";
            #endif
        }

        if(k.type == kernel_type_t::FOR_START)
        {
            // for now r29, r30, r31 are used as temporaries
            ss << "    ldi r29" <<", " << k.iterations << "\n";
            ss << "    ldi r30" <<", " << 1 << "\n";
            ss << "    ldi r31" <<", " << 0 << "\n";
        }

        return ss.str();
    }

    std::string get_epilogue(ql::quantum_kernel &k)
    {
        std::stringstream ss;

        if(k.type == kernel_type_t::DO_WHILE_END)
        {
            #if 0
            // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    b" << k.br_condition.operation_name <<" r" << (k.br_condition.operands[0])->id
               <<", r" << (k.br_condition.operands[1])->id << ", " << k.name << "_start\n";
            #else
            ss  <<"    cmp r" << (k.br_condition.operands[0])->id 
                <<", r" << (k.br_condition.operands[1])->id << '\n';
            ss  <<"    nop\n";
            ss  <<"    br " << k.br_condition.operation_name << ", "
                << k.name << "_start\n";
            #endif
        }

        if(k.type == kernel_type_t::FOR_END)
        {
            std::string kname(k.name);
            std::replace( kname.begin(), kname.end(), '_', ' ');
            std::istringstream iss(kname);
            std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                             std::istream_iterator<std::string>{} };

            // for now r29, r30, r31 are used
            ss << "    add r31, r31, r30\n";
            #if 0
            // Branching macros are not yet supported by assembler,
            // so for now the following can not be used
            ss << "    blt r31, r29, " << tokens[0] << "\n";
            #else
            ss  <<"    cmp r31, r29\n";
            ss  <<"    nop\n";
            ss  <<"    br lt, " << tokens[0] << "\n";
            #endif
        }

        return ss.str();
    }

    // kernel level compilation
    void compile(std::string prog_name, std::vector<quantum_kernel> kernels, ql::quantum_platform& platform)
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
            sskernels_qisa << "\n" << kernel.name << ":" << std::endl;
            sskernels_qisa << get_prologue(kernel);
            ql::circuit decomp_ckt;
            ql::circuit& ckt = kernel.c;
            auto num_creg = kernel.creg_count;
            if (! ckt.empty())
            {
                // decompose meta-instructions
                decompose_instructions(ckt, decomp_ckt, platform);

                // schedule with platform resource constraints
                ql::ir::bundles_t bundles = cc_light_schedule_rc(decomp_ckt, platform, num_qubits, num_creg);

                // std::cout << "QASM" << std::endl;
                // std::cout << ql::ir::qasm(bundles) << std::endl;

                sskernels_qisa << bundles2qisa(bundles, platform, mask_manager);
            }
            sskernels_qisa << get_epilogue(kernel);
        }

        sskernels_qisa << "\n    br always, start" << "\n"
                  << "    nop \n"
                  << "    nop" << std::endl;

        ssqisa << mask_manager.getMaskInstructions() << sskernels_qisa.str();
        // std::cout << ssqisa.str();

        // write qisa file
        std::ofstream fout;
        std::string qisafname( ql::options::get("output_dir") + "/" + prog_name + ".qisa");
        IOUT("Writing CC-Light QISA to " << qisafname);
        fout.open( qisafname, ios::binary);
        if ( fout.fail() )
        {
            EOUT("opening file " << qisafname << std::endl
                     << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }

        fout << ssqisa.str() << endl;
        fout.close();

        DOUT("Compiling CCLight eQASM [Done]");
    }

    /**
     * decompose
     */
    void decompose_instructions(ql::circuit& ckt, ql::circuit& decomp_ckt, ql::quantum_platform& platform)
    {
        DOUT("decomposing instructions...");
        for( auto ins : ckt )
        {
            auto & iname =  ins->name;
            str::lower_case(iname);
            DOUT("decomposing instruction " << iname << "...");            
            auto & iopers = ins->operands;
            int iopers_count = iopers.size();
            auto itype = ins->type();
            if(__classical_gate__ == itype)
            {
                DOUT("    classical instruction");

                if( (iname == "add") || (iname == "sub") ||
                    (iname == "and") || (iname == "or") || (iname == "xor") ||
                    (iname == "not") || (iname == "nop")
                  )
                {
                    // decomp_ckt.push_back(ins);
                    decomp_ckt.push_back(new ql::arch::classical_cc(iname, iopers));
                }
                else if( (iname == "eq") || (iname == "ne") || (iname == "lt") ||
                         (iname == "gt") || (iname == "le") || (iname == "ge")
                       )
                {
                    decomp_ckt.push_back(new ql::arch::classical_cc("cmp", {iopers[1], iopers[2]}));
                    decomp_ckt.push_back(new ql::arch::classical_cc("nop", {}));
                    decomp_ckt.push_back(new ql::arch::classical_cc("fbr_"+iname, {iopers[0]}));
                }
                else if(iname == "mov")
                {
                    // r28 is used as temp, TODO use creg properly to create temporary
                    decomp_ckt.push_back(new ql::arch::classical_cc("ldi", {28}, 0));
                    decomp_ckt.push_back(new ql::arch::classical_cc("add", {iopers[0], iopers[1], 28}));
                }
                else if(iname == "ldi")
                {
                    auto imval = ((ql::classical*)ins)->imm_value;
                    decomp_ckt.push_back(new ql::arch::classical_cc("ldi", iopers, imval));
                }
                else
                {
                    EOUT("Unknown decomposition of classical operation '" << iname << "' with '" << iopers_count << "' operands!");
                    throw ql::exception("Unknown classical operation '"+iname+"' with'"+std::to_string(iopers_count)+"' operands!", false);
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
                    json& instruction_settings = platform.instruction_settings;
                    std::string operation_type;
                    if (instruction_settings.find(iname) != instruction_settings.end())
                    {
                        operation_type = instruction_settings[iname]["type"];
                    }
                    else
                    {
                        EOUT("instruction settings not found for '" << iname << "' with '" << iopers_count << "' operands!");
                        throw ql::exception("instruction settings not found for '"+iname+"' with'"+std::to_string(iopers_count)+"' operands!", false);
                    }
                    bool is_measure = (operation_type == "readout");
                    if(is_measure)
                    {
                        // insert measure
                        DOUT("    readout instruction ");
                        auto qop = iopers[0];
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
                                WOUT("Unknown classical operand for measure/readout operation: '" << iname <<
                                    ". This will soon be depricated in favour of measure instruction with fmr" <<
                                    " to store measurement outcome to classical register.");
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

        /*
        cc_light_eqasm_program_t decomposed;
        for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
        {
        cc_light_eqasm_program_t dec = instr->decompose();
          for (cc_light_eqasm_instruction * i : dec)
             decomposed.push_back(i);
            }
            cc_light_eqasm_instructions.swap(decomposed);
        */
        DOUT("decomposing instructions...[Done]");
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
                EOUT("cc_light_instr not found for " << i);
                throw ql::exception("cc_light_instr not found", false);
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

