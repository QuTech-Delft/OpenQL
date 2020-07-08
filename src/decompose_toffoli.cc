/**
 * @file   decompose_toffoli.cc
 * @date   11/2016
 * @author Nader Khammassi
 */
#include "utils.h"
#include "circuit.h"
#include "kernel.h"
#include "decompose_toffoli.h"


namespace ql
{

    inline void decompose_toffoli_kernel(ql::quantum_kernel& kernel, const ql::quantum_platform & platform)
    {
        DOUT("decompose_toffoli_kernel()");
        for( auto cit = kernel.c.begin(); cit != kernel.c.end(); ++cit )
        {
            auto g = *cit;
            ql::gate_type_t gtype = g->type();
            std::vector<size_t> goperands = g->operands;

            ql::quantum_kernel toff_kernel("toff_kernel");
            toff_kernel.instruction_map = kernel.instruction_map;
            toff_kernel.qubit_count = kernel.qubit_count;
            toff_kernel.cycle_time = kernel.cycle_time;

            if( __toffoli_gate__ == gtype )
            {
                size_t cq1 = goperands[0];
                size_t cq2 = goperands[1];
                size_t tq = goperands[2];
                auto opt = ql::options::get("decompose_toffoli");
                if ( opt == "AM" )
                {
                    toff_kernel.controlled_cnot_AM(tq, cq1, cq2);
                }
                else
                {
                    toff_kernel.controlled_cnot_NC(tq, cq1, cq2);
                }
                ql::circuit& toff_ckt = toff_kernel.get_circuit();
                cit = kernel.c.erase(cit);
                cit = kernel.c.insert(cit, toff_ckt.begin(), toff_ckt.end());
                kernel.cycles_valid = false;
            }
        }
        DOUT("decompose_toffoli() [Done] ");
    }

    // decompose_toffoli pass
    void decompose_toffoli(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        auto tdopt = ql::options::get("decompose_toffoli");
        if( tdopt == "AM" || tdopt == "NC" )
        {
            IOUT("Decomposing Toffoli ...");
            for (size_t k=0; k<programp->kernels.size(); ++k)
            {
                decompose_toffoli_kernel(programp->kernels[k], platform);
            }
        }
        else if( tdopt == "no" )
        {
            IOUT("Not Decomposing Toffoli ...");
        }
        else
        {
            FATAL("Unknown option '" << tdopt << "' set for decompose_toffoli");
        }
    }
}
