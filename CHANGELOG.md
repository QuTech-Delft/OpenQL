# Changelog
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).


## [ next ] - [ TBD ]
### Added
- CC backend:
  - support for cQASM 1.2 features through new IR 
  - limitations
    - integer values must be non-negative
    - 

### Changed
- CC backend:
  - now uses new IR
  - no longer requires key "cc" to be present in instructions that define gate decompositions

### Removed
- CC backend:
  - deprecated JSON key "pragma/break" for instruction definitions
  - macro expansion for JSON key instruction/signal/value (unused anyway) 

### Fixed
- ...


## [ 0.10.0 ] - [ 2021-07-15 ]
### Added
- scalability options for coping with large multi-core systems, including a progress bar for the mapping process
- initial implementation of the Diamond architecture developed for Fujitsu (lead by Stephan Wong)
- full cQASM 1.2 read and write support, with options for different version levels and various language quirks
- new internal representation that encompasses the entire cQASM 1.2 language, and has many new generalized platform features
- lossless conversion functions between the two IR representations until all passes have been converted to the new representation
- new pass-based decomposition logic that supports arbitrary cQASM 1.2 code for the expansion and doesn't clobber scheduling information
- new pass for converting structured cQASM 1.2 control flow to basic block form
- new list scheduler based on the new IR

### Changed
- all written cQASM files are now in 1.2 format by default
- the cQASM reader no longer has a JSON configuration file for mapping cQASM gates to OpenQL gates; this translation is now part of OpenQL's platform data
- the old scheduler is replaced with a new implementation for most option variations, that outputs slightly different schedules
- statistics report output is also formatted slightly different, though information content is the same
- CC backend:
    - scheduling is now done using resource constraints by default

### Fixed
- excessive memory usage and slow platform construction for large multi-core systems


## [ 0.9.0 ] - [ 2021-05-27 ]
### Added
- architecture system: platform and compilation strategy defaults are now built into OpenQL, preventing the need for users to copypaste configuration files from the tests directory
- interface (C++ and Python) to compile cQASM 1.x
- allow 'wait' and 'barrier' in JSON section 'gate_decomposition'
- CC backend:
    - improved reporting on JSON semantic errors
    - added check for dimension of "instruments/qubits" against "instruments/ref_control_mode/control_bits"
    - added check for dimension of "instructions/<key>/cc/[signals,ref_signal]/value" against "instruments/ref_control_mode/control_bits"
    - added cross check of "instruments/ref_control_mode" against "instrument_definitions"
    - added support for "pragma/break" in JSON definition to define 'gate' that breaks out of loop
    - added support to distribute measurement results via DSM
    - added support for conditional gates
    - added compile option "--backend_cc_run_once"
    - added compile option "--backend_cc_verbose"

### Changed
- pass management: instead of a hardcoded compilation strategy, the strategy can be adjusted and fine-tuned manually
- pass options: instead of doing everything with global options, global options were replaced with pass options as much as possible
- most documentation is now generated from code and can be queried using API calls
- scheduler resources are completely reworked to be made more generic
- major internal refactoring and restructuring to facilitate the above two things
- CC backend:
    - renamed JSON field "signal_ref" to "ref_signal"
    - renamed JSON field "ref_signals_type" to "signal_type"
    - changed JSON field "static_codeword_override" to be a vector with one element per qubit parameter. To edit a JSON file using Sublime, use Replace with Regular Expressions: find=`"static_codeword_override": ([0-9])+`, replace=`"static_codeword_override": [\1]`
    - adopted new module synchronization scheme ("seq_bar semantics", requires CC software >= v0.2.0, PycQED after commit 470df5b)
    - JSON field "instruction/type" no longer used by backend, use "instruction/cc/readout_mode" to flag measurement instructions
    - allow specification of 2 triggers in JSON field "control_modes/*/trigger_bits" to support dual-QWG
    - changed label in generated code from "mainLoop" to "__mainLoop". Do not start kernel names with "__" (this should be specified by the API)
    - removed initial 1 cycle (20 ns) delay at start of kernels (resulting from bundle start_cycle starting at 1)
    - correctly handle kernel names containing "_" in conjunction with looping (formerly duplicate labels could arise)
    - added "seq_out 0,1" to program start to allow tracing of actual program start

### Removed
- CC-light code generation, as the CC-light is being phased out in the lab, and its many passes were obstacles for pass management and refactoring
- rotation optimization based on matrices; matrices in general were removed entirely because no one was using it
- the commute variation pass, as it has been superseded by in-place commutations within the scheduler
- the toffoli decomposition pass, as it wasn't really used; to decompose a toffoli gate, use generic platform-driven decomposition instead
- the defunct fidelity estimation logic from metrics.cc; this may be added again later, but requires lots of cleanup and isn't currently in use
- quantumsim and qsoverlay output; apparently this was no longer being used, and it was quite intertwined with the CC-light backend


