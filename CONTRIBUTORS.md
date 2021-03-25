
OpenQL framework has been created initially by Nader Khammassi.

Note: please fill your contributions in this file

- [Nader Khammassi](https://github.com/Nader-Khammassi)
    -   CBox Backend
    -   Configuration file support
    -   QASM loader for QASM syntax check
    -   C++ exceptions
    -   QISA map file generation
    -   QISA Control store generation

- [Imran Ashraf](https://github.com/imranashraf)
    -   support for hybrid classical/quantum compilation
    -   support for control flow (selection and repetition)
    -   kernel un-compute/conjugation feature
    -   multi-qubit control decomposition
    -   toffoli decompositions
    -   openql intermediate representation
    -   quantumsim simulator Backend
    -   compilation for CC-Light architecture
            - resource-constrained scheduling
            - parallel (SIMD and VLIW) QISA code generation
    -   flexible platform constraints specification and its implementation
    -   support for multi-qubit gates
    -   scheduling (ASAP/ALAP) algorithms
    -   parametrized gate decomposition
    -   unit-tests
    -   python Package for OpenQL
    -   cmake-based Compilation for cross-platform build setup
    -   conda recipies and packages
    -   single qubit flux operations
    -   cQASM v1.0 support
    -   OpenQL documentation

- [Adriaan Rol](https://github.com/AdriaanRol)
    -   Contributed to the Hardware Configuration Specification
    -   Utilizing qisa-as in unit-tests
    -   Testing OpenQL on the Hardware

- [Xiang Fu](https://github.com/gtaifu)
    -   Contributed to the Hardware Configuration Specification
    -   Testing OpenQL on the Hardware

- [Wouter Vlothuizen](https://github.com/wvlothuizen)
    -   backend for Central Controller (CC)
    -   new simplified qubit numbering scheme (rotated surface code fabric by 45 deg)
    -   support for comments in JSON file
    -   show line number and position on JSON syntax errors
    -   cleanup

- [Hans van Someren](https://github.com/jvansomeren)
    -   uniform scheduling algorithm
    -   resource constraint framework design
    -   resource constraint description for CC-Light architecture
    -   resource constrained list scheduling algorithms
    -   backward resource constraint checking
    -   forward and backward list scheduling algorithms
    -   gate commutation while scheduling
    -   clifford gate sequence optimization
    -   out of order gate creation
    -   staged decomposition description
    -   generalized passes, dumping and reporting
    -   platform topology specification and its implementation
    -   single qubit flux operations design
    -   initial placement mapping implementation
    -   basic routing implementation
    -   latency sensitive routing
    -   resource constrained routing
    -   scheduler integration into routing
    -   use moves next to swaps while routing
    -   crossbar spin-qubit scheduling and resource management
    -   recursive look-back and look-ahead routing
    -   arbitrary topology routing
    -   OpenQL documentation

- [Fer Grooteman](https://github.com/QFer)
    -   added interface (C++ and Python) to compile cQASM 1.0

- [Anneriet Krol](https://github.com/anneriet)
    -   unitary decomposition support

- [Razvan Nane](https://github.com/razvnane)
    -   compiler API and modularity support
    -   added C printer pass

- [Jeroen van Straten](https://github.com/jvanstraten)
    -   tutorial on DQCsim + OpenQL interoperation
    -   doxygen documentation
