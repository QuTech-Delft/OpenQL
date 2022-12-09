#include "ql/utils/str.h"
#include "ql/utils/filesystem.h"
#include "ql/ir/compat/platform.h"
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

/**
 * Function for switching on the optical path
 */
Str switchOn(UInt arg) {
    Str body = "switchOn q" + to_string(arg);
    return body;
}

/**
 * Function for switching off the optical path
 */
Str switchOff(UInt arg) {
    Str body = "switchOff q" + to_string(arg);
    return body;
}

/**
 * Function for load immediate (LDi)
 */
Str loadimm(Str value, Str reg_name, Str reg_value) {
    Str body = "LDi " + value + ", " + reg_name + to_string(reg_value);
    return body;
}

/**
 * Function for move (mov)
 */
Str mov(Str reg1_name, Str reg1_value, Str reg2_name, Str reg2_value) {
    Str body = "mov " + reg1_name + reg1_value + ", " + reg2_name + reg2_value;
    return body;
}

/**
 * Function for exciting the qubit with a custom laser (excite_mw)
 */
Str excite_mw(Str envelope, Str duration, Str frequency, Str phase, Str amp, UInt qubit){
    Str body_1 = "excite_MW " + to_string(envelope) + ", " +  to_string(duration) + ", ";
    Str body_2 = to_string(frequency) + ", " + to_string(phase) + ", " +
        to_string(amp)+ ", q" + to_string(qubit);
    Str body = body_1 + body_2;
    return body;
}

/**
 * Function for branch instructions (br)
 */
Str branch(Str name_1, Str value_1, Str comparison, Str name_2, Str value_2, Str target_name, Str target_value) {
    Str compare = name_1 + to_string(value_1) + comparison + name_2 + to_string(value_2);
    Str body = "BR " + compare +", " + target_name + to_string(target_value);
    return body;
}

/**
 * Function for creating a label (label)
 */
Str label(Str labelcount){
   Str body = "LABEL LAB" + labelcount;
   return body;
}

/**
 * Function for single qubit gate (qgate)
 */
Str qgate(Str gatename, UInt operand) {
    Str body_1 = "qgate " + to_upper(gatename) + ", ";
    Str body_2 = "q" + to_string(operand);
    Str body = body_1 + body_2;
    return body;
}

/**
 * Function for two-qubit gate (qgate2)
 */
Str qgate2(Str gatename, Str operand_1, Str operand_2) {
    Str body_1 = "qgate2 " + to_upper(gatename) + ", ";
    Str body_2 = operand_1 + ", " + operand_2;
    Str body = body_1 + body_2;
    return body;
}

/**
 * Function for storing information in memory (ST)
 */
Str store(Str reg_name1, Str reg_value1, Str reg_name2, Str reg_value2, Str memaddr){
    Str body_1 = "ST " + reg_name1 + reg_value1 + ", " + reg_name2 + reg_value2 + "($" + memaddr + ")";

//    Str body_2;
//    if (!memaddr.empty()) {
//        Str body_2 = "(" + memaddr + ")";
//    } else {
//    }
//    Str body = body_1 + body_2;
    return body_1;
}

/**
 * Function for addition (ADD)
 */
Str add(Str name_1, Str value_1, Str name_2, Str value_2, Str name_3, Str value_3) {
    Str body = "ADD " + name_1 + value_1 + ", " + name_2 + value_2 + ", " + name_3 + value_3;
    return body;
}

/**
 * Function for immediate addition (ADDi)
 */
Str addimm(Str value, Str regname, Str regvalue) {
    Str body = "ADDi " + regname + regvalue + ", " + value;
    return body;
}

/**
 * Function for unconditional jump (jump)
 */
Str jump(Str labelcount) {
    Str body = "JUMP LAB" + labelcount;
    return body;
}


} // namespace detail
} // namespace microcode
} // namespace gen
} // namespace pass
} // namespace diamond
} // namespace arch
} // namespace ql