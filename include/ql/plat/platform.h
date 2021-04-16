/** \file
 * Platform header for target-specific compilation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/opt.h"
#include "ql/utils/json.h"
#include "ql/utils/tree.h"
#include "ql/plat/hardware_configuration.h"
#include "ql/plat/topology.h"

namespace ql {
namespace plat {

class Platform : public utils::Node {
public:
    /**
     * User-specified name for the platform.
     */
    utils::Str name;

    /**
     * Name of the architecture being compiled for.
     *
     * TODO: this should be removed, and abstracted entirely to the much more
     *  generic pass management logic.
     */
    utils::Str eqasm_compiler_name;

    /**
     * The total number of physical qubits supported by the platform.
     */
    utils::UInt qubit_count;

    /**
     * The total number of 32-bit general-purpose classical registers supported
     * by the platform.
     */
    utils::UInt creg_count;

    /**
     * Historically, creg count was not specified in the platform description
     * file, and was instead implicitly taken from the amount allocated for the
     * program constructed from it. Setting this models this old behavior to
     * some extent: creg_count will then be increased whenever a program is
     * created with more than creg_count creg declarations.
     */
    utils::Bool compat_implicit_creg_count;

    /**
     * The total number of single-bit condition/measurement result registers
     * supported by the platform.
     */
    utils::UInt breg_count;

    /**
     * Same as compat_implicit_creg_count, but for bregs.
     */
    utils::Bool compat_implicit_breg_count;

    /**
     * Cycle time in nanoseconds.
     *
     * FIXME: why is this a UInt? Non-integer-nanosecond cycle times are not
     *  supported...? At least use picoseconds or femtoseconds as a unit if it
     *  needs to be fixed-point, 64-bit is plenty for that.
     */
    utils::UInt cycle_time;

    /**
     * Path to the JSON file that was used to configure this platform.
     *
     * FIXME: it's wrong that things are using this. Once constructed, the
     *  filename should be don't care (in theory, there doesn't even need to be
     *  a file).
     */
    utils::Str configuration_file_name;

    /**
     * The gate/instruction set supported by this platform.
     */
    InstructionMap instruction_map;

    /**
     * Raw instruction setting data for use by the eqasm backend, corresponding
     * to the `"instructions"` key in the root JSON object.
     *
     * FIXME: this shouldn't be here. Extra data should be part of the gate
     *  types (but there are no gate types yet, of course).
     */
    utils::Json instruction_settings;

    /**
     * Additional hardware settings (to use by the eqasm backend), corresponding
     * to the `"hardware_settings"` key in the root JSON object.
     */
    utils::Json hardware_settings;

    /**
     * Scheduling resource description (representing e.g. instrument/control
     * constraints), corresponding to the `"resources"` key in the root JSON
     * object.
     *
     * FIXME: this shouldn't be here as a raw JSON object.
     */
    utils::Json resources;

    /**
     * Topology/qubit grid description, corresponding to the `"topology"` key
     * in the root of the JSON object.
     *
     * FIXME: this shouldn't be here as a raw JSON object.
     */
    utils::Json topology;

    /**
     * Parsed topology/qubit grid information.
     */
    utils::Opt<Grid> grid;

    /**
     * Constructs a platform from the given configuration filename.
     */
    Platform(const utils::Str &name, const utils::Str &configuration_file_name);

    /**
     * Prints some basic info about the platform to stdout.
     */
    void print_info() const;

    // find settings for custom gate, preventing JSON exceptions
    const utils::Json &find_instruction(const utils::Str &iname) const;

    // find instruction type for custom gate
    utils::Str find_instruction_type(const utils::Str &iname) const;

    utils::UInt time_to_cycles(utils::Real time_ns) const;
};

using PlatformRef = utils::One<Platform>;

} // namespace plat
} // namespace ql
