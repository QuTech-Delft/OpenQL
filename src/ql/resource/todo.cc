/** \file
 * Resources copied from the CC-light platform.
 *
 * FIXME: needs cleanup, generalization, and conversion to new resource types,
 *  including support for the "undefined" direction (since the mapper was
 *  apparently already using it)
 */

#include "ql/resource/todo.h"

namespace ql {
namespace resource {
namespace todo {

using namespace utils;
using namespace rmgr::resource_types;

// in configuration file, duration is in nanoseconds, while here we prefer it to have it in cycles
// it is needed to define the extend of the resource occupation in case of multi-cycle operations
static UInt ccl_get_operation_duration(const ir::GateRef &ins, const plat::PlatformRef &platform) {
    return ceil( static_cast<Real>(ins->duration) / platform->cycle_time);
}

// operation type is "mw" (for microwave), "flux", "readout", or "extern" (used for inter-core)
// it reflects the different resources used to implement the various gates and that resource management must distinguish
static Str ccl_get_operation_type(const ir::GateRef &ins, const plat::PlatformRef &platform) {
    Str operation_type("cc_light_type");
    QL_JSON_ASSERT(platform->instruction_settings, ins->name, ins->name);
    if (!platform->instruction_settings[ins->name]["type"].is_null()) {
        operation_type = platform->instruction_settings[ins->name]["type"].get<Str>();
    }
    return operation_type;
}

ccl_channel_resource_t::ccl_channel_resource_t(
    const plat::PlatformRef &platform,
    rmgr::Direction dir
) :
    OldResource("channels", dir)
{
    QL_DOUT("... creating " << name << " resource");

    // ncores = topology.number_of_cores: total number of cores
    if (platform->topology.count("number_of_cores") <= 0) {
        ncores = 1;
        QL_DOUT("Number of cores (topology[\"number_of_cores\"] not defined; assuming: " << ncores);
    } else {
        ncores = platform->topology["number_of_cores"];
        if (ncores <= 0) {
            QL_FATAL("Number of cores (topology[\"number_of_cores\"]) is not a positive value: " << ncores);
        }
    }
    QL_DOUT("Number of cores = " << ncores);

    // nchannels = resources.channels.count: number of channels in each core
    if (platform->resources[name].count("count") <= 0) {
        nchannels = platform->qubit_count/ncores;   // i.e. as many as there are qubits in a core
        QL_DOUT("Number of channels per core (resources[\"channels\"][\"count\"]) not defined; assuming: " << nchannels);
    } else {
        nchannels = platform->resources[name]["count"];
        if (nchannels <= 0) {
            QL_DOUT("Number of channels per core (resources[\"channels\"][\"count\"]) is not a positive value: " << nchannels);
            nchannels = platform->qubit_count/ncores;   // i.e. as many as there are qubits in a core
        }
        if (nchannels > platform->qubit_count/ncores) {
            QL_FATAL("Number of channels per core (resources[\"channels\"][\"count\"]) is larger than number of qubits per core: " << nchannels);
        }
    }
    QL_DOUT("Number of channels per core= " << nchannels);

    state.resize(ncores);
    for (UInt i=0; i<ncores; i++) state[i].resize(nchannels, (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE));
}

Bool ccl_channel_resource_t::available(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt operation_duration = ccl_get_operation_duration(ins, platform);

    Bool is_ic = (operation_type == "extern");
    if (is_ic) {
        QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle  << " for: " << ins->qasm());
        if (direction == rmgr::Direction::FORWARD) {
            for (auto q : ins->operands) {
                UInt core = q/(platform->qubit_count/ncores);
                Bool is_avail = false;
                // fwd: channel c is busy till cycle=state[core][c],
                // when reserving state[core][c] = start_cycle + duration
                // i.e. all cycles < state[core][c] it is busy, i.e. available when start_cycle >= state[core][c]
                QL_DOUT(" available " << name << "? ... q=" << q << " core=" << core);
                for (UInt c=0; c<nchannels; c++) {
                    QL_DOUT(" available " << name << "? ... c=" << c);
                    if (
                        op_start_cycle >= state[core][c]
                    ) {
                        QL_DOUT(" available " << name << "! for qubit: " << q << " in core: " << core << " channel: " << c << " available");
                        is_avail = true;
                        break;
                    }
                }
                if (!is_avail) {
                     QL_DOUT(" busy " << name << "! for qubit: " << q << " in core: " << core << " all channels busy");
                     return false;
                }
            }
        } else {
            for (auto q : ins->operands) {
                UInt core = q/(platform->qubit_count/ncores);
                Bool is_avail = false;
                // bwd: channel c is busy from cycle=state[core][c],
                // when reserving state[core][c] = start_cycle
                // i.e. all cycles >= state[core][c] it is busy, i.e. available when start_cycle + duration <= state[core][c]
                QL_DOUT(" available " << name << "? ... q=" << q << " core=" << core);
                for (UInt c=0; c<nchannels; c++) {
                    QL_DOUT(" available " << name << "? ... c=" << c);
                    if (
                        op_start_cycle + operation_duration <= state[core][c]
                    ) {
                        QL_DOUT(" available " << name << "! for qubit: " << q << " in core: " << core << " channel: " << c << " available");
                        is_avail = true;
                        break;
                    }
                }
                if (!is_avail) {
                    QL_DOUT(" busy " << name << "! for qubit: " << q << " in core: " << core << " all channels busy");
                    return false;
                }
            }
        }
        QL_DOUT(" available " << name << " resource available for: " << ins->qasm());
    }
    return true;
}

void ccl_channel_resource_t::reserve(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    // for each operand:
    //     find a free channel c and then do
    //     state[core][c] = (forward_scheduling == direction ?  op_start_cycle + operation_duration : op_start_cycle );
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt      operation_duration = ccl_get_operation_duration(ins, platform);

    Bool is_ic = (operation_type == "extern");
    if (is_ic) {
        QL_DOUT(" reserve " << name << "? op_start_cycle: " << op_start_cycle  << " for: " << ins->qasm());
        if (direction == rmgr::Direction::FORWARD) {
            for (auto q : ins->operands) {
                UInt core = q/(platform->qubit_count/ncores);
                Bool is_avail = false;
                // fwd: channel c is busy till cycle=state[core][c],
                // when reserving state[core][c] = start_cycle + duration
                // i.e. all cycles < state[core][c] it is busy, i.e. available when start_cycle >= state[core][c]
                for (UInt c=0; c<nchannels; c++) {
                    if (
                        op_start_cycle >= state[core][c]
                    ) {
                        state[core][c] = op_start_cycle + operation_duration;
                        QL_DOUT(" reserved " << name << "? for qubit: " << q << " in core: " << core << " channel: " << c << "till cycle: " << state[core][c]);
                        is_avail = true;
                        break;
                    }
                }
                QL_ASSERT(is_avail);
            }
        } else {
            // bwd: channel c is busy from cycle=state[core][c],
            // i.e. all cycles >= state[core][c] it is busy, i.e. available when start_cycle <= state[core][c]
            for (auto q : ins->operands) {
                UInt core = q/(platform->qubit_count/ncores);
                Bool is_avail = false;
                // bwd: channel c is busy from cycle=state[core][c],
                // when reserving state[core][c] = start_cycle
                // i.e. all cycles >= state[core][c] it is busy, i.e. available when start_cycle <= state[core][c]
                for (UInt c=0; c<nchannels; c++) {
                    if (
                        op_start_cycle + operation_duration <= state[core][c]
                    ) {
                        state[core][c] = op_start_cycle;
                        QL_DOUT(" reserved " << name << "? for qubit: " << q << " in core: " << core << " channel: " << c << " from cycle: " << state[core][c]);
                        is_avail = true;
                        break;
                    }
                }
                QL_ASSERT(is_avail);
            }
        }
    }
}

} // namespace todo
} // namespace resource
} // namespace ql
