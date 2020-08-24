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
#include <scheduler.h>
#include <mapper.h>
#include <clifford.h>
#include <latency_compensation.h>
#include <buffer_insertion.h>
#include <qsoverlay.h>

// eqasm code : set of cc_light_eqasm instructions
typedef std::vector<ql::arch::cc_light_eqasm_instr_t> eqasm_t;

static void ccl_assert_fail(const char *f, int l, const char *s)
{
    FATAL("assert " << s << " failed in file " << f << " at line " << l);
}
#define CclAssert(condition)   { if (!(condition)) { ccl_assert_fail(__FILE__, __LINE__, #condition); } }

namespace ql
{
namespace arch
{

typedef std::vector<size_t>        qubit_set_t;
typedef std::pair<size_t,size_t>   qubit_pair_t;
typedef std::vector<qubit_pair_t>  qubit_pair_set_t;

const size_t MAX_S_REG =32;
const size_t MAX_T_REG =64;

extern size_t CurrSRegCount;
extern size_t CurrTRegCount;

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

static std::string classical_instruction2qisa(ql::arch::classical_cc* classical_ins)
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

// FIXME HvS cc_light_instr is name of attribute in json file, in gate: arch_operation_name, here in instruction_map?
// FIXME HvS attribute of gate or just in json? Generalization to arch_operation_name is unnecessary
inline std::string get_cc_light_instruction_name(std::string & id, const ql::quantum_platform & platform)
{
    std::string cc_light_instr_name;
    auto it = platform.instruction_map.find(id);
    if (it != platform.instruction_map.end())
    {
        custom_gate* g = it->second;
        cc_light_instr_name = g->arch_operation_name;
        if(cc_light_instr_name.empty())
        {
            FATAL("cc_light_instr not defined for instruction: " << id << " !");
        }
        // DOUT("cc_light_instr name: " << cc_light_instr_name);
    }
    else
    {
        FATAL("custom instruction not found for : " << id << " !");
    }
    return cc_light_instr_name;
}

static std::string ir2qisa(quantum_kernel & kernel,
    const ql::quantum_platform & platform, MaskManager & gMaskManager)
{
    IOUT("Generating CC-Light QISA");

    ql::ir::bundles_t   bundles1;
    CclAssert(kernel.cycles_valid);
    bundles1 = ql::ir::bundler(kernel.c, platform.cycle_time);

    IOUT("Combining parallel sections...");
    // combine parallel instructions of same type from different sections into a single section
    // this prepares for SIMD; each section will be a SIMD; of a quantum SIMD all operands are combined in a mask
    ql::ir::DebugBundles("Before combining parallel sections", bundles1);
    for (ql::ir::bundle_t & abundle : bundles1)
    {
        auto secIt1 = abundle.parallel_sections.begin();

        for( ; secIt1 != abundle.parallel_sections.end(); ++secIt1 )
        {
            for( auto secIt2 = std::next(secIt1); secIt2 != abundle.parallel_sections.end(); ++secIt2)
            {
                auto insIt1 = secIt1->begin();
                auto insIt2 = secIt2->begin();
                if(insIt1 != secIt1->end() && insIt2 != secIt2->end() )
                {
                    auto id1 = (*insIt1)->name;
                    auto id2 = (*insIt2)->name;
                    auto itype1 = (*insIt1)->type();
                    auto itype2 = (*insIt2)->type();
                    if (itype1 == __classical_gate__ || itype2 == __classical_gate__)
                    {
                        DOUT("Not splicing " << id1 << " and " << id2);
                        continue;
                    }

                    auto n1 = get_cc_light_instruction_name(id1, platform);
                    auto n2 = get_cc_light_instruction_name(id2, platform);
                    if( n1 == n2 )
                    {
                        DOUT("Splicing " << id1 << "/" << n1 << " and " << id2 << "/" << n2);
                        (*secIt1).splice(insIt1, (*secIt2) );
                    }
                    else
                    {
                        DOUT("Not splicing " << id1 << "/" << n1 << " and " << id2 << "/" << n2);
                    }
                }
            }
        }
    }
    ql::ir::DebugBundles("After combining", bundles1);
    IOUT("Removing empty sections...");
    // remove empty sections
    ql::ir::bundles_t bundles2;
    for (ql::ir::bundle_t & abundle1 : bundles1)
    {
        ql::ir::bundle_t abundle2;
        abundle2.start_cycle = abundle1.start_cycle;
        abundle2.duration_in_cycles = abundle1.duration_in_cycles;
        for(auto & sec : abundle1.parallel_sections)
        {
            if( !sec.empty() )
            {
                abundle2.parallel_sections.push_back(sec);
            }
        }
        bundles2.push_back(abundle2);
    }
    bundles1.clear();
    ql::ir::DebugBundles("After removing empty sections", bundles2);

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
    for (ql::ir::bundle_t & abundle : bundles2)
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

    // And now generate qisa
    // each section of a bundle will become a SIMD (all operations in a section are the same, see above)
    // for the operands of the SIMD, a mask will be used
    //
    // kernel prologue (start label) and epilogue are generated by the caller or ir2qisa
    std::stringstream ssqisa;   // output qisa in here
    size_t curr_cycle=0; // first instruction should be with pre-interval 1, 'bs 1' FIXME HvS start in cycle 0
    for (ql::ir::bundle_t & abundle : bundles2)
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
                    ssqisa << "    qwait " << 1 << "\n";
                    ssqisa << "    qwait " << delta-1 << "\n";
                }
                else
                {
                    ssqisa << "    qwait " << 1 << "\n";
                    ssqisa << "    qwait " << 1 << "\n";
                }
            }
            else
            {
                if(delta > 1)
                    ssqisa << "    qwait " << delta << "\n";
            }
            ssqisa << "    " << ssinst.str() << "\n";
        }
        else
        {
            // FIXME HvS next addition could be an option, assuming # comment convention of qisa
            // ssqisa << sspre.str() << ssinst.str() << "\t\t# @" << bcycle << "\n";
            ssqisa << sspre.str() << ssinst.str() << "\n";
        }
        curr_cycle+=delta;
    }

    auto & lastBundle = bundles2.back();
    int lbduration = lastBundle.duration_in_cycles;
    if(lbduration>1)
        ssqisa << "    qwait " << lbduration << "\n";

    IOUT("Generating CC-Light QISA [Done]");
    return ssqisa.str();
}

