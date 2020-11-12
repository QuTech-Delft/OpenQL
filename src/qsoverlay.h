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
    size_t num_qubits,
    const quantum_platform &platform,
    const utils::Str &suffix,
    size_t ns_per_cycle,
    bool compiled
);

} // namespace ql
