.. _ccplatform:

Central Controller Platform Configuration
-----------------------------------------


CC configuration file
^^^^^^^^^^^^^^^^^^^^^

.. FIXME: improve readability

This section describes the JSON configuration file format for OpenQL in conjunction
with the Central Controller (CC) backend. Note that for the CC - contrary to the CC-light - the final hardware output is entirely *determined* by the contents of the configuration file, there is no built-in knowledge of instrument connectivity or codeword organization.

The CC configuration file consists of several sections described below.

To select the CC backend, the following is required:

.. code::

    "eqasm_compiler" : "eqasm_backend_cc",

``resources`` unused for the CC backend, section may be empty

``topology`` unused for the CC backend, section may be empty

``alias`` unused by OpenQL

``hardware_settings`` is used to configure various
hardware settings of the platform as shown below. These settings affect the
scheduling of instructions. Please refer to :ref:`platform` for a full description and an example.

The following settings are not used by the CC backend:

* hardware_settings/mw_mw_buffer
* hardware_settings/mw_flux_buffer
* hardware_settings/mw_readout_buffer
* hardware_settings/flux_mw_buffer
* hardware_settings/flux_flux_buffer
* hardware_settings/flux_readout_buffer
* hardware_settings/readout_mw_buffer
* hardware_settings/readout_flux_buffer
* hardware_settings/readout_readout_buffer

All settings related to the CC backend are in section ``hardware_settings/eqasm_backend_cc`` of the configuration file. This section is divided into several subsections as shown below.

Instrument definitions
**********************

Subsection ``instrument_definitions`` defines immutable properties of instruments, i.e. independent of the actual control setup:

.. code-block:: none
    :linenos:

    "instrument_definitions": {
        "qutech-qwg": {
            "channels": 4,
            "control_group_sizes": [1, 4],
        },
        "zi-hdawg": {
            "channels": 8,
            "control_group_sizes": [1, 2, 4, 8], // NB: size=1 needs special treatment of waveforms because one AWG unit drives 2 channels
        },
        "qutech-vsm": {
            "channels": 32,
            "control_group_sizes": [1],
        },
        "zi-uhfqa": {
            "channels": 9,
            "control_group_sizes": [1],
        }
    },   // instrument_definitions

Where:

* ``channels`` defines the number of logical channels of the instrument. For most instruments there is one logical channel per physical channel, but the 'zi-uhfqa' provides 9 logical channels on one physical channel pair.
* ``control_group_sizes`` states possible arrangements of channels operating as a vector

.. FIXME: add example
..    // * ``latency`` latency from trigger to output in [ns]. FIXME: deprecated
.. FIXME: describe concept of 'group'


Control modes
*************

Subsection ``control_modes`` defines modes to control instruments. These define which bits are used to control groups of channels and/or get back measurement results:

