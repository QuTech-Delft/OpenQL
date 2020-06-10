/**
 * @file   cc_light_eqasm_compiler.h
 * @date   08/2017
 * @author Imran Ashraf
 *         Nader Khammassi
 * @brief  cclighteqasm compiler implementation
 */

#ifndef QL_CC_LIGHT_EQASM_COMPILER_H
#define QL_CC_LIGHT_EQASM_COMPILER_H

#include <compile_options.h>
#include <utils.h>
#include <platform.h>
#include <kernel.h>
#include <gate.h>
#include <ir.h>
#include <eqasm_compiler.h>
#include <arch/cc_light/cc_light_eqasm.h>
#include <arch/cc_light/cc_light_scheduler.h>
#include <mapper.h>
#include <clifford.h>
#include <qsoverlay.h>

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
    // int imm_value;
    classical_cc(std::string operation, std::vector<size_t> opers, int ivalue=0)
    {
        DOUT("Classical_cc constructor for operation " << operation);
        DOUT("... operands:");
        for (auto o : opers) DOUT ("...... " << o << " ");
        DOUT("...... ivalue= " << ivalue);

        // DOUT("adding classical_cc " << operation);
        str::lower_case(operation);
        name=operation;
        duration = 20;
        creg_operands=opers;
        int sz = creg_operands.size();
        if((   (name == "add") || (name == "sub")
            || (name == "and") || (name == "or") || (name == "xor")
           ) && (sz == 3))
        {
            DOUT("Adding 3 operand operation: " << name);
        }
        else if(((name == "not") || (name == "cmp")) && (sz == 2))
        {
            DOUT("Adding 2 operand operation: " << name);
        }
        else if( (name == "fmr")  && (sz == 2))
        {
            creg_operands= { opers[0] };
            operands= { opers[1] };
            DOUT("Adding 2 operand fmr operation: " << name);
        }
        else if( ( (name == "ldi") ||
            (name == "fbr_eq") || (name == "fbr_ne") || (name == "fbr_lt") ||
            (name == "fbr_gt") || (name == "fbr_le") || (name == "fbr_ge")
            ) && (sz == 1) )
        {
            if( (name == "ldi") )
            {
                DOUT("... setting int_operand of classical_cc gate for operation " << operation << " to " << ivalue);
                int_operand = ivalue;
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
        int sz = creg_operands.size();
        for(int i=0; i<sz; ++i)
        {
            if(i==sz-1)
                iopers += " r" + std::to_string(creg_operands[i]);
            else
                iopers += " r" + std::to_string(creg_operands[i]) + ",";
        }

        if(name == "ldi")
        {
            iopers += ", " + std::to_string(int_operand);
            return "ldi" + iopers;
        }
        else if(name == "fmr")
        {
            return name + " r" + std::to_string(creg_operands[0]) +
                          ", q" + std::to_string(operands[0]);
        }
        else
            return name + iopers;
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
    auto & iopers = classical_ins->creg_operands;
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
            ssclassical << ", " + std::to_string(classical_ins->int_operand);
        }
    }
    else if(iname == "fmr")
    {
        ssclassical << "fmr r" << iopers[0] << ", q" << classical_ins->operands[0];
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
    const ql::quantum_platform & platform, MaskManager & gMaskManager)
{
    IOUT("Generating CC-Light QISA");

    std::stringstream ssbundles, sspre, ssinst;
    size_t curr_cycle=0;

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
    for (ql::ir::bundle_t & abundle : bundles)
    {
        // sorts instructions alphabetically
        abundle.parallel_sections.sort( []
            (const ql::ir::section_t & sec1, const ql::ir::section_t & sec2) -> bool
            {
                auto i1 = sec1.begin();
                auto iname1 = (*(i1))->name;
                auto i2 = sec2.begin();
                auto iname2 = (*(i2))->name;
                return iname2 < iname1;
            });
    }

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

        for(auto secIt = abundle.parallel_sections.begin();
            secIt != abundle.parallel_sections.end(); ++secIt )
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
                DOUT("get cclight instr name for : " << iname);
                std::string cc_light_instr_name = get_cc_light_instruction_name(iname, platform);
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
                            dqubits.push_back( qubit_pair_t(op1, op2) );
                        }
                        else
                        {
                            throw ql::exception("Error : only 1 and 2 operand instructions are supported by cc light masks !",false);
                        }
                    }

                    std::string rname;
                    if(1 == nOperands)
                    {
                        rname = gMaskManager.getRegName(squbits);
                    }
                    else if(2 == nOperands)
                    {
                        rname = gMaskManager.getRegName(dqubits);
                    }
                    else
                    {
                        throw ql::exception("Error : only 1 and 2 operand instructions are supported by cc light masks !",false);
                    }

                    ssinst << cc_light_instr_name << " " << rname;
                }
            }

            if( std::next(secIt) != abundle.parallel_sections.end() )
            {
                ssinst << " | ";
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
            // ssbundles << sspre.str() << ssinst.str() << "\t\t# @" << bcycle << "\n";
            ssbundles << sspre.str() << ssinst.str() << "\n";
        }
        curr_cycle+=delta;
    }

    auto & lastBundle = bundles.back();
    int lbduration = lastBundle.duration_in_cycles;
    if(lbduration>1)
        ssbundles << "    qwait " << lbduration << "\n";

    IOUT("Generating CC-Light QISA [Done]");
    return ssbundles.str();
}

