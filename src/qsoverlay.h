/** \file
 * Implementation for pass that writes circuits using the qsoverlay format.
 */

#include "ql/utils/str.h"
#include "ql/ir/ir.h"
#include "ql/plat/platform.h"

namespace ql {

//Only support for DiCarlo setup atm
void write_qsoverlay_program(
    const ir::ProgramRef &program,
    utils::UInt num_qubits,
    const plat::PlatformRef &platform,
    const utils::Str &suffix,
    utils::UInt ns_per_cycle,
    utils::Bool compiled
);

} // namespace ql
