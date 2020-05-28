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

#include <utils.h>
#include <gate.h>
#include <ir.h>
#include <circuit.h>
#include <scheduler.h>
#include <resource_manager.h>

#include <iomanip>
#include <map>
#include <sstream>

using namespace std;
using namespace lemon;

namespace ql
{
namespace arch
{
// FIXME: superseded by platform.h::get_instruction_name()
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

ql::ir::bundles_t cc_light_schedule(ql::circuit & ckt, 
    const ql::quantum_platform & platform, std::string & dot, size_t nqubits, size_t ncreg = 0)
{
    IOUT("Scheduling CC-Light instructions ...");

    Scheduler sched;
    sched.init(ckt, platform, nqubits, ncreg);

    ql::ir::bundles_t bundles1;
    std::string schedopt = ql::options::get("scheduler");
    if ("ASAP" == schedopt)
    {
        bundles1 = sched.schedule_asap(dot);
    }
    else if ("ALAP" == schedopt)
    {
        bundles1 = sched.schedule_alap(dot);
    }
    else
    {
        EOUT("Unknown scheduler");
        throw ql::exception("Unknown scheduler!", false);

    }

    ql::ir::DebugBundles("After scheduling", bundles1);

    // combine parallel instrcutions of same type from different sections
    // into a single section
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
                    // else
                    // {
                    //     DOUT("Not splicing " << id1 << "/" << n1 << " and " << id2 << "/" << n2);
                    // }
                }
            }
        }
    }

    ql::ir::DebugBundles("After combining", bundles1);

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

    ql::ir::DebugBundles("After removing empty sections", bundles2);

    IOUT("Scheduling CC-Light instructions [Done].");
    return bundles2;
}

ql::ir::bundles_t cc_light_schedule_rc(ql::circuit & ckt, 
    const ql::quantum_platform & platform, std::string & dot, size_t nqubits, size_t ncreg = 0)
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
    sched.init(ckt, platform, nqubits, ncreg);
    ql::ir::bundles_t bundles1;
    if ("ASAP" == schedopt)
    {
        bundles1 = sched.schedule_asap(rm, platform, dot);
    }
    else if ("ALAP" == schedopt)
    {
        bundles1 = sched.schedule_alap(rm, platform, dot);
    }
    else
    {
        EOUT("Unknown scheduler");
        throw ql::exception("Unknown scheduler!", false);

    }

    IOUT("Combining parallel sections...");
    ql::ir::DebugBundles("After scheduling_rc", bundles1);

    // combine parallel instrcutions of same type from different sections
    // into a single section
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
    ql::ir::DebugBundles("After combinging", bundles1);

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
    ql::ir::DebugBundles("After removing empty sections", bundles2);

    IOUT("Resource constraint scheduling of CC-Light instructions [Done].");
    return bundles2;
}



} // end of namespace arch
} // end of namespace ql

#endif
