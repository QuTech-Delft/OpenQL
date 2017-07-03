## Specifying experimental configuration

The goal of the config is to specify the information required to convert operations to qumis instructions.  It should be emphasized that flexibility and extensibility of this format is a driving requirement.  The config is specified in JSON and contains a nested dictionary with the required information for **every** allowed operation including it's arguments.  If there is no entry in the configuration for a specific operation with it's arguments then it is not allowed.  There is a different entry in the configuration for different arguments of the same function: e.g., `X180(q0)` and `X180(q1)` would be two distinct entries in the config. In addition to the configuration entries there are several globally defined parameters. 

## Globally defined parameters

- `qubit names` (list) : list of qubit names (not needed?)
- `cycle_time` (int) : the clock cycle time of the device running the qumis

- `MW_MW_buffer` (int) : buffer between pulses of type MW and MW in ns
- `MW_Flux_buffer` (int) : buffer between pulses of type MW and Flux in ns
- `MW_RO_buffer` (int) : buffer between pulses of type MW and RO in ns
- `Flux_MW_buffer` (int) : buffer between pulses of type Flux and MW in ns
- `Flux_Flux_buffer` (int) : buffer between pulses of type Flux and Flux in ns
- `Flux_RO_buffer` (int) : buffer between pulses of type Flux and RO in ns
- `RO_MW_buffer` (int) : buffer between pulses of type RO and MW in ns
- `RO_Flux_buffer` (int) : buffer between pulses of type RO and Flux in ns
- `RO_RO_buffer` (int) : buffer between pulses of type RO and RO in ns

## Configuration entries
Entries are either an `alias` or a full entry. 

`entry` (str) : unique id for the operation with arguments formatted as `op_name(arg1, ..., argN)`

- `alias` entries only contain one item:
    + `alias` (str) : name of operation +args it is an alias of
- full entries contain the following pieces of information 
    + `duration` (int) : duration of the operation in ns
    + `latency` (int): latency of operation in ns
    + `qubits` (list) : what qubits this operation affects (can be empty list)
    + `matrix` (matrix): the process matrix, can be an empty matrix.
    + `target_matrix` (matrix): the ideal process matrix of the operation, can be an empty matrix.
    + type (str): one of either `MW`, `Flux`, `RO`, `None`
    + `qumis_instr` (str): one of `wait`, `pulse`, `trigger`, `CW_trigger`, `dummy`, `measure`.
    + `qumis_instr_kw` (dict): dictionary containing keyword arguments for the qumis instruction. 

## Qumis instruction 
OpenQL supports the following instructions

- `wait` 
    + time (int): time to wait in ns, must be multiple of `cycle_time`
- `pulse` plays a waveform from one of the internal AWG's
    + awg_nr (int): index specifying the awg to play the pulse on 
    + lut_idx (int) : index of the lookuptable that stores the waveform
- `trigger` : raises a trigger for a specified time
    + `trigger_bit` (int) : trigger_bit
    + `trigger_duration` (int): duration for which to set the trigger bit high
- `CW_trigger`
    + `trigger_bit` (int) : trigger_bit
    + `trigger_duration` (int): duration for which to set the trigger bit high
    + `CW_bits` (list of int) : bits specifying the bits available for the codeword
    + `codeword_duration` (int): duration for which to raise the codeword bits high
    + `codeword` (int) : integer specifying what bits to raise from the CW_bits
- `dummy` blocks the qubit for the duration specified but executes no instruction
    + no arguments
- `measure`
    + `measurement_duration` (int): duration of the measurement instruction.



## Potential extensibility and current flaws

- **New qumis instructions** will be needed as hardware changes (e.g., vector switch matrix) or to support different platforms (spin-qubits, NV-centers, etc). This should not affect the structure of this config but will change the content in the future.
- **Classical logic** is not specified in this document but does relate to the underlying qumis instructions, e.g, operations conditional on measurement outcomes.
- **Composite qumis instructions**. In the current proposal it is not possible to specify composite qumis instructions even though there is a clear potential for this. An example would be quickly putting a copy of a pulse on a different channel or triggering a scope. 
- The format is flattened out completely, not taking use of any structure in the configuration. This is on purpose.

## Example use of aliases

- CZ(q0, q1) -> CZ(q1, q0)
- CZ_11(q0, q1) -> CZ_11(q1, q0)
- CZ_01(q0, q1) -> CZ_10(q1, q0) 

- X180(q0) -> X(q0)
- X180(q0) -> rX180(q0)


