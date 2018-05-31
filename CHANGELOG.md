# Change Log
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [ 0.5 ] - [ 2018-xx-xx ]
### Added
-

### Changed
-

### Removed
- 

### Fixed
-

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
