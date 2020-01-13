.. _ccplatform:

Central Controller Platform Configuration
-----------------------------------------

This section describes the JSON configuration file format for OpenQL in conjunction
with the Central Controller (CC) backend

CC backend specific parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This section describes parameters that are specific for the OpenQL CC backend.

Instruction additions
*********************

.. table::
    :align: center

    =========================================== =========== ====================    ================
    Path                                        Type        Values                  Note
    =========================================== =========== ====================    ================
    instructions/<key>/cc/signal/type
    instructions/<key>/cc/signal/operand_idx
    instructions/<key>/cc/signal/value
    instructions/<key>/cc/signal_ref
    =========================================== =========== ====================    ================

'value' supports the following macro expansions:
* {gateName}
* {instrumentName}
* {instrumentGroup}
* {qubit}

Note that for the CC - contrary to the CC-light - the final hardware output is entirely *determined* by the contents of
the configuration file, there is no built-in knowledge of instrument connectivity or codeword organization.

CC Backend configuration
************************

FIXME: add description


Standard OpenQL features
^^^^^^^^^^^^^^^^^^^^^^^^

FIXME: just refer to relevant section. Kept here until we're sure this has been absorbed elsewhere

Standard parameters
*******************

This section describes parameters that are independent of the OpenQL backend used and
should therefore be compatible with other backends.


.. table::
    :align: center

    =================================== =========== ====================    ================
    Path                                Type        Values                  Note
    =================================== =========== ====================    ================
    eqasm_compiler                      String      "eqasm_backend_cc"
    hardware_settings/qubit_number      Int                                 number of qubits
    hardware_settings/cycle_time        Int                                 the clock cycle time of the device in [ns]
    instructions/<key>                                                      name for the instruction (NB: supports several naming schemes)
    instructions/<key>/duration         Int                                 duration in [ns]
    instructions/<key>/latency          Int                                 optional instruction latency in [ns] (effect unclear)
    instructions/<key>/matrix                                               the process matrix. Required, but generally does not contain useful information
    instructions/<key>/qubits           Optional
    gate_decomposition                  Optional
    =================================== =========== ====================    ================


Instruction names
*****************

The following syntaxes can be used for instruction names (i.e. the <key> in instructions/<key> as described above:

- "<name>"
- "<name><qubits>"

FIXME: special treatment of names by scheduler/backend
- "readout" : backend
- "measure"

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



Unused parameters
^^^^^^^^^^^^^^^^^

Non-implemented parameters
**************************

This section describes parameters that occur in documentation or example files, but are not actually implemented.


hardware_settings/mw_mw_buffer
hardware_settings/mw_flux_buffer
hardware_settings/mw_readout_buffer
hardware_settings/flux_mw_buffer
hardware_settings/flux_flux_buffer
hardware_settings/flux_readout_buffer
hardware_settings/readout_mw_buffer
hardware_settings/readout_flux_buffer
hardware_settings/readout_readout_buffer
instructions/<key>/disable_optimization
alias

CC-light parameters
*******************

instructions/<key>/cc_light_instr_type
instructions/<key>/cc_light_cond
instructions/<key>/cc_light_opcode
instructions/<key>/cc_light_codeword
instructions/<key>/cc_light_left_codeword
instructions/<key>/cc_light_right_codeword
resources/*
topology/*
