#include "ql/utils/str.h"
#include "ql/utils/filesystem.h"
#include "ql/plat/platform.h"
#include "ql/com/options.h"
#include "functions.h"


namespace ql {
namespace arch {
namespace diamond {
namespace pass {
namespace gen {
namespace microcode {
namespace detail {

using namespace utils;

// Defines the instruction for setting the optical switches to the correct qubit.
Str switchOn(unsigned long arg) {
    Str body = "switchOn " + to_string(arg);
    return body;
}

Str switchOff(unsigned long arg) {
    Str body = "switchOff " + to_string(arg);
    return body;
}

Str loadimm(Str value, Str reg_name, Str reg_value) {
    Str body = "LDi " + value + ", " + reg_name + reg_value;
    return body;
}

Str mov(Str reg1_name, Str reg1_value, Str reg2_name, Str reg2_value) {
    Str body = "mov " + reg1_name + reg1_value + ", " + reg2_name + reg2_value;
    return body;
}


} // namespace detail
} // namespace microcode
} // namespace gen
} // namespace pass
} // namespace diamond
} // namespace arch
} // namespace ql