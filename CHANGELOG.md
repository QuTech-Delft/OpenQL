# Change Log
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

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
-
