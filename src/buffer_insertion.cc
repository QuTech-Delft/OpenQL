/**
 * @file   buffer_insertion.cc
 * @date   01/2017
 * @author Imran Ashraf
 * @author Hans van Someren
 * @brief  buffer insertion
 */

#include "utils.h"
#include "gate.h"
#include "kernel.h"
#include "circuit.h"
#include "ir.h"
#include "report.h"

#include "buffer_insertion.h"

namespace ql
{
    /*
        The intended functionality and the use of insertion of buffer delays are not clear;
        see tests/test_cc_light.py for examples of use, though.
        Currently the functionality and code below strongly depend on bundles:
        only a previous bundle and the current bundle are checked for a pair of operations
        but the intended delay could be required between a bundler farther back because the duration is longer
        so the current implementation may not do what it intends.
        Below, the code is updated to modularity, but it is an exact copy of what it was,
        with creating bundles from the circuit and updating the circuit from the bundles around it.
        Once clarity is gained on intended functionality and use, it can be rewritten and corrected.
    */
    void insert_buffer_delays_kernel(ql::quantum_kernel& kernel, const ql::quantum_platform & platform)
    {
        DOUT("Loading buffer settings ...");
        std::map< std::pair<std::string,std::string>, size_t> buffer_cycles_map;

        // populate buffer map
        // 'none' type is a dummy type and 0 buffer cycles will be inserted for
        // instructions of type 'none'
        //
        // this has nothing to do with dependence graph generation but with scheduling
        // so should be in resource-constrained scheduler constructor

        std::vector<std::string> buffer_names = {"none", "mw", "flux", "readout"};
        for(auto & buf1 : buffer_names)
        {
            for(auto & buf2 : buffer_names)
            {
                auto bpair = std::pair<std::string,std::string>(buf1,buf2);
                auto bname = buf1+ "_" + buf2 + "_buffer";
                if(platform.hardware_settings.count(bname) > 0)
                {
                    buffer_cycles_map[ bpair ] = size_t(std::ceil(
                        static_cast<float>(platform.hardware_settings[bname]) / platform.cycle_time));
                }
                // DOUT("Initializing " << bname << ": "<< buffer_cycles_map[bpair]);
            }
        }

        DOUT("Buffer-buffer delay insertion ... ");

        ql::circuit* circp = &kernel.c;
        ql::ir::bundles_t bundles = ql::ir::bundler(*circp, platform.cycle_time);

        std::vector<std::string> operations_prev_bundle;
        size_t buffer_cycles_accum = 0;
        for(ql::ir::bundle_t & abundle : bundles)
        {
            std::vector<std::string> operations_curr_bundle;
            for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
            {
                for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                {
                    auto & id = (*insIt)->name;
                    std::string op_type("none");
                    if(platform.instruction_settings.count(id) > 0)
                    {
                        if(platform.instruction_settings[id].count("type") > 0)
                        {
                            op_type = platform.instruction_settings[id]["type"].get<std::string>();
                        }
                    }
                    operations_curr_bundle.push_back(op_type);
                }
            }

            size_t buffer_cycles = 0;
            for(auto & op_prev : operations_prev_bundle)
            {
                for(auto & op_curr : operations_curr_bundle)
                {
                    auto temp_buf_cycles = buffer_cycles_map[ std::pair<std::string,std::string>(op_prev, op_curr) ];
                    DOUT("... considering buffer_" << op_prev << "_" << op_curr << ": " << temp_buf_cycles);
                    buffer_cycles = std::max(temp_buf_cycles, buffer_cycles);
                }
            }
            DOUT( "... inserting buffer : " << buffer_cycles);
            buffer_cycles_accum += buffer_cycles;
            abundle.start_cycle = abundle.start_cycle + buffer_cycles_accum;
            operations_prev_bundle = operations_curr_bundle;
        }

        *circp = ql::ir::circuiter(bundles);

        DOUT("Buffer-buffer delay insertion [DONE] ");
    }

    void insert_buffer_delays(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        ql::report_statistics(programp, platform, "in", passname, "# ");
        ql::report_qasm(programp, platform, "in", passname);

        for (size_t k=0; k<programp->kernels.size(); ++k)
        {
            insert_buffer_delays_kernel(programp->kernels[k], platform);
        }

        ql::report_statistics(programp, platform, "out", passname, "# ");
        ql::report_qasm(programp, platform, "out", passname);
    }
}