void WriteCCLightQisa(std::string prog_name, const ql::quantum_platform & platform, MaskManager & gMaskManager,
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


void WriteCCLightQisaTimeStamped(std::string prog_name, const ql::quantum_platform & platform, MaskManager & gMaskManager,
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

        for(auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt)
        {
            qubit_set_t squbits;
            qubit_pair_set_t dqubits;
            auto firstInsIt = secIt->begin();

            auto id = (*(firstInsIt))->name;
            std::string cc_light_instr_name = get_cc_light_instruction_name(id, platform);
            auto itype = (*(firstInsIt))->type();
            auto nOperands = ((*firstInsIt)->operands).size();
            if(itype == __nop_gate__)
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

            if(std::next(secIt) != abundle.parallel_sections.end())
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

    // FIXME: should be private
    std::string get_qisa_prologue(ql::quantum_kernel &k)
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

    std::string get_qisa_epilogue(ql::quantum_kernel &k)
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


    void decompose_post_schedule(ql::ir::bundles_t & bundles_dst,
        const ql::quantum_platform& platform)
    {
        auto bundles_src = bundles_dst;

        IOUT("Post scheduling decomposition ...");
        if (ql::options::get("cz_mode") == "auto")
        {
            IOUT("decompose cz to cz+sqf...");

            typedef std::pair<size_t,size_t> qubits_pair_t;
            std::map< qubits_pair_t, size_t > qubitpair2edge;           // map: pair of qubits to edge (from grid configuration)
            std::map<size_t, std::vector<size_t> > edge_detunes_qubits; // map: edge to vector of qubits that edge detunes (resource desc.)

            // initialize qubitpair2edge map from json description; this is a constant map
            for(auto & anedge : platform.topology["edges"])
            {
                size_t s = anedge["src"];
                size_t d = anedge["dst"];
                size_t e = anedge["id"];

                qubits_pair_t aqpair(s,d);
                auto it = qubitpair2edge.find(aqpair);
                if( it != qubitpair2edge.end() )
                {
                    FATAL("re-defining edge " << s <<"->" << d << " !");
                }
                else
                {
                    qubitpair2edge[aqpair] = e;
                }
            }

            // initialize edge_detunes_qubits map from json description; this is a constant map
            auto & constraints = platform.resources["detuned_qubits"]["connection_map"];
            for (auto it = constraints.begin(); it != constraints.end(); ++it)
            {
                size_t edgeNo = stoi( it.key() );
                auto & detuned_qubits = it.value();
                for(auto & q : detuned_qubits)
                    edge_detunes_qubits[edgeNo].push_back(q);
            }

            for (
                auto bundles_src_it = bundles_src.begin(), bundles_dst_it = bundles_dst.begin();
                bundles_src_it != bundles_src.end() && bundles_dst_it != bundles_dst.end();
                ++bundles_src_it, ++bundles_dst_it
                )
            {
                for(auto sec_src_it = bundles_src_it->parallel_sections.begin();
                    sec_src_it != bundles_src_it->parallel_sections.end();
                    ++sec_src_it)
                {
                    for(auto ins_src_it = sec_src_it->begin();
                        ins_src_it != sec_src_it->end();
                        ++ins_src_it )
                    {
                        std::string id = (*ins_src_it)->name;
                        std::string operation_type = "";
                        size_t nOperands = ((*ins_src_it)->operands).size();
                        if(2 == nOperands)
                        {
                            auto it = platform.instruction_map.find(id);
                            if (it != platform.instruction_map.end())
                            {
                                if(platform.instruction_settings[id].count("type") > 0)
                                {
                                    operation_type = platform.instruction_settings[id]["type"].get<std::string>();
                                }
                            }
                            else
                            {
                                FATAL("custom instruction not found for : " << id << " !");
                            }

                            bool is_flux_2_qubit = ( (operation_type == "flux") );
                            if( is_flux_2_qubit )
                            {
                                auto & q0 = (*ins_src_it)->operands[0];
                                auto & q1 = (*ins_src_it)->operands[1];
                                DOUT("found 2 qubit flux gate on " << q0 << " and " << q1);
                                qubits_pair_t aqpair(q0, q1);
                                auto it = qubitpair2edge.find(aqpair);
                                if( it != qubitpair2edge.end() )
                                {
                                    auto edge_no = qubitpair2edge[aqpair];
                                    DOUT("add the following sqf gates for edge: " << edge_no << ":");
                                    for( auto & q : edge_detunes_qubits[edge_no])
                                    {
                                        DOUT("sqf q" << q);
                                        custom_gate* g = new custom_gate("sqf q"+std::to_string(q));
                                        g->operands.push_back(q);

                                        ql::ir::section_t asec;
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

    void clifford_optimize(std::string prog_name, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform, std::string opt)
    {
        if (ql::options::get(opt) == "no")
        {
            DOUT("Clifford optimization on program " << prog_name << " at " << opt << " not DONE");
            return;
        }
        DOUT("Clifford optimization on program " << prog_name << " at " << opt << " ...");

        ql::report::report_statistics(prog_name, kernels, platform, "in", opt, "# ");
        ql::report::report_qasm(prog_name, kernels, platform, "in", opt);

        Clifford cliff;
        for(auto &kernel : kernels)
        {
            cliff.Optimize(kernel, opt);
        }

        ql::report::report_statistics(prog_name, kernels, platform, "out", opt, "# ");
        ql::report::report_qasm(prog_name, kernels, platform, "out", opt);
    }

    void map(std::string prog_name, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform)
    {
        auto mapopt = ql::options::get("mapper");
        if (mapopt == "no" )
        {
            IOUT("Not mapping kernels");
            return;
        }

        ql::report::report_statistics(prog_name, kernels, platform, "in", "mapper", "# ");
        ql::report::report_qasm(prog_name, kernels, platform, "in", "mapper");

        Mapper mapper;  // virgin mapper creation; for role of Init functions, see comment at top of mapper.h
        mapper.Init(&platform); // platform specifies number of real qubits, i.e. locations for virtual qubits

        std::ofstream   ofs;
        ofs = ql::report::report_open(prog_name, "out", "mapper");

        size_t  total_swaps = 0;        // for reporting, data is mapper specific
        size_t  total_moves = 0;        // for reporting, data is mapper specific
        double  total_timetaken = 0.0;  // total over kernels of time taken by mapper
        for(auto &kernel : kernels)
        {
            IOUT("Mapping kernel: " << kernel.name);

            // compute timetaken, start interval timer here
            double    timetaken = 0.0;
            using namespace std::chrono;
            high_resolution_clock::time_point t1 = high_resolution_clock::now();

            mapper.Map(kernel);
                // kernel.qubit_count starts off as number of virtual qubits, i.e. highest indexed qubit minus 1
                // kernel.qubit_count is updated by Map to highest index of real qubits used minus -1

            // computing timetaken, stop interval timer
            high_resolution_clock::time_point t2 = high_resolution_clock::now();
            duration<double> time_span = t2 - t1;
            timetaken = time_span.count();

            kernel.bundles = ql::ir::bundler(kernel.c, ns_per_cycle);   // assignment to kernel.bundles only for reporting below

            ql::report::report_kernel_statistics(ofs, kernel, platform, "# ");
            std::stringstream ss;
            ss << "# ----- swaps added: " << mapper.nswapsadded << "\n";
            ss << "# ----- of which moves added: " << mapper.nmovesadded << "\n";
            ss << "# ----- virt2real map before mapper:" << ql::utils::to_string(mapper.v2r_in) << "\n";
            ss << "# ----- virt2real map after initial placement:" << ql::utils::to_string(mapper.v2r_ip) << "\n";
            ss << "# ----- virt2real map after mapper:" << ql::utils::to_string(mapper.v2r_out) << "\n";
            ss << "# ----- realqubit states before mapper:" << ql::utils::to_string(mapper.rs_in) << "\n";
            ss << "# ----- realqubit states after mapper:" << ql::utils::to_string(mapper.rs_out) << "\n";
            ss << "# ----- time taken: " << timetaken << "\n";
            ql::report::report_string(ofs, ss.str());

            total_swaps += mapper.nswapsadded;
            total_moves += mapper.nmovesadded;
            total_timetaken += timetaken;
        }
        ql::report::report_totals_statistics(ofs, kernels, platform, "# ");
        std::stringstream ss;
        ss << "# Total no. of swaps: " << total_swaps << "\n";
        ss << "# Total no. of moves of swaps: " << total_moves << "\n";
        ss << "# Total time taken: " << total_timetaken << "\n";
        ql::report::report_string(ofs, ss.str());
        ql::report::report_close(ofs);

        ql::report::report_bundles(prog_name, kernels, platform, "out", "mapper");
    }

    void schedule(std::string prog_name, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform, std::string opt)
    {
        ql::report::report_statistics(prog_name, kernels, platform, "in", "rcscheduler", "# ");
        ql::report::report_qasm(prog_name, kernels, platform, "in", "rcscheduler");

        for(auto &kernel : kernels)
        {
            IOUT("Scheduling kernel: " << kernel.name);
            if (! kernel.c.empty())
            {
                auto num_creg = kernel.creg_count;
                std::string     sched_dot;

                kernel.bundles = cc_light_schedule_rc(kernel.c, platform, sched_dot, num_qubits, num_creg);

                if (ql::options::get("print_dot_graphs") == "yes")
                {
                    std::stringstream fname;
                    fname << ql::options::get("output_dir") << "/" << kernel.name << "_" << opt << ".dot";
                    IOUT("writing " << opt << " dependence graph dot file to '" << fname.str() << "' ...");
                    ql::utils::write_file(fname.str(), sched_dot);
                }
            }
        }

        ql::report::report_statistics(prog_name, kernels, platform, "out", "rcscheduler", "# ");
        ql::report::report_bundles(prog_name, kernels, platform, "out", "rcscheduler");
    }

    /*
     * program-level compilation of qasm to cc_light_eqasm
     */
    void compile(std::string prog_name, ql::circuit& ckt, ql::quantum_platform& platform)
    {
        FATAL("cc_light_eqasm_compiler::compile interface with circuit not supported");
    }

    // kernel level compilation
    void compile(std::string prog_name, std::vector<quantum_kernel>& kernels,
        const ql::quantum_platform& platform)
    {
        DOUT("Compiling " << kernels.size() << " kernels to generate CCLight eQASM ... ");

        ql::report::report_qasm(prog_name, kernels, platform, "in0", "cc_light_compiler");

        load_hw_settings(platform);
        // check whether json instruction entries have cc_light_instr attribute
        const json& instruction_settings = platform.instruction_settings;
        for(const json & i : instruction_settings)
        {
            if(i.count("cc_light_instr") <= 0)
            {
                FATAL("cc_light_instr not found for " << i);
            }
        }
        ql::report::report_qasm(prog_name, kernels, platform, "in1", "cc_light_compiler");

        for(auto &kernel : kernels)
        {
            kernel.bundles.clear();         // circuit change coming; destroy bundles
            IOUT("Decomposing kernel: " << kernel.name);
            if (! kernel.c.empty())
            {
                // decompose meta-instructions
                ql::circuit decomposed_ckt;
                decompose_pre_schedule(kernel.c, decomposed_ckt, platform);
                kernel.c = decomposed_ckt;
            }
        }

        ql::report::report_qasm(prog_name, kernels, platform, "in", "cc_light_compiler");
        ql::report::report_statistics(prog_name, kernels, platform, "in", "cc_light_compiler", "# ");

        if (ql::options::get("quantumsim") == "yes")
            write_quantumsim_program(prog_name, num_qubits, kernels, platform, "");
		else if (ql::options::get("quantumsim") == "qsoverlay")
			write_qsoverlay_program(prog_name, num_qubits, kernels, platform, "", ns_per_cycle, false);

        // compute timetaken, start interval timer here
        double    total_timetaken = 0.0;
        using namespace std::chrono;
        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        clifford_optimize(prog_name, kernels, platform, "clifford_premapper");

        map(prog_name, kernels, platform);

        clifford_optimize(prog_name, kernels, platform, "clifford_postmapper");

        schedule(prog_name, kernels, platform, "rcscheduler");

        // computing timetaken, stop interval timer
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = t2 - t1;
        total_timetaken = time_span.count();

        // report totals over all kernels, over all eqasm passes contributing to mapping
        std::ofstream   ofs;
        ofs = ql::report::report_open(prog_name, "out", "cc_light_compiler");
        for (auto& k : kernels) { ql::report::report_kernel_statistics(ofs, k, platform, "# "); }
        ql::report::report_totals_statistics(ofs, kernels, platform, "# ");
        std::stringstream ss;
        ss << "# Total time taken: " << total_timetaken << "\n";
        ql::report::report_string(ofs, ss.str());
        ql::report::report_close(ofs);
        ql::report::report_bundles(prog_name, kernels, platform, "out", "cc_light_compiler");

        // decompose meta-instructions after scheduling
        for(auto &kernel : kernels)
        {
            IOUT("Decomposing meta-instructions kernel after post-scheduling: " << kernel.name);
            if (! kernel.c.empty())
            {
                decompose_post_schedule(kernel.bundles, platform);
                // after this, kernel.bundles is valid, kernel.circuit is old/invalid
            }
        }

        if (ql::options::get("quantumsim") == "yes")
            write_quantumsim_program(prog_name, num_qubits, kernels, platform, "mapped");
		else if (ql::options::get("quantumsim") == "qsoverlay")
			write_qsoverlay_program(prog_name, num_qubits, kernels, platform, "mapped", ns_per_cycle, true);

        // generate_opcode_cs_files(platform);

        // generate qisa
        MaskManager mask_manager;
        std::stringstream ssqisa, sskernels_qisa;
        sskernels_qisa << "start:" << std::endl;
        for(auto &kernel : kernels)
        {
            sskernels_qisa << "\n" << kernel.name << ":" << std::endl;
            sskernels_qisa << get_qisa_prologue(kernel);
            if (! kernel.c.empty())
            {
                sskernels_qisa << bundles2qisa(kernel.bundles, platform, mask_manager);
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
    void decompose_pre_schedule(ql::circuit& ckt, ql::circuit& decomp_ckt, const ql::quantum_platform& platform)
    {
        DOUT("decomposing instructions...");
        for( auto ins : ckt )
        {
            auto & iname =  ins->name;
            str::lower_case(iname);
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
    }


private:

    void load_hw_settings(const ql::quantum_platform& platform)
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
        catch (json::exception &e)
        {
            throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter '"+params[p-1]+"'\n\t"+ std::string(e.what()),false);
        }
    }

    void generate_opcode_cs_files(const ql::quantum_platform& platform)
    {
        DOUT("Generating opcode file ...");
        const json& instruction_settings       = platform.instruction_settings;

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
        for (const json & i : instruction_settings)
        {
            std::string instr_name;
            DOUT("Looking for instruction: " << i);
            if (i.count("cc_light_instr") <= 0)
            {
                EOUT("cc_light_instr not found for " << i);
                throw ql::exception("cc_light_instr not found for <> ", false);
            }
            else
            {
                instr_name = i["cc_light_instr"].get<std::string>();
            }
            DOUT("... instr_name=" << instr_name);

            if (i.count("cc_light_opcode") <= 0)
                throw ql::exception("[x] error : ql::eqasm_compiler::compile() : missing opcode for instruction '"+instr_name,false);
            else
                opcode = i["cc_light_opcode"];
            DOUT("... opcode=" << opcode);

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
            DOUT("..... mapit done mapt->second:" << mapit->second); DOUT(".....instr_name_2_opcode[instr_name]=" << instr_name_2_opcode[instr_name]);

            if (i["cc_light_instr_type"] == "single_qubit_gate")
            {
                DOUT("..... in single_qubit_gate ...");
                if (opcode_set.find(opcode) != opcode_set.end())
                    continue;

                // opcode range check
                DOUT("..... opcode found");
                if (i["type"] == "readout")
                {
                    if (opcode < 0x4 || opcode > 0x7)
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : invalid opcode for measure instruction '"+instr_name+"' : should be in [0x04..0x07] range : current opcode: "+std::to_string(opcode),false);
                }
                else if (opcode < 1 || opcode > 127)
                {
                    throw ql::exception("[x] error : ql::eqasm_compiler::compile() : invalid opcode for single qubit gate instruction '"+instr_name+"' : should be in [1..127] range : current opcode: "+std::to_string(opcode),false);
                }
                DOUT("..... inserting opcode in opcode_set ...");
                opcode_set.insert(opcode);

                size_t condition  = (i.count("cc_light_cond")<=0? 0 :i["cc_light_cond"].get<size_t>());
                DOUT("..... condition=" << condition);

                if (i.count("cc_light_instr") <=0 )
                    throw ql::exception("[x] error : ql::eqasm_compiler::compile() : 'cc_light_instr' attribute missing in gate definition (opcode: "+std::to_string(opcode),false);

                DOUT("..... composing constrol store line ...");
                DOUT("..... opcode_ss generation ...");
                opcode_ss << "def_q_arg_st[" << i["cc_light_instr"] << "]\t= " << std::showbase << std::hex << opcode << "\n";
                DOUT("..... optype computation...");
                auto optype     = (i["type"] == "mw" ? 1 : (i["type"] == "flux" ? 2 : ((i["type"] == "readout" ? 3 : 0))));
                DOUT("..... optype:" << optype);
                auto codeword   = i["cc_light_codeword"];
                DOUT("..... codeword:" << codeword);
                control_store << "     " << i["cc_light_opcode"] << ":     " << condition << "          " << optype << "          " << codeword << "          0          0\n";
                DOUT("..... done line");
            }
            else if (i["cc_light_instr_type"] == "two_qubit_gate")
            {
                size_t opcode     = i["cc_light_opcode"];
                if (opcode_set.find(opcode) != opcode_set.end())
                    continue;
                if (opcode < 127 || opcode > 255)
                    throw ql::exception("[x] error : ql::eqasm_compiler::compile() : invalid opcode for two qubits gate instruction '"+instr_name+"' : should be in [128..255] range : current opcode: "+std::to_string(opcode),false);
                opcode_set.insert(opcode);

                size_t condition  = (i.count("cc_light_cond") <= 0? 0 :i["cc_light_cond"].get<size_t>());

                if (i.count("cc_light_instr") <= 0)
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
        }

        DOUT("... writing cs.txt=" << opcode);
        std::string cs_filename = ql::options::get("output_dir") + "/cs.txt";
        IOUT("writing control store file to '" << cs_filename << "' ...");
        ql::utils::write_file(cs_filename, control_store.str());

        DOUT("... writing qisa_opcodes.qmap=" << opcode);
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

private:
    // write cc_light scheduled bundles for quantumsim
    // when cc_light independent, it should be extracted and put in src/quantumsim.h
    void write_quantumsim_program( std::string prog_name, size_t num_qubits,
        std::vector<quantum_kernel>& kernels, const ql::quantum_platform & platform, std::string suffix)
    {
        IOUT("Writing scheduled Quantumsim program");
        ofstream fout;
        string qfname( ql::options::get("output_dir") + "/" + "quantumsim_" + prog_name + "_" + suffix + ".py");
        DOUT("Writing scheduled Quantumsim program to " << qfname);
        IOUT("Writing scheduled Quantumsim program to " << qfname);
        fout.open( qfname, ios::binary);
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
             << endl;

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
             << endl;

        fout << "\n# create a circuit\n";
        fout << "def circuit_generated(t1=np.inf, t2=np.inf, dephasing_axis=None, dephasing_angle=None, dephase_var=0, readout_error=0.0) :\n";
        fout << "    c = Circuit(title=\"" << prog_name << "\")\n";

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
        MapperAssert (kernels.size() <= 1);
        std::vector<size_t> check_usecount;
        check_usecount.resize(count, 0);

        for (auto & gp: kernels.front().c)
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
            std::stringstream ssbundles;
            ssbundles << "\n    sampler = uniform_noisy_sampler(readout_error=readout_error, seed=42)\n";
            ssbundles << "\n    # add gates\n";
            fout << ssbundles.str();
        }
        for(auto &kernel : kernels)
        {
            DOUT("... adding gates, a new kernel");
            if (kernel.bundles.empty())
            {
                IOUT("No bundles for adding gates");
            }
            else
            {
                for ( ql::ir::bundle_t & abundle : kernel.bundles)
                {
                    DOUT("... adding gates, a new bundle");
                    auto bcycle = abundle.start_cycle;

                    std::stringstream ssbundles;
                    for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
                    {
                        DOUT("... adding gates, a new section in a bundle");
                        for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                        {
                            auto & iname = (*insIt)->name;
                            auto & operands = (*insIt)->operands;
                            auto duration = (*insIt)->duration;     // duration in nano-seconds
                            // size_t operation_duration = std::ceil( static_cast<float>(duration) / ns_per_cycle);
                            if( iname == "measure")
                            {
                                DOUT("... adding gates, a measure");
                                auto op = operands.back();
                                ssbundles << "    c.add_qubit(\"m" << op << "\")\n";
                                ssbundles << "    c.add_gate("
                                          << "ButterflyGate("
                                          << "\"q" << op <<"\", "
                                          << "time=" << ((bcycle-1)*ns_per_cycle) << ", "
                                          << "p_exc=0,"
                                          << "p_dec= 0.005)"
                                          << ")\n" ;
                                ssbundles << "    c.add_measurement("
                                    << "\"q" << op << "\", "
                                    << "time=" << ((bcycle - 1)*ns_per_cycle) + (duration/4) << ", "
                                    << "output_bit=\"m" << op << "\", "
                                    << "sampler=sampler"
                                    << ")\n";
                                ssbundles << "    c.add_gate("
                                    << "ButterflyGate("
                                    << "\"q" << op << "\", "
                                    << "time=" << ((bcycle - 1)*ns_per_cycle) + duration/2 << ", "
                                    << "p_exc=0,"
                                    << "p_dec= 0.015)"
                                    << ")\n";

                            }
                            else if( iname == "y90" or iname == "ym90" or iname == "y" or iname == "x" or
                                iname == "x90" or iname == "xm90")
                            {
                                DOUT("... adding gates, another gate");
                                ssbundles <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssbundles << "\"q" << *opit <<"\", ";
                                    ssbundles << "\"q" << operands.back()<<"\"";
                                }
                                ssbundles << ", time=" << ((bcycle - 1)*ns_per_cycle) + (duration/2) << ", dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle))" << endl;
                            }
                            else if( iname == "cz")
                            {
                               DOUT("... adding gates, another gate");
                                ssbundles <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssbundles << "\"q" << *opit <<"\", ";
                                    ssbundles << "\"q" << operands.back()<<"\"";
                                }
                                ssbundles << ", time=" << ((bcycle - 1)*ns_per_cycle) + (duration/2) << ", dephase_var=dephase_var))" << endl;
                            }
                            else
                            {
                                DOUT("... adding gates, another gate");
                                ssbundles <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssbundles << "\"q" << *opit <<"\", ";
                                    ssbundles << "\"q" << operands.back()<<"\"";
                                }
                                ssbundles << ", time=" << ((bcycle - 1)*ns_per_cycle) + (duration/2) << "))" << endl;
                            }
                        }
                    }
                    fout << ssbundles.str();
                }
                fout << "    return c";
                fout << "    \n\n";
                ql::report::report_kernel_statistics(fout, kernel, platform, "    # ");
            }
        }
        ql::report::report_string(fout, "    \n");
        ql::report::report_string(fout, "    # Program-wide statistics:\n");
        ql::report::report_totals_statistics(fout, kernels, platform, "    # ");
        fout << "    return c";

        fout.close();
        IOUT("Writing scheduled Quantumsim program [Done]");
    }

};
}
}

#endif // QL_CC_LIGHT_EQASM_COMPILER_H