.. code-block:: JSON
    :linenos:

    "control_modes": {
        "awg8-mw-vsm-hack": {                     // ZI_HDAWG8.py::cfg_codeword_protocol() == 'microwave'. Old hack to skip DIO[8]
            "control_bits": [
                [7,6,5,4,3,2,1,0],                // group 0
                [16,15,14,13,12,11,10,9]          // group 1
            ],
            "trigger_bits": [31]
        },
        "awg8-mw-vsm": {                          // the way the mode above should have been
            "control_bits": [
                [7,6,5,4,3,2,1,0],                // group 0
                [15,14,13,12,11,10,9,8]           // group 1
            ],
            "trigger_bits": [31]
        },
        "awg8-mw-direct-iq": {                    // just I&Q to generate microwave without VSM. HDAWG8: "new_novsm_microwave"
            "control_bits": [
                [6,5,4,3,2,1,0],                  // group 0
                [13,12,11,10,9,8,7],              // group 1
                [22,21,20,19,18,17,16],           // group 2. NB: starts at bit 16 so twin-QWG can also support it
                [29,28,27,26,25,24,23]            // group 4
            ],
            "trigger_bits": [15,31]
        },
        "awg8-flux": {                             // ZI_HDAWG8.py::cfg_codeword_protocol() == 'flux'
            // NB: please note that internally one AWG unit handles 2 channels, which requires special handling of the waveforms
            "control_bits": [
                [2,1,0],                          // group 0
                [5,4,3],
                [8,7,6],
                [11,10,9],
                [18,17,16],                       // group 4. NB: starts at bit 16 so twin-QWG can also support it
                [21,20,19],
                [24,23,22],
                [27,26,25]                        // group 7
            ],
            "trigger_bits": [31]
        },
        "awg8-flux-vector-8": {                    // single code word for 8 flux channels.
            "control_bits": [
                [7,6,5,4,3,2,1,0]
            ],
            "trigger_bits": [31]
        },
        "uhfqa-9ch": {
            "control_bits": [[17],[18],[19],[20],[21],[22],[23],[24],[25]],    // group[0:8]
            "trigger_bits": [16],
            "result_bits": [[1],[2],[3],[4],[5],[6],[7],[8],[9]],              // group[0:8]
            "data_valid_bits": [0]
        },
        "vsm-32ch":{
            "control_bits": [
                [0],[1],[2],[3],[4],[5],[6],[7],                      // group[0:7]
                [8],[9],[10],[11],[12],[13],[14],[15],                // group[8:15]
                [16],[17],[18],[19],[20],[21],[22],[23],              // group[16:23]
                [24],[25],[26],[27],[28],[28],[30],[31]               // group[24:31]
            ],
            "trigger_bits": []                                       // no trigger
        }
    },   // control_modes

Where:

* ``<key>`` is a name which can be referred to from key ``instruments/[]/ref_control_mode``
* ``control_bits`` defines G groups of B bits, with:

    - G determines which the ``instrument_definitions/<key>/control_group_sizes`` used
    - B is an ordered list of bits (MSB to LSB) used for the code word
* ``trigger_bits`` vector of bits used to trigger the instrument. Must either be size 1 (common trigger) or size G (separate trigger per group), or 2 (common trigger duplicated on 2 bits, to support dual-QWG)
* ``result_bits`` reserved for future use
* ``data_valid_bits`` reserved for future use


Signals
*******

Subsection ``signals`` provides a signal library that gate definitions can refer to:

.. code-block:: JSON
    :linenos:

    "signals": {
        "single-qubit-mw": [
            {   "type": "mw",
                "operand_idx": 0,
                "value": [
                    "{gateName}-{instrumentName}:{instrumentGroup}-gi",
                    "{gateName}-{instrumentName}:{instrumentGroup}-gq",
                    "{gateName}-{instrumentName}:{instrumentGroup}-di",
                    "{gateName}-{instrumentName}:{instrumentGroup}-dq"
                ]
            },
            {   "type": "switch",
                "operand_idx": 0,
                "value": ["dummy"]                                  // NB: no actual signal is generated
            }
        ],
        "two-qubit-flux": [
            {   "type": "flux",
                "operand_idx": 0,                                   // control
                "value": ["flux-0-{qubit}"]
            },
            {   "type": "flux",
                "operand_idx": 1,                                   // target
                "value": ["flux-1-{qubit}"]
            }
        ]
    },  // signals

Where:

* ``<key>`` is a name which can be referred to from key ``instructions/<>/cc/ref_signal``. It defines an array of records with the fields below:

    * ``type`` defines a signal type. This is used to select an instrument that provides that signal type through key ``instruments/*/signal_type``. The types are entirely user defined, there is no builtin notion of their meaning.
    * ``operand_idx`` states the operand index of the instruction/gate this signal refers to. Signals must be defined for all operand_idx the gate refers to, so a 3-qubit gate needs to define 0 through 2. Several signals with the same operand_idx can be defined to select several signal types, as shown in "single-qubit-mw" which has both "mw" (provided by an AWG) and "switch" (provided by a VSM)
    * ``value`` defines a vector of signal names. Supports the following macro expansions:

        * {gateName}
        * {instrumentName}
        * {instrumentGroup}
        * {qubit}

.. FIXME:
    rewrite field 'operand_idx
    describe the (future) use of field 'value'
    expand


Instruments
***********

Subsection ``instruments`` defines instruments used in this setup, their configuration and connectivity.

.. code-block:: JSON
    :linenos:

    "instruments": [
        // readout.
        {
            "name": "ro_0",
            "qubits": [[6], [11], [], [], [], [], [], [], []],
            "signal_type": "measure",
            "ref_instrument_definition": "zi-uhfqa",
            "ref_control_mode": "uhfqa-9ch",
            "controller": {
                "name": "cc",
                "slot": 0,
                "io_module": "CC-CONN-DIO"
            }
        },
        // ...

        // microwave.
        {
            "name": "mw_0",
            "qubits": [                                             // data qubits:
                [2, 8, 14],                                         // [freq L]
                [1, 4, 6, 10, 12, 15]                               // [freq H]
            ],
            "signal_type": "mw",
            "ref_instrument_definition": "zi-hdawg",
            "ref_control_mode": "awg8-mw-vsm-hack",
            "controller": {
                "name": "cc",
                "slot": 3,
                "io_module": "CC-CONN-DIO-DIFF"
            }
        },
        // ...

        // VSM
        {
            "name": "vsm_0",
            "qubits": [
                [2], [8], [14], [],  [], [], [], [],                // [freq L]
                [1], [4], [6], [10], [12], [15], [], [],            // [freq H]
                [0], [5], [9], [13], [], [], [], [],                // [freq Mg]
                [3], [7], [11], [16], [], [], [], []                // [freq My]
            ],
            "signal_type": "switch",
            "ref_instrument_definition": "qutech-vsm",
            "ref_control_mode": "vsm-32ch",
            "controller": {
                "name": "cc",
                "slot": 5,
                "io_module": "cc-conn-vsm"
            }
        },

        // flux
        {
            "name": "flux_0",
            "qubits": [[0], [1], [2], [3], [4], [5], [6], [7]],
            "signal_type": "flux",
            "ref_instrument_definition": "zi-hdawg",
            "ref_control_mode": "awg8-flux",
            "controller": {
                "name": "cc",
                "slot": 6,
                "io_module": "CC-CONN-DIO-DIFF"
            }
        },
        // ...
    ] // instruments

Where:

* ``name`` a friendly name for the instrument
* ``ref_instrument_definition`` selects record under ``instrument_definitions``, which must exist or an error is raised
* ``ref_control_mode`` selects record under ``control_modes``, which must exist or an error is raised
* ``signal_type`` defines which signal type this instrument instance provides.
* ``qubits`` G groups of 1 or more qubits. G must match one of the available group sizes of ``instrument_definitions/<ref_instrument_definition>/control_group_sizes``. If more than 1 qubits are stated per group - e.g. for an AWG used in conjunction with a VSM - they may not produce conflicting signals at any time slot, or an error is raised
* ``force_cond_gates_on`` optional, reserved for future use
* ``controller/slot`` the slot number of the CC this instrument is connected to
* ``controller/name`` reserved for future use
* ``controller/io_module`` reserved for future use

.. FIXME: describe matching process of 'signal_type' against 'signals/*/type'

Additions to section 'instructions'
***********************************

The CC backend extends section ``instructions/<key>`` with a subsection ``cc`` as shown in the example below:

.. code-block:: JSON
    :linenos:

    "ry180": {
        "duration": 20,
        "matrix": [ [0.0,1.0], [1.0,0.0], [1.0,0.0], [0.0,0.0] ],
        "cc": {
            "ref_signal": "single-qubit-mw",
            "static_codeword_override": [2]
        }
    },
    "cz_park": {
        "duration": 40,
        "matrix": [ [0.0,1.0], [1.0,0.0], [1.0,0.0], [0.0,0.0] ],
        "cc": {
            "signal": [
                {   "type": "flux",
                    "operand_idx": 0,                                   // control
                    "value": ["flux-0-{qubit}"]
                },
                {   "type": "flux",
                    "operand_idx": 1,                                   // target
                    "value": ["flux-1-{qubit}"]
                },
                {   "type": "flux",
                    "operand_idx": 2,                                   // park
                    "value": ["park_cz-{qubit}"]
                }
            ],
            "static_codeword_override": [1,2,3]
        }
    }
    "_wait_uhfqa": {
        "duration": 220,
        "matrix": [ [0.0,1.0], [1.0,0.0], [1.0,0.0], [0.0,0.0] ],
        "cc": {
            "signal": []
        }
    },
    "_dist_dsm": {
        "duration": 20,
        "matrix": [ [0.0,1.0], [1.0,0.0], [1.0,0.0], [0.0,0.0] ],
        "cc": {
            "readout_mode": "feedback",
            "signal": [
                {	"type": "measure",
                    "operand_idx": 0,
                    "value": []
                }
            ]
        }
    },
    "_wait_dsm": {
        "duration": 80,
        "matrix": [ [0.0,1.0], [1.0,0.0], [1.0,0.0], [0.0,0.0] ],
        "cc": {
            "signal": []
        }
    },
    "if_1_break": {
        "duration": 60,
        "matrix": [ [0.0,1.0], [1.0,0.0], [1.0,0.0], [0.0,0.0] ],
        "cc": {
            "signal": [],
            "pragma": {
                "break": 1
            }
        }
    }

Where:

