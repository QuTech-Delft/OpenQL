/** \file
 * Implementation for pass that writes circuits using the qsoverlay format.
 */

#include "utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

//Only support for DiCarlo setup atm
void write_qsoverlay_program(
    quantum_program *programp,
    utils::UInt num_qubits,
    const quantum_platform &platform,
    const utils::Str &suffix,
    utils::UInt ns_per_cycle,
    utils::Bool compiled
);

} // namespace ql