/**
 * cclight eqasm compiler
 */
class cc_light_eqasm_compiler : public eqasm_compiler
{
public:

    cc_light_eqasm_program_t cc_light_eqasm_instructions;
    size_t          total_exec_time = 0;

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

    void ccl_decompose_pre_schedule(quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        ql::report_statistics(programp, platform, "in", passname, "# ");
        ql::report_qasm(programp, platform, "in", passname);

        for(auto &kernel : programp->kernels)
        {
	    ccl_decompose_pre_schedule_kernel(kernel, platform);
        }

        ql::report_statistics(programp, platform, "out", passname, "# ");
        ql::report_qasm(programp, platform, "out", passname);
    }

    void ccl_decompose_post_schedule(quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        ql::report_statistics(programp, platform, "in", passname, "# ");
        ql::report_qasm(programp, platform, "in", passname);

        for(auto &kernel : programp->kernels)
        {
            IOUT("Decomposing meta-instructions kernel after post-scheduling: " << kernel.name);
            if (! kernel.c.empty())
            {
                CclAssert(kernel.cycles_valid);
                ql::ir::bundles_t bundles = ql::ir::bundler(kernel.c, platform.cycle_time);
                ccl_decompose_post_schedule_bundles(bundles, platform);
                kernel.c = ql::ir::circuiter(bundles);
                CclAssert(kernel.cycles_valid);
            }
        }
        ql::report_statistics(programp, platform, "out", passname, "# ");
        ql::report_qasm(programp, platform, "out", passname);
    }

    void ccl_decompose_post_schedule_bundles(ql::ir::bundles_t & bundles_dst,
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
        fout.open( qisafname, ios::binary);
        if ( fout.fail() )
        {
            EOUT("opening file " << qisafname << std::endl
                     << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }
        fout << ssqisa.str() << endl;
        fout.close();
        // end qisa_generation pass
    }

private:
    // write cc_light scheduled bundles for quantumsim
    // when cc_light independent, it should be extracted and put in src/quantumsim.h
    void write_quantumsim_program( quantum_program* programp, size_t num_qubits, const ql::quantum_platform & platform, std::string suffix)
    {
        IOUT("Writing scheduled Quantumsim program");
        ofstream fout;
        string qfname( ql::options::get("output_dir") + "/" + "quantumsim_" + programp->unique_name + "_" + suffix + ".py");
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
        MapperAssert (programp->kernels.size() <= 1);
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
            CclAssert(kernel.cycles_valid);
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
                            else if( iname == "y90" or iname == "ym90" or iname == "y" or iname == "x" or
                                iname == "x90" or iname == "xm90")
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
                                ssqs << ", time=" << ((bcycle - 1)*platform.cycle_time) + (duration/2) << ", dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle))" << endl;
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
                                ssqs << ", time=" << ((bcycle - 1)*platform.cycle_time) + (duration/2) << ", dephase_var=dephase_var))" << endl;
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
                                ssqs << ", time=" << ((bcycle - 1)*platform.cycle_time) + (duration/2) << "))" << endl;
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
}
}

#endif // QL_CC_LIGHT_EQASM_COMPILER_H
