/** \file
 * Defines information about the no-op architecture.
 */

#include "ql/arch/none/info.h"

namespace ql {
namespace arch {
namespace none {

/**
 * Writes the documentation for this architecture to the given output
 * stream.
 */
void Info::dump_docs(std::ostream &os, const utils::Str &line_prefix) const {
    utils::dump_str(os, line_prefix, R"(
    This is just a dummy architecture that does not include any backend passes
    by default, does not provide shortcuts for any architecture-specific passes
    and resources, and does not do any platform-specific preprocessing on the
    platform configuration file. You can use it when you just want to try OpenQL
    out, or when your target is an architecture-agnostic simulator.

    The default configuration file consists of relatively sane defaults for
    simulating the resulting cQASM output with the QX simulator.
    )");
}

/**
 * Returns a user-friendly type name for this architecture. Used for
 * documentation generation.
 */
utils::Str Info::get_friendly_name() const {
    return "None";
}

/**
 * Returns the name of the namespace for this architecture.
 */
utils::Str Info::get_namespace_name() const {
    return "none";
}

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> Info::get_eqasm_compiler_names() const {
    return {"none", "qx", ""};
}

/**
 * Should generate a sane default platform JSON file, for when the user
 * constructs a Platform without JSON data. This is done by specifying an
 * architecture namespace identifier instead of a JSON filename. Optionally,
 * the user may specify a variant suffix, separated using a dot, to select
 * a variation of the architecture; for instance, for CC-light, there might
 * be variations for surface-5, surface-7, and surface-17. This JSON data
 * will still be preprocessed by preprocess_platform().
 */
utils::Str Info::get_default_platform(const utils::Str &variant) const {

    // NOTE: based on tests/hardware_config_qx.json at the time of writing.
    return R"({
    "eqasm_compiler" : "none",

    "hardware_settings": {
        "qubit_number": 10,
        "cycle_time" : 20
    },

    "resources": {
    },
    "topology" : {
    },

    "instructions": {

        "prep_x" : {
            "duration": 40,
            "qubits": ["q0","q1"]
        },

        "prep_y" : {
            "duration": 40,
            "qubits": ["q0","q1"]
        },

        "prep_z" : {
            "duration": 40,
            "qubits": ["q0","q1"]
        },

        "i" : {
            "duration": 40
        },

        "h" : {
            "duration": 40
        },

        "x" : {
            "duration": 40
        },

        "y" : {
            "duration": 40
        },

        "z" : {
            "duration": 40
        },

        "x90" : {
            "duration": 40
        },

        "y90" : {
            "duration": 20
        },

        "x180" : {
            "duration": 40
        },

        "y180" : {
            "duration": 40
        },
)" R"(
        "mx90" : {
            "duration": 40
        },

        "my90" : {
            "duration": 20
        },

        "rx" : {
            "duration": 40
        },

        "ry" : {
            "duration": 40
        },

        "rz" : {
            "duration": 40
        },

        "s" : {
            "duration": 40
        },

        "sdag" : {
            "duration": 40
        },

        "t" : {
            "duration": 40
        },

        "tdag" : {
            "duration": 40
        },

        "cnot" : {
            "duration": 80
        },

        "toffoli" : {
            "duration": 80
        },

        "cz" : {
            "duration": 80
        },

        "swap" : {
            "duration": 80
        },

        "measure" : {
            "duration": 300
        },

        "measure_x" : {
            "duration": 300
        },

        "measure_y" : {
            "duration": 300
        },

        "measure_z" : {
            "duration": 300
        },

        "display": {
            "duration": 20,
            "qubits": []
        },

        "display_binary": {
            "duration": 20,
            "qubits": []
        }
    },

    "gate_decomposition": {
    }
})";

}

} // namespace none
} // namespace arch
} // namespace ql
