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

#include <src/utils.h>
#include <src/gate.h>
#include <src/ir.h>
#include <src/circuit.h>
#include <src/scheduler.h>
#include <src/arch/cc_light_resource_manager.h>

#include <iomanip>
#include <map>
#include <sstream>

using namespace std;
using namespace lemon;

namespace ql
{
namespace arch
{

std::string get_cc_light_instruction_name(std::string & id, const ql::quantum_platform & platform)
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

void DebugBundles(std::string at, ql::ir::bundles_t& bundles)
{
    DOUT("DebugBundles at: " << at << " showing " << bundles.size() << " bundles");
    for (ql::ir::bundle_t & abundle : bundles)
    {
        DOUT("... quantum bundle with nsections: " << abundle.parallel_sections.size());
        for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
        {
            DOUT("... section with ngates: " << secIt->size());
            for ( auto gp : (*secIt))
            {
                // auto n = get_cc_light_instruction_name(gp->name, platform);
                DOUT("... ... gate: " << gp->qasm() << " name: " << gp->name << " cc_light_iname: " << "?");
            }
        }
    }
}


ql::ir::bundles_t cc_light_schedule(ql::circuit & ckt, 
    ql::quantum_platform & platform, std::string & sched_dot, size_t nqubits, size_t ncreg = 0)
{
    IOUT("Scheduling CC-Light instructions ...");

    Scheduler sched;
    sched.Init(ckt, platform, nqubits, ncreg);
    // sched.PrintDot();
    ql::ir::bundles_t bundles1;
    std::string schedopt = ql::options::get("scheduler");
    if ("ASAP" == schedopt)
    {
        bundles1 = sched.schedule_asap(sched_dot);
    }
    else if ("ALAP" == schedopt)
    {
        bundles1 = sched.schedule_alap(sched_dot);
    }
    else
    {
        EOUT("Unknown scheduler");
        throw ql::exception("Unknown scheduler!", false);

    }

    // DebugBundles("After scheduling", bundles1);

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

    // DebugBundles("After combining", bundles1);

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

    // DebugBundles("After removing empty sections", bundles2);

    IOUT("Scheduling CC-Light instructions [Done].");
    return bundles2;
}

ql::ir::bundles_t cc_light_schedule_rc(ql::circuit & ckt, 
    const ql::quantum_platform & platform, std::string & sched_dot, size_t nqubits, size_t ncreg = 0)
{
    IOUT("Resource constraint scheduling of CC-Light instructions ...");

    scheduling_direction_t  direction;
    std::string schedopt = ql::options::get("scheduler");
    if ("ASAP" == schedopt)
    {
        direction = forward_scheduling;
    }
    else if ("ALAP" == schedopt)
    {
        direction = backward_scheduling;
    }
    else
    {
        EOUT("Unknown scheduler");
        throw ql::exception("Unknown scheduler!", false);

    }
    resource_manager_t rm(platform, direction);

    Scheduler sched;
    sched.Init(ckt, platform, nqubits, ncreg);
    ql::ir::bundles_t bundles1;
    if ("ASAP" == schedopt)
    {
        bundles1 = sched.schedule_asap(rm, platform, sched_dot);
    }
    else if ("ALAP" == schedopt)
    {
        bundles1 = sched.schedule_alap(rm, platform, sched_dot);
    }
    else
    {
        EOUT("Unknown scheduler");
        throw ql::exception("Unknown scheduler!", false);

    }

    IOUT("Combining parallel sections...");
    // DebugBundles("After scheduling_rc", bundles1);

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
    // DebugBundles("After combinging", bundles1);

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
    // DebugBundles("After removing empty sections", bundles2);

    IOUT("Resource constraint scheduling of CC-Light instructions [Done].");
    return bundles2;
}



} // end of namespace arch
} // end of namespace ql

#endif
