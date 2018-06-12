/**
 * @file   cc_light_scheduler.h
 * @date   08/2017
 * @author Imran Ashraf
 * @brief  resource-constraint scheduler and code generator for cc-light
 */

#ifndef CC_LIGHT_SCHEDULER_H
#define CC_LIGHT_SCHEDULER_H

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>

#include <ql/utils.h>
#include <ql/gate.h>
#include <ql/ir.h>
#include <ql/circuit.h>
#include <ql/scheduler.h>
#include <ql/arch/cc_light_resource_manager.h>

#include <iomanip>
#include <map>
#include <sstream>

using namespace std;
using namespace lemon;

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


void PrintBundles(ql::ir::bundles_t & bundles)
{
    COUT("Printing simplified CC-Light QISA");

    for (ql::ir::bundle_t & abundle : bundles)
    {
        std::cout << abundle.start_cycle << "  ";

        for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
        {
            for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
            {
                std::cout << (*insIt)->qasm();
                if( std::next(insIt) != secIt->end() )
                {
                    std::cout << " , ";
                }
            }
            if( std::next(secIt) != abundle.parallel_sections.end() )
            {
                std::cout << " | ";
            }
        }
        std::cout << "\n";
    }
}

std::string get_cc_light_instruction_name(std::string & id, ql::quantum_platform & platform)
{
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
        // DOUT("cc_light_instr name: " << cc_light_instr_name);
    }
    else
    {
        EOUT("custom instruction not found for : " << id << " !");
        throw ql::exception("Error : custom instruction not found for : "+id+" !",false);
    }
    return cc_light_instr_name;
}

std::string classical_instruction2qisa(ql::classical* classical_ins)
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
        if(iname == "fmr")
        {
            ssclassical << " r" << iopers[0] << ", q" << iopers[1];
        }
        else
        {
            for(int i=0; i<iopers_count; ++i)
            {
                if(i==iopers_count-1)
                    ssclassical << " r" <<  iopers[i];
                else
                    ssclassical << " r" << iopers[i] << ",";
            }
            if(iname == "ldi")
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

    std::stringstream ssbundles;
    size_t curr_cycle=0;

    for (ql::ir::bundle_t & abundle : bundles)
    {
        auto bcycle = abundle.start_cycle;
        auto delta = bcycle - curr_cycle;

        if(delta < 8)
            ssbundles << "    bs " << delta << "    ";
        else
            ssbundles << "    qwait " << delta-1 << "\n"
                      << "    bs 1    ";

        for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
        {
            qubit_set_t squbits;
            qubit_pair_set_t dqubits;
            auto firstInsIt = secIt->begin();
            auto iname = (*(firstInsIt))->name;
            auto itype = (*(firstInsIt))->type();

            if(__classical_gate__ == itype)
            {
                ssbundles << classical_instruction2qisa( (ql::classical *)(*firstInsIt) );
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
                    // DOUT("cc_light_instr name: " << cc_light_instr_name);
                }
                else
                {
                    EOUT("custom instruction not found for : " << id << " !");
                    throw ql::exception("Error : custom instruction not found for : "+id+" !",false);
                }

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
        }
        curr_cycle+=delta;
        ssbundles << "\n";
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


ql::ir::bundles_t cc_light_schedule(ql::circuit & ckt, 
    ql::quantum_platform & platform, size_t nqubits)
{
    IOUT("Scheduling CC-Light instructions ...");
    Scheduler sched;
    sched.Init(ckt, platform, nqubits, 0); //TODO creg_count is 0 for now
    // sched.PrintDot();
    ql::ir::bundles_t bundles1 = sched.schedule_asap();

    // combine parallel instrcutions of same type from different sections
    // into a single section
    for (ql::ir::bundle_t & abundle : bundles1)
    {
        auto secIt1 = abundle.parallel_sections.begin();
        auto firstInsIt = secIt1->begin();
        auto itype = (*(firstInsIt))->type();
        
        // TODO check it again
        if(__classical_gate__ == itype)
        {
            continue;
        }
        else
        {
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
                        auto n1 = get_cc_light_instruction_name(id1, platform);
                        auto n2 = get_cc_light_instruction_name(id2, platform);
                        if( n1 == n2 )
                        {
                            // DOUT("splicing " << n1 << " and " << n2);
                            (*secIt1).splice(insIt1, (*secIt2) );
                        }
                        // else
                        // {
                        //     DOUT("Not splicing " << n1 << " and " << n2);
                        // }
                    }
                }
            }
        }
    }

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

    IOUT("Scheduling CC-Light instructions [Done].");
    return bundles2;
}


ql::ir::bundles_t cc_light_schedule_rc(ql::circuit & ckt, 
    ql::quantum_platform & platform, size_t nqubits)
{
    IOUT("Resource constraint scheduling of CC-Light instructions ...");
    resource_manager_t rm(platform);
    Scheduler sched;
    sched.Init(ckt, platform, nqubits, 0); //TODO creg_count is 0 for now
    ql::ir::bundles_t bundles1 = sched.schedule_asap(rm, platform);

    // combine parallel instrcutions of same type from different sections
    // into a single section
    for (ql::ir::bundle_t & abundle : bundles1)
    {
        auto secIt1 = abundle.parallel_sections.begin();
        auto firstInsIt = secIt1->begin();
        auto itype = (*(firstInsIt))->type();

        if(__classical_gate__ == itype)
        {
            continue;
        }
        else
        {
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
                        auto n1 = get_cc_light_instruction_name(id1, platform);
                        auto n2 = get_cc_light_instruction_name(id2, platform);
                        if( n1 == n2 )
                        {
                            DOUT("splicing " << n1 << " and " << n2);
                            (*secIt1).splice(insIt1, (*secIt2) );
                        }
                        else
                        {
                            DOUT("Not splicing " << n1 << " and " << n2);
                        }
                    }
                }
            }
        }
    }

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

    IOUT("Resource constraint scheduling of CC-Light instructions [Done].");
    return bundles2;
}



} // end of namespace arch
} // end of namespace ql

#endif