* ``cc/ref_signal`` points to a signal definition in ``hardware_settings/eqasm_backend_cc/signals``, which must exist or an error is raised
* ``cc/signal`` defines a signal in place, in an identical fashion as ``hardware_settings/eqasm_backend_cc/signals``. May be empty (``[]``) to disable signal generation.
* ``cc/static_codeword_override`` provides a user defined array of codeword (one entry per operand) for this instruction. Currently, this key is compulsory (if signal is non-empty), but in the future, codewords will be assigned automatically to make better use of limited codeword space
* ``cc/readout_mode`` defines an instruction to perform readout if non-empty. If the value "feedback" is used, code is generated to read and distribute the instrument result.
* ``cc/pragma/break`` enables special functionality which makes the gate break out of a for loop if the associated qubit was measured as 1 (``"pragma" { "break": 1 }``) or 0 (``"pragma" { "break": 0 }``

The following standard OpenQL fields are used:

* ``<key>`` name for the instruction. The following syntaxes can be used for instruction names:

    - "<name>"
    - "<name><qubits>"
* ``duration`` duration in [ns]
* ``matrix`` the process matrix. Required, but only used if optimization is enabled
* ``latency`` optional instruction latency in [ns], used by scheduler
* ``qubits`` optional

.. FIXME: special treatment of names by scheduler/backend
    - "readout" : backend
    - "measure"

The following fields in 'instructions' are not used by the CC backend:

* ``type``
* ``cc_light_instr``
* ``cc_light_instr_type``
* ``cc_light_cond``
* ``cc_light_opcode``
* ``cc_light_codeword``
* ``cc_light_left_codeword``
* ``cc_light_right_codeword``
* ``disable_optimization`` not implemented in OpenQL

Program flow feedback
************************************************

To support Repeat Until Success type experiments, two special fields were added to the gate definition for the CC, as
shown in the previous section:

-   the `"readout_mode": "feedback"` clause in the `"_dist_dsm"` gate causes the backend to generate code to retrieve the
    measurement result from the DIO interface and distribute it across the CC;
-   the `"pragma": { "break": 1 }` clause  in the `"if_1_break"` gate causes the backend to generate code to break out
    of a OpenQL loop if the associated qubit is read as 1 (or similarly if 0).

For convenience, the gate decomposition section can be extended with
`"measure_fb %0": ["measure %0", "_wait_uhfqa %0", "_dist_dsm %0", "_wait_dsm %0"]`

This creates a `measure_fb` instruction consisting of four parts:
-   triggering a measurement (on the UHFQA)
-   waiting for the internal processing time of the UHFQA
-   retrieve the measurement result, and distribute it across the CC
-   wait fot the data distribution to finish

The following example code contains a real RUS experiment using PycQED:

.. code-block:: Python

    from pycqed.measurement.openql_experiments import openql_helpers as oqh
    for i, angle in enumerate(angles):
        oqh.ql.set_option('output_dir', 'd:\\githubrepos\\pycqed_py3\\pycqed\\measurement\\openql_experiments\\output')
        p = oqh.create_program('feedback_{}'.format(angle), config_fn)
        k = oqh.create_kernel("initialize_block_{}".format(angle), p)

        # Initialize
        k.prepz(qidx)

        # Block do once (prepare |1>)
        k.gate("rx180", [qidx])
        p.add_kernel(k)

        # Begin conditional block
        q = oqh.create_kernel("conditional_block_{}".format(angle), p)
        # Repeat until success 0
        q.gate("measure_fb", [qidx])
        q.gate("if_0_break", [qidx])

        # Correction for result 1
        q.gate("rx180", [qidx])
        p.add_for(q, 1000000)

        # Block finalize
        r = oqh.create_kernel("finalize_block_{}".format(angle), p)
        cw_idx = angle // 20 + 9
        r.gate('cw_{:02}'.format(cw_idx), [qidx])

        # Final measurement
        r.gate("measure_fb", [qidx])
        p.add_kernel(r)

        oqh.compile(p, extra_openql_options=[('backend_cc_run_once', 'yes')])

Caveats:

-   it is not possible to mix `measure_fb` and `measure` in a single program. This is a consequence of the way measurements
    are read from the input DIO interface of the CC: every measurement (both from `measure_fb` and `measure`) is pushed
    onto an input FIFO. This FIFO is only popped by a `measure_fb` instruction. If the two types are mixed, misalignment
    occurs between what is written and read. No check is currently performed by the backend.
-   `break` statements may only occur inside a `for` loop. No check is currently performed by the backend.
-   `break` statements implicitly refer to the last `measure_fb` earlier in code as a result of implicit allocation of
    variables.

These limitations will vanish when integration with cQASM 2.0 is completed.


.. FIXME:
    add conditional gates


.. FIXME: TBW
    Converting quantum gates to instrument codewords
    *******************************************************


Compiler options
^^^^^^^^^^^^^^^^

The following OpenQL compiler options are specific for the CC backend:

* ``backend_cc_run_once`` create a .vq1asm program that runs once instead of repeating indefinitely (default "no" to maintain compatibility, alternatively "yes")
* ``backend_cc_verbose`` add verbose comments to generated .vq1asm file (default "yes", alternatively "no")
* ``backend_cc_map_input_file`` name of CC input map file, default "". Reserved for future extension to generate codewords automatically

FIXME: refer to standard options


CC backend output files
^^^^^^^^^^^^^^^^^^^^^^^

.vq1asm: 'Vectored Q1 assembly' file for the Central Controller

.vcd: timing file, can be viewed using GTKWave (http://gtkwave.sourceforge.net)

Standard OpenQL features
^^^^^^^^^^^^^^^^^^^^^^^^

FIXME: just refer to relevant section. Kept here until we're sure this has been absorbed elsewhere


Parametrized gate-decomposition
*******************************

Parametrized gate decompositions can be specified in gate_decomposition section, as shown below:

    "rx180 %0" : ["x %0"]

Based on this, k.gate('rx180', 3) will be decomposed to x(q3). Similarly, multi-qubit gate-decompositions can be
specified as:

    "cnot %0,%1" : ["ry90 %0", "cz %0,%1", "ry90 %1"]


Specialized gate-decomposition
******************************

Specialized gate decompositions can be specified in gate_decomposition section, as shown below:

    "rx180 q0" : ["x q0"]
    "cz_park q0,q1" : ["cz q0,q1", "park q3"]
