#ifndef _QL_COMPILE_OPTIONS_H
#define _QL_COMPILE_OPTIONS_H

// deprecation options
#define OPT_MICRO_CODE                  0   // enable old support for CBOX microcode
#define OPT_TARGET_PLATFORM             0   // use target_platform, which is not actually used
#define OPT_UNFINISHED_OPTIMIZATION     0   // enable unfinished optimization that actually did nothing
#define OPT_USED_HARDWARE               0   // inclusion of custom_gate::used_hardware, which isn't really used
#define OPT_CUSTOM_GATE_LOAD            0   // inclusion of custom_gate::load, which isn't really used

#endif // ndef _QL_COMPILE_OPTIONS_H