### Fixed
- changed register used for FOR loop, so it doesn't clash with delay setting
- fixed documentation for python setup and running tests
- various miscellaneous bugs, dangling pointers, and memory leaks


## [ 0.8.0 ] - [ 2019-10-31 ]
### Added
- support for CC backend

### Changed

### Removed

### Fixed
- fixed issue with duplicate kernel names
- updated json library to fix osx builds


## [ 0.7.1 ] - [ 2019-09-02 ]
### Added

### Changed
- re-factored folders

### Removed

### Fixed
- fixed issue with correct python library picking on tud win systems


## [ 0.7.0 ] - [ 2019-06-03 ]
### Added
- support for single qubit flux options (auto/manual modes)
- option to control generation of qasm files and dot graphs
- NPROCS=n variable can now be set for faster compilation to use n threads
- conda build recipe
- conda binary releases for Linux, Windows platform (not yet available for OSX due to a conda distribution issue)


### Changed
- openql is now public
- improved resource-constrained scheduling
- sweep point array is now optional
- support for barrier/wait on all qubits


### Removed
- set_sweep_points(sweep_points list, num of sweep points)

### Fixed
- resource-constrained qasm is generated by same scheduler for cc-light as is used to generate qisa
- Illegal parameter in gate_decomposition


## [ 0.6 ] - [ 2018-10-29 ]
### Added
- generated qasm code conforms to cQASM v1.0 specification
- added libqasm to pytest to test conformance of generated qasm

### Changed
- ALAP scheduler is the default option (Issue #193)
- compiling an empty program raises error (Issue #164)

### Removed

### Fixed
- tests are added to test option setting/getting (Issue #190)


## [ 0.5.5 ] - [ 2018-10-25 ]
### Added

### Changed
- simplified interface of Program.set_sweep_points (Issue #184)

### Removed

### Fixed
- instruction ordering to generate consistent qisa (Issue #190)
- stateful behaviour in OpenQL (Issue #171)


## [ 0.5.4 ] - [ 2018-10-17 ]
### Added

### Changed

### Removed

### Fixed
- qubit ordering in SMIS and SMIT instructions


## [ 0.5.3 ] - [ 2018-10-11 ]
### Added
- added detuning constraints for cclight

### Changed

### Removed

### Fixed
- alap scheduling for cclight


## [ 0.5.2 ] - [ 2018-10-10 ]
### Added

### Changed

### Removed

### Fixed
- wrong target qubits in the configuration files
- Jenkins test build profile to test against assembler


## [ 0.5.1 ] - [ 2018-09-12 ]
### Added
- API to obtain version number

### Changed

### Removed

### Fixed
- qisa format (removed comma)


## [ 0.5 ] - [ 2018-06-26 ]
### Added
- support for classical instructions
- support for flow control (selection and repetition)
- classical register manager implementation

### Changed
- measure instruction updated to support classical target register
- kernels are not any more fused to generate a single qisa program

### Removed
- kernel does not recieve iteration count, deprecated in favor of for-loop

### Fixed
- qisa format (pre-interval syntax updated)

## [ 0.4.1 ] - [ 2018-05-31 ]
### Added
-

### Changed
-

### Removed
-

### Fixed
- getting started example


## [ 0.4 ] - [ 2018-05-19 ]
### Added
- kernel conjugation/un-compute feature
- multi-qubit control decomposition
- toffoli decomposition
- QASM loader for QASM v1.0 syntax check
- initial support for Quantumsim backend
- vebosity levels


### Changed
- program options can be set/get with simple api calls
- when adding gates, qubits should always be specified as list
- updated qisa-as support for tests

### Removed
- qisa-as is not a part of openql
- prog.compile() does not get optimiz/schedule/verbose options

### Fixed
- static iteration count for scheduled qasm
- roation angle printing


## [ 0.3 ] - [ 2017-10-24 ]
### Added
- CCLight eQASM compiler
- unittests using qisa-as
- simplified gate decompositions
- wait/barrier instructions


### Changed
-

### Removed
-

### Fixed
- varying prepz duration
- M_PI issue in windows install


## [ 0.2 ] - [ 2017-08-18 ]
### Added
- CBox eQASM compiler
- Python and C++ interface
- Configuration file specifiction
- trace support for qumis code
- cmake based builds

### Changed
-

### Removed
-

### Fixed

