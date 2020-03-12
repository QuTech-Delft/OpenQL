#ifndef _QL_COMPILE_OPTIONS_H
#define _QL_COMPILE_OPTIONS_H

// the options below actually do not change the Python API at all if disabled
#define OPT_CUSTOM_GATE_LOAD            0   // inclusion of custom_gate::load, which isn't really used
#define OPT_CUSTOM_GATE_OPERATION_TYPE  0   // inclusion of custom_gate::operation_type, which isn't really used
#define OPT_CUSTOM_GATE_EXPLICIT_CTOR   0   // inclusion of custom_gate::custom_gate(everything), which isn't really used
#define OPT_CUSTOM_GATE_PARAMETERS      0   // inclusion of custom_gate::parameters, which isn't really used
#define OPT_LACKS_SWIG_INTERFACE        0   // functions not exposed through SWIG

#endif // ndef _QL_COMPILE_OPTIONS_H
