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
void cc_light_schedule_rc(ql::circuit & ckt, 
    const ql::quantum_platform & platform, std::string & dot, size_t nqubits, size_t ncreg = 0)
{
    IOUT("Resource constraint scheduling of CC-Light instructions ...");

    std::string schedopt = ql::options::get("scheduler");
    if ("ASAP" == schedopt)
    {
        Scheduler sched;
        sched.init(ckt, platform, nqubits, ncreg);

        resource_manager_t rm(platform, forward_scheduling);
        sched.schedule_asap(rm, platform, dot);
    }
    else if ("ALAP" == schedopt)
    {
        Scheduler sched;
        sched.init(ckt, platform, nqubits, ncreg);

        resource_manager_t rm(platform, backward_scheduling);
        sched.schedule_alap(rm, platform, dot);
    }
    else
    {
        FATAL("Not supported scheduler option: scheduler=" << schedopt);
    }

    IOUT("Resource constraint scheduling of CC-Light instructions [Done].");
}



} // end of namespace arch
} // end of namespace ql

#endif
