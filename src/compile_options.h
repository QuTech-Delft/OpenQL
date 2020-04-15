#ifndef _QL_COMPILE_OPTIONS_H
#define _QL_COMPILE_OPTIONS_H

// deprecation options, should be removed in the future.
#define OPT_MICRO_CODE                  0   // enable old support for CBOX microcode

// the options below actually do not change the Python API at all if disabled
#define OPT_TARGET_PLATFORM             0   // use target_platform, which is not actually used
#define OPT_UNFINISHED_OPTIMIZATION     0   // enable unfinished optimization that actually did nothing
#define OPT_USED_HARDWARE               0   // inclusion of custom_gate::used_hardware, which isn't really used
#define OPT_CUSTOM_GATE_LOAD            0   // inclusion of custom_gate::load, which isn't really used
#define OPT_CUSTOM_GATE_OPERATION_TYPE  0   // inclusion of custom_gate::operation_type, which isn't really used
#define OPT_CUSTOM_GATE_EXPLICIT_CTOR   0   // inclusion of custom_gate::custom_gate(everything), which isn't really used
#define OPT_CUSTOM_GATE_PARAMETERS      0   // inclusion of custom_gate::parameters, which isn't really used
#define OPT_LACKS_SWIG_INTERFACE        0   // functions not exposed through SWIG

// new features
#define OPT_DECOMPOSE_WAIT_BARRIER      1   // allow wait/barrier in JSON section gate_decomposition

#endif // ndef _QL_COMPILE_OPTIONS_H
