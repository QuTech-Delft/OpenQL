/** \file
 * Platform header for target-specific compilation.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/json.h"
#include "hardware_configuration.h"

namespace ql {

class quantum_platform {
public:
    utils::Str              name;                     // platform name
    utils::Str              eqasm_compiler_name;      // eqasm backend
    utils::UInt             qubit_number;             // number of qubits
    utils::UInt             cycle_time;               // in [ns]
    utils::Str              configuration_file_name;  // configuration file name
    instruction_map_t       instruction_map;          // supported operations
    utils::Json             instruction_settings;     // instruction settings (to use by the eqasm backend)
    utils::Json             hardware_settings;        // additional hardware settings (to use by the eqasm backend)

    utils::Json             resources;
    utils::Json             topology;
    utils::Json             aliases;                  // workaround the generic instruction composition

    // FIXME: constructed object is not usable
    quantum_platform();
    quantum_platform(const utils::Str &name, const utils::Str &configuration_file_name);
    void print_info() const;
    utils::UInt get_qubit_number() const;  // FIXME: qubit_number is public anyway

    /**
     * @brief   Find architecture instruction name for a custom gate
     *
     * @param   iname   Name of instruction, e.g. "x q5" ('specialized custom gate') or "x" ('parameterized custom gate')
     * @return  value of 'arch_operation_name', e.g. "x"
     * @note    On CC-light, arch_operation_name is set from JSON field cc_light_instr
     * @note    Based on cc_light_scheduler.h::get_cc_light_instruction_name()
     * @note    Only works for custom instructions defined in JSON
       FIXME:   it may be more useful to get the information directly from JSON, because arch_operation_name is not really generic
     */

    // find settings for custom gate, preventing JSON exceptions
    const utils::Json &find_instruction(const utils::Str &iname) const;

    // find instruction type for custom gate
    utils::Str find_instruction_type(const utils::Str &iname) const;

    utils::UInt time_to_cycles(utils::Real time_ns) const;
};

} // namespace ql
