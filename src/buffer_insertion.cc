/** \file
 * Buffer insertion pass implementation.
 *
 * The intended functionality and the use of insertion of buffer delays are not
 * clear; see tests/test_cc_light.py for examples of use, though.
 *
 * Currently the functionality and code below strongly depend on bundles: only
 * a previous bundle and the current bundle are checked for a pair of operations
 * but the intended delay could be required between a bundler farther back
 * because the duration is longer so the current implementation may not do what
 * it intends. Below, the code is updated to modularity, but it is an exact copy
 * of what it was, with creating bundles from the circuit and updating the
 * circuit from the bundles around it. Once clarity is gained on intended
 * functionality and use, it can be rewritten and corrected.
 */

#include "utils/str.h"
#include "utils/map.h"
#include "utils/vec.h"
#include "utils/pair.h"
#include "kernel.h"
#include "circuit.h"
#include "ir.h"
#include "report.h"

#include "buffer_insertion.h"

namespace ql {

using namespace utils;

/*
*/
static void insert_buffer_delays_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform
) {
    QL_DOUT("Loading buffer settings ...");
    Map<Pair<Str, Str>, UInt> buffer_cycles_map;

    // populate buffer map
    // 'none' type is the default type in case a gate doesn't specify one in the config file
    // it a dummy type and 0 buffer cycles will be inserted for instructions of type 'none'
    // for pairs of instructions not represented in the buffer settings in the config file, 0 is inserted as well
    //
    // this has nothing to do with dependence graph generation but with scheduling
    // so should be in resource-constrained scheduler constructor

    Vec<Str> optype_names = {"none", "mw", "flux", "readout", "extern"};
    for (auto &buf1 : optype_names) {
        for (auto &buf2 : optype_names) {
            auto bname = buf1 + "_" + buf2 + "_buffer";
            if (platform.hardware_settings.count(bname) > 0) {
                buffer_cycles_map.set({buf1, buf2}) = UInt(ceil(
                    static_cast<float>(platform.hardware_settings[bname]) /
                    platform.cycle_time));
            } else {
                buffer_cycles_map.set({buf1, buf2}) = 0;
            }
            QL_DOUT("Initializing " << bname << ": "<< buffer_cycles_map.get({buf1, buf2}));
        }
    }

    QL_DOUT("Buffer-buffer delay insertion ... ");

    circuit *circp = &kernel.c;
    ir::bundles_t bundles = ir::bundler(*circp, platform.cycle_time);

    Vec<Str> optypes_prev_bundle;
    UInt buffer_cycles_accum = 0;
    for (ir::bundle_t &abundle : bundles) {
        Vec<Str> optypes_curr_bundle;    // map of all gates in this bundle to their types
        for (auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt) {
            for (auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt) {
                auto &id = (*insIt)->name;
                Str op_type("none");    // default type attribute
                if (platform.instruction_settings.count(id) > 0) {
                    // so gate is specified in config file
                    if (platform.instruction_settings[id].count("type") > 0) {
                        // and it has a type attribute
                        op_type = platform.instruction_settings[id]["type"].get<Str>();
                    }
                }
                optypes_curr_bundle.push_back(op_type);
            }
        }

        UInt buffer_cycles = 0; // max of buffer_cycles for all combinations of optypes in current and previous bundle
        for (auto &op_prev : optypes_prev_bundle) {
            for (auto &op_curr : optypes_curr_bundle) {
                auto temp_buf_cycles = buffer_cycles_map.get({op_prev, op_curr}, 0);
                QL_DOUT("... considering buffer_" << op_prev << "_" << op_curr
                                                  << ": " << temp_buf_cycles);
                buffer_cycles = max(temp_buf_cycles, buffer_cycles);
            }
        }
        QL_DOUT("... inserting buffer : " << buffer_cycles);
        buffer_cycles_accum += buffer_cycles;
        abundle.start_cycle = abundle.start_cycle + buffer_cycles_accum;
        optypes_prev_bundle = optypes_curr_bundle;
    }

    *circp = ir::circuiter(bundles);

    QL_DOUT("Buffer-buffer delay insertion [DONE] ");
}

void insert_buffer_delays(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    for (auto &kernel : programp->kernels) {
        insert_buffer_delays_kernel(kernel, platform);
    }

    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

} // namespace ql
