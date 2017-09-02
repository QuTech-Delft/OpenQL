/**
 * @file   cc_light_scheduler.h
 * @date   08/2017
 * @author Imran Ashraf
 * @brief  ASAP/AlAP scheduler for cc light
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
#include <ql/circuit.h>
#include <ql/dependenceGraph.h>

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
typedef std::string                mask_t;

size_t CurrRegCount=0;

class Mask
{
public:
    qubit_set_t qubits;
    size_t regNo;
    std::string regName;
    Mask() {}
    Mask(qubit_set_t & qs) : qubits(qs) 
    { 
        if(CurrRegCount < 32)
        {
            regNo = CurrRegCount++;
            regName = "s" + std::to_string(regNo);
        }
        else
        {
            println(" !!!! Handle cases requiring more registers");
        }
    }

    Mask(qubit_set_t & qs, std::string rn) : qubits(qs), regName(rn)
    { 
        if(CurrRegCount < 32)
        {
            regNo = CurrRegCount++;
        }
        else
        {
            println(" !!!! Handle cases requiring more registers");
        }
    }

};

std::map<size_t,Mask> Reg2Mask;
std::map<qubit_set_t,Mask> QS2Mask;

static class MaskManager
{
public:
    MaskManager()
    {
        for(size_t i=0; i<7; ++i)
        {
            qubit_set_t qs;
            qs.push_back(i);
            Mask m(qs);
            QS2Mask[qs] = m;
            Reg2Mask[m.regNo] = m;
        }

        {
            qubit_set_t qs;
            for(auto i=0; i<7; i++) qs.push_back(i);
            Mask m(qs, "all_qubits");
            QS2Mask[qs] = m;
            Reg2Mask[m.regNo] = m;
        }

        {
            qubit_set_t qs;
            qs.push_back(0); qs.push_back(1); qs.push_back(5); qs.push_back(6);
            Mask m(qs, "data_qubits");
            QS2Mask[qs] = m;
            Reg2Mask[m.regNo] = m;
        }

        {
            qubit_set_t qs;
            qs.push_back(2); qs.push_back(3); qs.push_back(4);
            Mask m(qs, "ancilla_qubits");
            QS2Mask[qs] = m;
            Reg2Mask[m.regNo] = m;
        }
    }

    size_t getRegNo( qubit_set_t & qs )
    {
        auto it = QS2Mask.find(qs);
        if( it == QS2Mask.end() )
        {
            Mask m(qs);
            QS2Mask[qs] = m;
            Reg2Mask[m.regNo] = m;
        }
        return QS2Mask[qs].regNo;
    }

    std::string getRegName( qubit_set_t & qs )
    {
        auto it = QS2Mask.find(qs);
        if( it == QS2Mask.end() )
        {
            Mask m(qs);
            QS2Mask[qs] = m;
            Reg2Mask[m.regNo] = m;
        }
        return QS2Mask[qs].regName;
    }

    std::string getMaskInstructions()
    {
        std::stringstream ssmasks;
        for(size_t r=0; r<CurrRegCount; ++r)
        {
            auto & m = Reg2Mask[r];
            ssmasks << "smis " << m.regName << " , { ";
            for(auto it = m.qubits.begin(); it != m.qubits.end(); ++it)
            {
                ssmasks << *it;
                if( std::next(it) != m.qubits.end() )
                    ssmasks << ", ";
            }
            ssmasks << " } \n";
        }
        return ssmasks.str();
    }

} gMaskManager;



// typedef std::list<ql::gate*>ParallelSection;
// class Bundle
// {
// public:
//     size_t cycle;
//     std::list<ParallelSection>ParallelSections;
// };
// typedef std::list<Bundle>Bundles;

void PrintBundles(Bundles & bundles)
{
    for (Bundle & abundle : bundles)
    {
        std::cout << abundle.cycle << "  ";

        for( auto secIt = abundle.ParallelSections.begin(); secIt != abundle.ParallelSections.end(); ++secIt )
        {
            for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
            {
                std::cout << (*insIt)->qasm();
                if( std::next(insIt) != secIt->end() )
                {
                    std::cout << " , ";
                }
            }
            if( std::next(secIt) != abundle.ParallelSections.end() )
            {
                std::cout << " | ";
            }
        }
        std::cout << "\n";
    }

    // for (Bundle & abundle : bundles)
    // {
    //     std::cout << abundle.cycle << "  ";

    //     for(auto & sec : abundle.ParallelSections)
    //     {
    //         for(auto & ins: sec)
    //         {
    //             std::cout << ins->qasm() << " , ";
    //         }
    //         std::cout << "|";
    //     }
    //     std::cout << "\n";
    // }

}

void PrintCCLighQasm(Bundles & bundles)
{
    std::stringstream ssbundles;
    size_t curr_cycle=1;

    for (Bundle & abundle : bundles)
    {
        auto bcycle = abundle.cycle;
        auto delta = bcycle - curr_cycle;

        if(delta < 8)
            ssbundles << "bs " << delta << "    ";
        else
            ssbundles << "qwait " << delta-1 << "\n"
                      << "bs 1    ";

        for( auto secIt = abundle.ParallelSections.begin(); secIt != abundle.ParallelSections.end(); ++secIt )
        {
            qubit_set_t qs;
            for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
            {
                for ( auto & op : (*insIt)->operands )
                {
                    qs.push_back(op);
                }
            }
            auto firstInsIt = secIt->begin();
            auto n = (*(firstInsIt))->name;
            auto t = (*(firstInsIt))->type();
            if(t == __nop_gate__)
            {
                ssbundles << n;
            }
            else
            {
                auto rname = gMaskManager.getRegName(qs);
                ssbundles << n << " " << rname;
            }

            if( std::next(secIt) != abundle.ParallelSections.end() )
            {
                ssbundles << " | ";
            }
        }
        curr_cycle+=delta;
        ssbundles << "\n";
    }

    std::cout << gMaskManager.getMaskInstructions() << endl << ssbundles.str() << endl;
}

void cc_light_schedule(size_t nqubits, ql::circuit & ckt, ql::quantum_platform & platform, bool verbose=true)
{
    Bundles bundles1;

    println("scheduling ccLight instructions ...");
    DependGraph dg;
    dg.Init(nqubits, ckt, platform, verbose);
    bundles1 = dg.GetBundlesScheduleALAP();

    for (Bundle & abundle : bundles1)
    {
        for( auto secIt1 = abundle.ParallelSections.begin(); secIt1 != abundle.ParallelSections.end(); ++secIt1 )
        {
            for( auto secIt2 = std::next(secIt1); secIt2 != abundle.ParallelSections.end(); ++secIt2)
            {
                auto insIt1 = secIt1->begin();
                auto insIt2 = secIt2->begin();
                if(insIt1 != secIt1->end() && insIt2 != secIt2->end() )
                {
                    if( (*insIt1)->type() == (*insIt2)->type() )
                    {
                        // std::cout << (*insIt1)->qasm() << " same " << (*insIt2)->qasm() << endl;
                        (*secIt1).splice(insIt1, (*secIt2) );
                    }
                    // else
                    // {
                    //     std::cout << (*insIt1)->qasm() << " different " << (*insIt2)->qasm() << endl;
                    // }
                }
            }
        }
    }

    // remove empty sections
    Bundles bundles2;
    for (Bundle & abundle1 : bundles1)
    {
        Bundle abundle2;
        abundle2.cycle = abundle1.cycle;
        for(auto & sec : abundle1.ParallelSections)
        {
            if( !sec.empty() )
            {
                abundle2.ParallelSections.push_back(sec);
            }
        }
        bundles2.push_back(abundle2);
    }

    // print scheduled bundles with parallelism
    PrintBundles(bundles2);

    // print scheduled bundles with parallelism in cc-light syntax
    PrintCCLighQasm(bundles2);

    println("scheduling ccLight instructions done.");
}


} // end of namespace arch
} // end of namespace ql

#endif
