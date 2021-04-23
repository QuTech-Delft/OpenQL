/** \file
 * Defines information about the CC-light architecture.
 */

#include "ql/arch/cc_light/info.h"

#include "ql/com/options.h"

namespace ql {
namespace arch {
namespace cc_light {

/**
 * Writes the documentation for this architecture to the given output
 * stream.
 */
void Info::dump_docs(std::ostream &os, const utils::Str &line_prefix) const {
    utils::dump_str(os, line_prefix, R"(
    TODO: port from docs/platform_ccl.rst
    )");
}

/**
 * Returns a user-friendly type name for this pass. Used for documentation
 * generation.
 */
utils::Str Info::get_friendly_name() const {
    return "CC-light";
}

/**
 * Returns the name of the namespace for this architecture.
 */
utils::Str Info::get_namespace_name() const {
    return "cc_light";
}

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> Info::get_eqasm_compiler_names() const {
    return {"cc_light", "cc_light_compiler"};
}

/**
 * Should generate a sane default platform JSON file. This JSON data will
 * still be preprocessed by preprocess_platform().
 */
utils::Str Info::get_default_platform() const {

    // NOTE: based on tests/hardware_config_cc_light.json at the time of
    // writing.
    return R"({
    "eqasm_compiler" : "cc_light",

    "hardware_settings": {
        "qubit_number": 7,
        "cycle_time" : 20,
        "mw_mw_buffer": 0,
        "mw_flux_buffer": 0,
        "mw_readout_buffer": 0,
        "flux_mw_buffer": 0,
        "flux_flux_buffer": 0,
        "flux_readout_buffer": 0,
        "readout_mw_buffer": 0,
        "readout_flux_buffer": 0,
        "readout_readout_buffer": 0
    },

    "resources": {
        "qubits": {
            "count": 7
        },
        "qwgs": {
            "count": 3,
            "connection_map": {
                "0" : [0, 1],
                "1" : [2, 3, 4],
                "2" : [5, 6]
            }
        },
        "meas_units": {
            "count": 2,
            "connection_map": {
                "0" : [0, 2, 3, 5, 6],
                "1" : [1, 4]
            }
        },
        "edges": {
            "count": 16,
            "connection_map": {
                "0": [2, 10],
                "1": [3, 11],
                "2": [0, 8],
                "3": [1, 9],
                "4": [6, 14],
                "5": [7, 15],
                "6": [4, 12],
                "7": [5, 13],
                "8": [2, 10],
                "9": [3, 11],
                "10": [0, 8],
                "11": [1, 9],
                "12": [6, 14],
                "13": [7, 15],
                "14": [4, 12],
                "15": [5, 13]
            }
        },
        "detuned_qubits": {
            "description": "A two-qubit flux gate lowers the frequency of its source qubit to get near the frequency of its target qubit.  Any two qubits which have near frequencies execute a two-qubit flux gate.  To prevent any neighbor qubit of the source qubit that has the same frequency as the target qubit to interact as well, those neighbors must have their frequency detuned (lowered out of the way).  A detuned qubit cannot execute a single-qubit rotation (an instruction of 'mw' type).  An edge is a pair of qubits which can execute a two-qubit flux gate.  There are 'count' qubits. For each edge it is described, when executing a two-qubit gate for it, which set of qubits it detunes.",
            "count": 7,
            "connection_map": {
                "0": [3],
                "1": [2],
                "2": [4],
                "3": [3],
                "4": [],
                "5": [6],
                "6": [5],
                "7": [],
                "8": [3],
                "9": [2],
                "10": [4],
                "11": [3],
                "12": [],
                "13": [6],
                "14": [5],
                "15": []
            }
        }
    },

    "topology": {
        "x_size": 5,
        "y_size": 3,
        "qubits": [
            { "id": 0,  "x": 1, "y": 2 },
            { "id": 1,  "x": 3, "y": 2 },
            { "id": 2,  "x": 0, "y": 1 },
            { "id": 3,  "x": 2, "y": 1 },
            { "id": 4,  "x": 4, "y": 1 },
            { "id": 5,  "x": 1, "y": 0 },
            { "id": 6,  "x": 3, "y": 0 }
        ],
        "edges": [
            { "id": 0,  "src": 2, "dst": 0 },
            { "id": 1,  "src": 0, "dst": 3 },
            { "id": 2,  "src": 3, "dst": 1 },
            { "id": 3,  "src": 1, "dst": 4 },
            { "id": 4,  "src": 2, "dst": 5 },
            { "id": 5,  "src": 5, "dst": 3 },
            { "id": 6,  "src": 3, "dst": 6 },
            { "id": 7,  "src": 6, "dst": 4 },
            { "id": 8,  "src": 0, "dst": 2 },
            { "id": 9,  "src": 3, "dst": 0 },
            { "id": 10, "src": 1, "dst": 3 },
            { "id": 11, "src": 4, "dst": 1 },
            { "id": 12, "src": 5, "dst": 2 },
            { "id": 13, "src": 3, "dst": 5 },
            { "id": 14, "src": 6, "dst": 3 },
            { "id": 15, "src": 4, "dst": 6 }
        ]
    },
)" R"(
    "instructions": {
        "prepx q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "prepx"
        },
        "prepx q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "prepx"
        },
        "prepx q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "prepx"
        },
        "prepx q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "prepx"
        },
        "prepx q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "prepx"
        },
        "prepx q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "prepx"
        },
        "prepx q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "prepx"
        },
        "prepz q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "none",
            "cc_light_instr": "prepz"
        },
        "prepz q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "none",
            "cc_light_instr": "prepz"
        },
        "prepz q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "none",
            "cc_light_instr": "prepz"
        },
        "prepz q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "none",
            "cc_light_instr": "prepz"
        },
        "prepz q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "none",
            "cc_light_instr": "prepz"
        },
        "prepz q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "none",
            "cc_light_instr": "prepz"
        },
        "prepz q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "none",
            "cc_light_instr": "prepz"
        },
        "cprepz q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "cprepz"
        },
        "measx q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "readout",
            "cc_light_instr": "measx"
        },
        "measx q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "readout",
            "cc_light_instr": "measx"
        },
        "measx q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "readout",
            "cc_light_instr": "measx"
        },
        "measx q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "readout",
            "cc_light_instr": "measx"
        },
        "measx q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "readout",
            "cc_light_instr": "measx"
        },
        "measx q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "readout",
            "cc_light_instr": "measx"
        },
        "measx q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "readout",
            "cc_light_instr": "measx"
        },
        "measz q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measz q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measz q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measz q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measz q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measz q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measz q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "readout",
            "cc_light_instr": "measz"
        },)" R"(
        "measure q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measure q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measure q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measure q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measure q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measure q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "measure q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "readout",
            "cc_light_instr": "measz"
        },
        "i q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "i"
        },
        "i q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "i"
        },
        "i q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "i"
        },
        "i q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "i"
        },
        "i q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "i"
        },
        "i q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "i"
        },
        "i q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "i"
        },
        "x q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "x"
        },
        "x q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "x"
        },
        "x q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "x"
        },
        "x q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "x"
        },
        "x q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "x"
        },
        "x q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "x"
        },
        "x q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "x"
        },
        "y q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "y"
        },
        "y q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "y"
        },
        "y q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "y"
        },
        "y q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "y"
        },
        "y q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "y"
        },
        "y q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "y"
        },
        "y q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "y"
        },
        "z q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "z"
        },
        "z q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "z"
        },
        "z q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "z"
        },
        "z q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "z"
        },
        "z q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "z"
        },
        "z q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "z"
        },
        "z q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "z"
        },
        "h q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "h"
        },
        "h q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "h"
        },
        "h q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "h"
        },
        "h q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "h"
        },
        "h q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "h"
        },
        "h q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "h"
        },
        "h q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "h"
        },
        "s q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "s"
        },
        "s q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "s"
        },
        "s q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "s"
        },
        "s q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "s"
        },
        "s q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "s"
        },
        "s q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "s"
        },
        "s q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "s"
        },
        "sdag q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "sdag"
        },
        "sdag q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "sdag"
        },
        "sdag q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "sdag"
        },
        "sdag q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "sdag"
        },
        "sdag q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "sdag"
        },
        "sdag q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "sdag"
        },
        "sdag q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "sdag"
        },
        "rx90 q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "x90"
        },
        "rx90 q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "x90"
        },
        "rx90 q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "x90"
        },
        "rx90 q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "x90"
        },
        "rx90 q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "x90"
        },)" R"(
        "rx90 q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "x90"
        },
        "rx90 q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "x90"
        },
        "xm90 q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "xm90"
        },
        "xm90 q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "xm90"
        },
        "xm90 q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "xm90"
        },
        "xm90 q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "xm90"
        },
        "xm90 q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "xm90"
        },
        "xm90 q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "xm90"
        },
        "xm90 q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "xm90"
        },
        "ry90 q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "y90"
        },
        "ry90 q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "y90"
        },
        "ry90 q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "y90"
        },
        "ry90 q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "y90"
        },
        "ry90 q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "y90"
        },
        "ry90 q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "y90"
        },
        "ry90 q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "y90"
        },
        "ym90 q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "ym90"
        },
        "ym90 q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "ym90"
        },
        "ym90 q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "ym90"
        },
        "ym90 q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "ym90"
        },
        "ym90 q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "ym90"
        },
        "ym90 q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "ym90"
        },
        "ym90 q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "ym90"
        },
        "t q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "t"
        },
        "t q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "t"
        },
        "t q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "t"
        },
        "t q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "t"
        },
        "t q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "t"
        },
        "t q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "t"
        },
        "t q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "t"
        },
        "tdag q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "tdag"
        },
        "tdag q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "tdag"
        },
        "tdag q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "tdag"
        },
        "tdag q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "tdag"
        },
        "tdag q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "tdag"
        },
        "tdag q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "tdag"
        },
        "tdag q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "tdag"
        },
        "x45 q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "x45"
        },
        "x45 q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "x45"
        },
        "x45 q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "x45"
        },
        "x45 q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "x45"
        },
        "x45 q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "x45"
        },
        "x45 q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "x45"
        },
        "x45 q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "x45"
        },
        "xm45 q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "xm45"
        },
        "xm45 q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "type": "mw",
            "cc_light_instr": "xm45"
        },
        "xm45 q2": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q2"],
            "type": "mw",
            "cc_light_instr": "xm45"
        },
        "xm45 q3": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q3"],
            "type": "mw",
            "cc_light_instr": "xm45"
        },
        "xm45 q4": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q4"],
            "type": "mw",
            "cc_light_instr": "xm45"
        },
        "xm45 q5": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q5"],
            "type": "mw",
            "cc_light_instr": "xm45"
        },
        "xm45 q6": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q6"],
            "type": "mw",
            "cc_light_instr": "xm45"
        },
        "ry180 q0": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q0"],
            "type": "mw",
            "cc_light_instr": "ry180"
        },)" R"(
        "cnot q2,q0": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q2","q0"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q0,q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q0","q3"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q3,q1": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3","q1"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q1,q4": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q1","q4"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q2,q5": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q2","q5"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q5,q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q5","q3"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q3,q6": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3","q6"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q6,q4": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q6","q4"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q0,q2": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q0","q2"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q3,q0": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3","q0"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q1,q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q1","q3"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q4,q1": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q4","q1"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q5,q2": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q5","q2"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q3,q5": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3","q5"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q6,q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q6","q3"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },
        "cnot q4,q6": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q4","q6"],
            "type": "flux",
            "cc_light_instr": "cnot"
        },

        "sqf q0": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q0"],
            "type": "flux",
            "cc_light_instr": "sqf"
        },

        "sqf q1": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q1"],
            "type": "flux",
            "cc_light_instr": "sqf"
        },

        "sqf q2": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q2"],
            "type": "flux",
            "cc_light_instr": "sqf"
        },

        "sqf q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3"],
            "type": "flux",
            "cc_light_instr": "sqf"
        },

        "sqf q4": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q4"],
            "type": "flux",
            "cc_light_instr": "sqf"
        },

        "sqf q5": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q5"],
            "type": "flux",
            "cc_light_instr": "sqf"
        },

        "sqf q6": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q6"],
            "type": "flux",
            "cc_light_instr": "sqf"
        },

        "cz q2,q0": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q2","q0"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q0,q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q0","q3"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q3,q1": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3","q1"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q1,q4": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q1","q4"],
            "type": "flux",
            "cc_light_instr": "cz"
        },)" R"(
        "cz q2,q5": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q2","q5"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q5,q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q5","q3"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q3,q6": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3","q6"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q6,q4": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q6","q4"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q0,q2": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q0","q2"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q3,q0": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3","q0"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q1,q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q1","q3"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q4,q1": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q4","q1"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q5,q2": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q5","q2"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q3,q5": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q3","q5"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q6,q3": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q6","q3"],
            "type": "flux",
            "cc_light_instr": "cz"
        },
        "cz q4,q6": {
            "duration": 80,
            "latency": 0,
            "qubits": ["q4","q6"],
            "type": "flux",
            "cc_light_instr": "cz"
        }
    },

    "gate_decomposition": {
        "rx180 %0" : ["x %0"],
        "cnot %0,%1" : ["ry90 %0","cz %0,%1","ry90 %1"]
    }
})";

}

