/**
 * @file   latency_compensation.cc
 * @date   01/2017
 * @author Imran Ashraf
 * @author Hans van Someren
 * @brief  latency compensation
 */

#include "utils.h"
#include "gate.h"
#include "kernel.h"
#include "circuit.h"
#include "ir.h"
#include "report.h"

#include "latency_compensation.h"

namespace ql
{
    static bool lc_cycle_lessthan(ql::gate* gp1, ql::gate* gp2)
    {
        return gp1->cycle < gp2->cycle;
    }

    // sort circuit by the gates' cycle attribute in non-decreasing order
    static void lc_sort_by_cycle(ql::circuit *cp)
    {
        // std::sort doesn't preserve the original order of elements that have equal values but std::stable_sort does
        std::stable_sort(cp->begin(), cp->end(), lc_cycle_lessthan);
    }

    void latency_compensation_kernel(ql::quantum_kernel& kernel, const ql::quantum_platform & platform)
    {
        DOUT("Latency compensation ...");

        ql::circuit* circp = &kernel.c;

        bool    compensated_one = false;
        for ( auto & gp : *circp)
        {
            auto & id = gp->name;
            // DOUT("Latency compensating instruction: " << id);
            long latency_cycles=0;

            if(platform.instruction_settings.count(id) > 0)
            {
                if(platform.instruction_settings[id].count("latency") > 0)
                {
                    float latency_ns = platform.instruction_settings[id]["latency"];
                    latency_cycles = long(std::ceil( static_cast<float>(std::abs(latency_ns)) / platform.cycle_time)) *
                                          ql::utils::sign_of(latency_ns);
                    compensated_one = true;

                    gp->cycle = gp->cycle + latency_cycles;
                    DOUT( "... compensated to @" << gp->cycle << " <- " << id << " with " << latency_cycles );
                }
            }
        }

        if (compensated_one)
        {
            DOUT("... sorting on cycle value after latency compensation");
            lc_sort_by_cycle(circp);

            DOUT("... printing schedule after latency compensation");
            for ( auto & gp : *circp)
            {
                DOUT("...... @(" << gp->cycle << "): " << gp->qasm());
            }
        }
        else
        {
            DOUT("... no gate latency compensated");
        }
        DOUT("Latency compensation [DONE]");
    }

    void latency_compensation(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        ql::report_statistics(programp, platform, "in", passname, "# ");
        ql::report_qasm(programp, platform, "in", passname);

        for (size_t k=0; k<programp->kernels.size(); ++k)
        {
            latency_compensation_kernel(programp->kernels[k], platform);
        }

        ql::report_statistics(programp, platform, "out", passname, "# ");
        ql::report_qasm(programp, platform, "out", passname);
    }
}
