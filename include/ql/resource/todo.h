/** \file
 * Resource management for CC-light platform.
 *
 * FIXME: needs cleanup, generalization, and conversion to new resource types,
 *  including support for the "undefined" direction (since the mapper was
 *  apparently already using it)
 */

#pragma once

#include <fstream>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/vec.h"
#include "ql/utils/json.h"
#include "ql/rmgr/resource_types/compat.h"

namespace ql {
namespace resource {
namespace todo {

// Inter-core communication gates use channels between cores.
// Each inter-core communication gate uses two channels, one at each side of the communication,
// to establish a connection between the cores, and especially between the qubits that are operands of the gate.
// Each of these two channels must remain allocated to the connection, while the communication is ongoing.
// When the communication gate is ready, the two channels can be released.
// Of those channels only a limited and configurable number may be available for each core.
// While during scheduling of a communication gate, no such channels are available, the gate is delayed.
class ccl_channel_resource_t : public rmgr::resource_types::OldResource {
public:
    utils::UInt ncores;                             // topology.number_of_cores: total number of cores
    utils::UInt nchannels;                          // resources.channels.count: number of channels in each core
    // fwd: channel c is busy till cycle=state[core][c],
    // i.e. all cycles < state[core][c] it is busy, i.e. start_cycle must be >= state[core][c]
    // bwd: channel c is busy from cycle=state[core][c],
    // i.e. all cycles >= state[core][c] it is busy, i.e. start_cycle+duration must be <= state[core][c]
    utils::Vec<utils::Vec<utils::UInt>> state;

    ccl_channel_resource_t(const plat::PlatformRef &platform, rmgr::Direction dir);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const override;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) override;
};

} // namespace todo

using Channels = rmgr::resource_types::Compat<todo::ccl_channel_resource_t>;

} // namespace resource
} // namespace ql
