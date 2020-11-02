/**
 * @file   qsoverlay.h
 * @date   11/2019
 * @author Diogo Valada
 * @brief  Prepares the quantumsim circuits using the qsoverlay format
 */

#include <string>
#include "program.h"
#include "platform.h"

namespace ql {

//Only support for DiCarlo setup atm
void write_qsoverlay_program(
    quantum_program *programp,
    size_t num_qubits,
    const ql::quantum_platform &platform,
    const std::string &suffix,
    size_t ns_per_cycle,
    bool compiled
);

} // namespace ql
