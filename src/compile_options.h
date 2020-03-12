#ifndef _QL_COMPILE_OPTIONS_H
#define _QL_COMPILE_OPTIONS_H

// the options below actually do not change the Python API at all if disabled
#define OPT_CUSTOM_GATE_PARAMETERS      0   // inclusion of custom_gate::parameters, which isn't really used
#define OPT_LACKS_SWIG_INTERFACE        0   // functions not exposed through SWIG

#endif // ndef _QL_COMPILE_OPTIONS_H
