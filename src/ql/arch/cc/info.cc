/** \file
 * Defines information about the CC architecture.
 */

#include "ql/arch/cc/info.h"

#include "ql/com/options.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/options.h"

namespace ql {
namespace arch {
namespace cc {

/**
 * Writes the documentation for this architecture to the given output
 * stream.
 */
void Info::dump_docs(std::ostream &os, const utils::Str &line_prefix) const {
    utils::dump_str(os, line_prefix, R"(
    TODO: port from docs/platform_cc.rst
    )");
}

/**
 * Returns a user-friendly type name for this pass. Used for documentation
 * generation.
 */
utils::Str Info::get_friendly_name() const {
    return "QuTech Central Controller";
}

/**
 * Returns the name of the namespace for this architecture.
 */
utils::Str Info::get_namespace_name() const {
    return "cc";
}

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> Info::get_eqasm_compiler_names() const {
    return {"cc", "eqasm_backend_cc"};
}

/**
 * Should generate a sane default platform JSON file. This JSON data will
 * still be preprocessed by preprocess_platform().
 */
utils::Str Info::get_default_platform() const {

    // NOTE: based on tests/cc/cc_s5_direct_iq.json at the time of writing.
    return R"({
    "eqasm_compiler" : "cc",
    "hardware_settings": {
        "qubit_number": 5,
        "cycle_time" : 20,
        "eqasm_backend_cc": {
            "instrument_definitions": {
                "qutech-qwg": {
                    "channels": 4,
                    "control_group_sizes": [1, 4],
                    "latency": 50
                },
                "zi-hdawg": {
                    "channels": 8,
                    "control_group_sizes": [1, 2, 4, 8],
                    "latency": 300
                },
                "qutech-vsm": {
                    "channels": 32,
                    "control_group_sizes": [1],
                    "latency": 10
                },
                "zi-uhfqa": {
                    "channels": 9,
                    "control_group_sizes": [1],
                    "latency": 150
                }
            },
            "control_modes": {
                "awg8-mw-vsm-hack": {
                    "control_bits": [
                        [7,6,5,4,3,2,1,0],
                        [16,15,14,13,12,11,10,9]
                    ],
                    "trigger_bits": [31]
                },
                "awg8-mw-vsm": {
                    "control_bits": [
                        [7,6,5,4,3,2,1,0],
                        [23,22,21,20,19,18,17,16]
                    ],
                    "trigger_bits": [31]
                },
                "awg8-mw-direct-iq": {
                    "control_bits": [
                        [6,5,4,3,2,1,0],
                        [13,12,11,10,9,8,7],
                        [22,21,20,19,18,17,16],
                        [29,28,27,26,25,24,23]
                    ],
                    "trigger_bits": [15,31]
                },
                "awg8-flux": {
                    "control_bits": [
                        [2,1,0],
                        [5,4,3],
                        [8,7,6],
                        [11,10,9],
                        [18,17,16],
                        [21,20,19],
                        [24,23,22],
                        [27,26,25]
                    ],
                    "trigger_bits": [31]
                },
                "awg8-flux-vector-8": {
                    "control_bits": [
                        [7,6,5,4,3,2,1,0]
                    ],
                    "trigger_bits": [31]
                },
                "dualqwg-mw-direct-iq": {
                    "control_bits": [
                        [6,5,4,3,2,1,0],
                        [13,12,11,10,9,8,7],
                        [22,21,20,19,18,17,16],
                        [29,28,27,26,25,24,23]
                    ],
                    "trigger_bits": [15,31]
                },
                "uhfqa-9ch": {
                    "control_bits": [[17],[18],[19],[20],[21],[22],[23],[24],[25]],
                    "trigger_bits": [16],
                    "result_bits": [[1],[2],[3],[4],[5],[6],[7],[8],[9]],
                    "data_valid_bits": [0]
                },
                "vsm-32ch":{
                    "control_bits": [
                        [0],[1],[2],[3],[4],[5],[6],[7],
                        [8],[9],[10],[11],[12],[13],[14],[15],
                        [16],[17],[18],[19],[20],[21],[22],[23],
                        [24],[25],[26],[27],[28],[28],[30],[31]
                    ],
                    "trigger_bits": []
                }
            },
            "signals": {
                "single-qubit-mw": [
                    {
                        "type": "mw",
                        "operand_idx": 0,
                        "value": [
                            "{gateName}-{instrumentName}:{instrumentGroup}-i",
                            "{gateName}-{instrumentName}:{instrumentGroup}-q"
                        ]
                    }
                ],
                "two-qubit-flux": [
                    {
                        "type": "flux",
                        "operand_idx": 0,
                        "value": ["flux-0-{qubit}"]
                    },
                    {
                        "type": "flux",
                        "operand_idx": 1,
                        "value": ["flux-1-{qubit}"]
                    }
                ],
                "single-qubit-flux": [
                    {
                        "type": "flux",
                        "operand_idx": 0,
                        "value": ["flux-0-{qubit}"]
                    }
                ]
            },
            "instruments": [
                {
                    "name": "ro_1",
                    "qubits": [[0], [2], [3], [4], [], [], [], [], []],
                    "signal_type": "measure",
                    "ref_instrument_definition": "zi-uhfqa",
                    "ref_control_mode": "uhfqa-9ch",
                    "controller": {
                        "name": "cc",
                        "slot": 0,
                        "io_module": "CC-CONN-DIO"
                    }
                },
                {
                    "name": "ro_2",
                    "qubits": [[1], [], [], [], [], [], [], [], []],
                    "signal_type": "measure",
                    "ref_instrument_definition": "zi-uhfqa",
                    "ref_control_mode": "uhfqa-9ch",
                    "controller": {
                        "name": "cc",
                        "slot": 1,
                        "io_module": "CC-CONN-DIO"
                    }
                },
                {
                    "name": "mw_0",
                    "qubits": [
                        [0],
                        [1],
                        [2],
                        [3]
                    ],
                    "signal_type": "mw",
                    "ref_instrument_definition": "zi-hdawg",
                    "ref_control_mode": "awg8-mw-direct-iq",
                    "controller": {
                        "name": "cc",
                        "slot": 2,
                        "io_module": "CC-CONN-DIO-DIFF"
                    }
                },
                {
                    "name": "mw_1",
                    "qubits": [
                        [4],
                        [],
                        [],
                        []
                    ],
                    "signal_type": "mw",
                    "ref_instrument_definition": "zi-hdawg",
                    "ref_control_mode": "awg8-mw-direct-iq",
                    "controller": {
                        "name": "cc",
                        "slot": 3,
                        "io_module": "CC-CONN-DIO-DIFF"
                    }
                },
                {
                    "name": "flux_0",
                    "qubits": [[0], [1], [2], [3], [4], [], [], []],
                    "signal_type": "flux",
                    "ref_instrument_definition": "zi-hdawg",
                    "ref_control_mode": "awg8-flux",
                    "controller": {
                        "name": "cc",
                        "slot": 4,
                        "io_module": "CC-CONN-DIO-DIFF"
                    }
                }
            ]
        }
    },
    "gate_decomposition": {
        "measz %0": ["measure %0"],
        "x %0": ["rx180 %0"],
        "y %0": ["ry180 %0"],
        "h %0": ["ry90 %0", "ry180 %0"],
        "z %0": ["rx180 %0","ry180 %0"],
        "t %0": ["ry90 %0","rx45 %0","rym90 %0"],
        "tdag %0": ["ry90 %0","rxm45 %0","rym90 %0"],
        "s %0": ["ry90 %0","rx90 %0","rym90 %0"],
        "sdag %0": ["ry90 %0","rxm90 %0","rym90 %0"],
        "cnot %0 %1": ["rym90 %1", "cz %0 %1", "ry90 %1"],
        "cz q0 q2": ["barrier q0,q2", "sf_cz_se q0", "sf_cz_nw q2", "barrier q0,q2"],
        "cz q2 q0": ["barrier q0,q2", "sf_cz_se q0", "sf_cz_nw q2", "barrier q0,q2"],
        "cz q1 q2": ["barrier q1,q2", "sf_cz_sw q1", "sf_cz_ne q2", "barrier q1,q2"],
        "cz q2 q1": ["barrier q1,q2", "sf_cz_sw q1", "sf_cz_ne q2", "barrier q1,q2"],
        "cz q3 q2": ["barrier q2,q3,q4", "sf_cz_sw q2", "sf_cz_ne q3", "sf_park q4", "barrier q2,q3,q4"],
        "cz q2 q3": ["barrier q2,q3,q4", "sf_cz_sw q2", "sf_cz_ne q3", "sf_park q4", "barrier q2,q3,q4"],
        "cz q4 q2": ["barrier q2,q3,q4", "sf_cz_se q2", "sf_cz_nw q4", "sf_park q3", "barrier q2,q3,q4"],
        "cz q2 q4": ["barrier q2,q3,q4", "sf_cz_se q2", "sf_cz_nw q4", "sf_park q3", "barrier q2,q3,q4"],
        "x180 %0": ["rx180 %0"],
        "y180 %0": ["ry180 %0"],
        "y90 %0": ["ry90 %0"],
        "x90 %0": ["rx90 %0"],
        "ym90 %0": ["rym90 %0"],
        "xm90 %0": ["rxm90 %0"],
        "cl_0 %0": ["i %0"],
        "cl_1 %0": ["ry90 %0", "rx90 %0"],
        "cl_2 %0": ["rxm90 %0", "rym90 %0"],
        "cl_3 %0": ["rx180 %0"],
        "cl_4 %0": ["rym90 %0", "rxm90 %0"],
        "cl_5 %0": ["rx90 %0", "rym90 %0"],
        "cl_6 %0": ["ry180 %0"],
        "cl_7 %0": ["rym90 %0", "rx90 %0"],
        "cl_8 %0": ["rx90 %0", "ry90 %0"],
        "cl_9 %0": ["rx180 %0", "ry180 %0"],
        "cl_10 %0": ["ry90 %0", "rxm90 %0"],
        "cl_11 %0": ["rxm90 %0", "ry90 %0"],
        "cl_12 %0": ["ry90 %0", "rx180 %0"],
        "cl_13 %0": ["rxm90 %0"],
        "cl_14 %0": ["rx90 %0", "rym90 %0", "rxm90 %0"],
        "cl_15 %0": ["rym90 %0"],
        "cl_16 %0": ["rx90 %0"],
        "cl_17 %0": ["rx90 %0", "ry90 %0", "rx90 %0"],
        "cl_18 %0": ["rym90 %0", "rx180 %0"],
        "cl_19 %0": ["rx90 %0", "ry180 %0"],
        "cl_20 %0": ["rx90 %0", "rym90 %0", "rx90 %0"],
        "cl_21 %0": ["ry90 %0"],
        "cl_22 %0": ["rxm90 %0", "ry180 %0"],
        "cl_23 %0": ["rx90 %0", "ry90 %0", "rxm90 %0"],
        "measure_fb %0": ["measure %0", "_wait_uhfqa %0", "_dist_dsm %0", "_wait_dsm %0"]
    },
    "instructions": {
        "i": {
            "duration": 20,
            "cc": {
                "signal": [],
                "static_codeword_override": [0]
            }
        },
        "rx180": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [1]
            }
        },
        "ry180": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [2]
            }
        },
        "rx90": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [3]
            }
        },
        "ry90": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [4]
            }
        },
        "rxm90": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [5]
            }
        },
        "rym90": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [6]
            }
        },
        "ry45": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [1]
            }
        },
        "rym45": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [2]
            }
        },
        "rx45": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [3]
            }
        },
        "rxm45": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [4]
            }
        },
        "cz": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [1,1]
            }
        },
        "cz_park": {
            "duration": 80,
            "cc": {
                "signal": [
                    {   "type": "flux",
                        "operand_idx": 0,
                        "value": ["flux-0-{qubit}"]
                    },
                    {   "type": "flux",
                        "operand_idx": 1,
                        "value": ["flux-1-{qubit}"]
                    },
                    {   "type": "flux",
                        "operand_idx": 2,
                        "value": ["park_cz-{qubit}"]
                    }
                ],
                "static_codeword_override": [0,0,0]
            }
        },
        "park_cz" : {
            "duration" : 80,
            "cc": {
                "signal": [
                    {    "type": "flux",
                        "operand_idx": 0,
                        "value": ["park_cz-{qubit}"]
                    }
                ],
                "static_codeword_override": [0]
            }
        },
        "park_measure" : {
            "duration" : 2000,
            "cc": {
                "signal": [
                    {    "type": "flux",
                        "operand_idx": 0,
                        "value": ["park_measure-{qubit}"]
                    }
                ],
                "static_codeword_override": [0]
            }
        },
        "prepz": {
            "duration": 200000,
            "cc": {
                "signal": [],
                "static_codeword_override": [0]
            }
        },
        "measure": {
            "duration": 2000,
            "cc": {
                "readout_mode": "",
                "signal": [
                    {    "type": "measure",
                        "operand_idx": 0,
                        "value": ["dummy"],
                        "weight": ["dummy"]
                    }
                ],
                "static_codeword_override": [0]
            }
        },
        "_wait_uhfqa": {
            "duration": 220,
            "cc": {
                "signal": []
            }
        },
        "_dist_dsm": {
            "duration": 20,
            "cc": {
                "readout_mode": "feedback",
                "signal": [
                    {    "type": "measure",
                        "operand_idx": 0,
                        "value": []
                    }
                ]
            }
        },
        "_wait_dsm": {
            "duration": 80,
            "cc": {
                "signal": []
            }
        },
        "if_0_break": {
            "duration": 60,
            "cc": {
                "signal": [],
                "pragma": {
                    "break": 0
                }
            }
        },
        "if_1_break": {
            "duration": 60,
            "cc": {
                "signal": [],
                "pragma": {
                    "break": 1
                }
            }
        },
        "square": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [0]
            }
        },
        "spec": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [0]
            }
        },
        "rx12": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [0]
            }
        },
        "cw_00": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [0]
            }
        },
        "cw_01": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [1]
            }
        },
        "cw_02": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [2]
            }
        },
        "cw_03": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [3]
            }
        },
        "cw_04": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [4]
            }
        },
        "cw_05": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [5]
            }
        },
        "cw_06": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [6]
            }
        },
        "cw_07": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [7]
            }
        },
        "cw_08": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [8]
            }
        },
        "cw_09": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [9]
            }
        },
        "cw_10": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [0]
            }
        },
        "cw_11": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [1]
            }
        },
        "cw_12": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [2]
            }
        },
        "cw_13": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [3]
            }
        },
        "cw_14": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [4]
            }
        },
        "cw_15": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [5]
            }
        },
        "cw_16": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [6]
            }
        },
        "cw_17": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [7]
            }
        },
        "cw_18": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [8]
            }
        },
        "cw_19": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [9]
            }
        },
        "cw_20": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [0]
            }
        },
        "cw_21": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [1]
            }
        },
        "cw_22": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [2]
            }
        },
        "cw_23": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [3]
            }
        },
        "cw_24": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [4]
            }
        },
        "cw_25": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [5]
            }
        },
        "cw_26": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [6]
            }
        },
        "cw_27": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [7]
            }
        },
        "cw_28": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [8]
            }
        },
        "cw_29": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [9]
            }
        },
        "cw_30": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [0]
            }
        },
        "cw_31": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [1]
            }
        },
        "fl_cw_00": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [0]
            }
        },
        "fl_cw_01": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [1]
            }
        },
        "fl_cw_02": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [2]
            }
        },
        "fl_cw_03": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [3]
            }
        },
        "fl_cw_04": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [4]
            }
        },
        "fl_cw_05": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [5]
            }
        },
        "fl_cw_06": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [6]
            }
        },
        "fl_cw_07": {
            "duration": 80,
            "cc": {
                "ref_signal": "two-qubit-flux",
                "static_codeword_override": [7]
            }
        },
        "sf_cz_ne q2": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [1]
            }
        },
        "sf_cz_ne q3": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [1]
            }
        },
        "sf_cz_se q0": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [2]
            }
        },
        "sf_cz_se q2": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [2]
            }
        },
        "sf_cz_sw q1": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [3]
            }
        },
        "sf_cz_sw q2": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [3]
            }
        },
        "sf_cz_nw q2": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [4]
            }
        },
        "sf_cz_nw q4": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [4]
            }
        },
        "sf_park q3": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [5]
            }
        },
        "sf_park q4": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [5]
            }
        },
        "sf_sp_park": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [5]
            }
        },
        "sf_square": {
            "duration": 80,
            "cc": {
                "ref_signal": "single-qubit-flux",
                "static_codeword_override": [6]
            }
        }
    },
    "topology": {
        "edges": [
            { "id":  0, "src":  2, "dst":  0 },
            { "id":  1, "src":  2, "dst":  1 },
            { "id":  2, "src":  2, "dst":  3 },
            { "id":  3, "src":  2, "dst":  4 }
        ]
    },
    "resources": {
        "qubits": {
            "count": 5
        },
        "qwgs" : {
            "count": 5,
            "connection_map": {
                "0": [0],
                "1": [1],
                "2": [2],
                "3": [3],
                "4": [4]
            }
        },
        "meas_units" : {
            "count": 2,
            "connection_map": {
                "0": [0, 2, 3, 4],
                "1": [1]
            }
        },
        "edges": {
            "count": 4,
            "connection_map": {
                "0": [0, 2],
                "1": [1, 2],
                "2": [3, 2],
                "3": [4, 2]
            }
        }
    }
})";

}

/**
 * Preprocessing logic for the platform JSON configuration file. May be used
 * to generate/expand certain things that are always the same for that
 * platform, to save typing in the configuration file (and reduce the amount
 * of mistakes made).
 */
void Info::preprocess_platform(utils::Json &data) const {

    // TODO Wouter: any CC-specific platform configuration file preprocessing
    //  you might want to do for the resources can go here!

}

/**
 * Adds the default "backend passes" for this platform. Called by
 * pmgr::Manager::from_defaults() when no compiler configuration file is
 * specified. This typically includes at least the architecture-specific
 * code generation pass, but anything after prescheduling and optimization
 * is considered a backend pass.
 */
void Info::populate_backend_passes(pmgr::Manager &manager) const {

    // Mimic original CC backend.
    manager.append_pass(
        "sch.Schedule",
        "scheduler",
        {
#if OPT_CC_SCHEDULE_RC
            {"resource_constraints", "yes"}
#else
            {"resource_constraints", "no"}
#endif
        }
    );
    manager.append_pass(
        "arch.cc.gen.VQ1Asm",
        "codegen",
        {
            {"output_prefix", com::options::global["output_dir"].as_str() + "/%N"}
        }
    );

}

} // namespace cc
} // namespace arch
} // namespace ql