/**
 * Adds the default "backend passes" for this platform. Called by
 * pmgr::Manager::from_defaults() when no compiler configuration file is
 * specified. This typically includes at least the architecture-specific
 * code generation pass, but anything after prescheduling and optimization
 * is considered a backend pass.
 */
void Info::populate_backend_passes(pmgr::Manager &manager) const {

    // Mapping.
    if (com::options::global["clifford_premapper"].as_bool()) {
        manager.append_pass(
            "opt.clifford.Optimize",
            "clifford_premapper"
        );
    }
    if (com::options::global["mapper"].as_str() != "no") {
        manager.append_pass(
            "map.qubits.Map",
            "mapper"
        );
    }
    if (com::options::global["clifford_postmapper"].as_bool()) {
        manager.append_pass(
            "opt.clifford.Optimize",
            "clifford_postmapper"
        );
    }

    // Scheduling.
    manager.append_pass(
        "sch.Schedule",
        "rcscheduler",
        {
            {"resource_constraints", "yes"}
        }
    );
    manager.append_pass(
        "io.cqasm.Report",
        "lastqasmwriter",
        {
            {"output_prefix", com::options::global["output_dir"].as_str() + "/%N"},
            {"output_suffix", "_last.qasm"}
        }
    );

    // R.I.P. CC-light code generation.

}

} // namespace cc_light
} // namespace arch
} // namespace ql
