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
Str switchOn(UInt arg) {
    Str body = "switchOn q" + to_string(arg);
    return body;
}

Str switchOff(UInt arg) {
    Str body = "switchOff q" + to_string(arg);
    return body;
}

Str loadimm(Str value, Str reg_name, Str reg_value) {
    Str body = "LDi " + value + ", " + reg_name + to_string(reg_value);
    return body;
}

Str mov(Str reg1_name, Str reg1_value, Str reg2_name, Str reg2_value) {
    Str body = "mov " + reg1_name + reg1_value + ", " + reg2_name + reg2_value;
    return body;
}

Str excite_mw(Str envelope, Str duration, Str frequency, Str phase, UInt qubit){
    Str body_1 = "excite_MW " + to_string(envelope) + ", " +  to_string(duration) + ", ";
    Str body_2 = to_string(frequency) + ", " + to_string(phase) + ", q" + to_string(qubit);
    Str body = body_1 + body_2;
    return body;
}

Str branch(Str name_1, Str value_1, Str comparison, Str name_2, Str value_2, Str target_name, Str target_value) {
    Str compare = name_1 + to_string(value_1) + comparison + name_2 + to_string(value_2);
    Str body = "BR " + compare +", " + target_name + to_string(target_value);
    return body;
}

Str label(Str labelcount){
   Str body = "LABEL LAB" + labelcount;
   return body;
}

Str qgate(Str gatename, utils::Vec<utils::UInt> operand) {
    Str body_1 = "qgate " + to_upper(gatename) + ", ";
    Str body_2 = "q" + to_string(operand[0]);
    Str body = body_1 + body_2;
    return body;
}

Str qgate2(Str gatename, Str operand_1, Str operand_2) {
    Str body_1 = "qgate2 " + to_upper(gatename) + ", ";
    Str body_2 = operand_1 + ", " + operand_2;
    Str body = body_1 + body_2;
    return body;
}

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

Str add(Str name_1, Str value_1, Str name_2, Str value_2, Str name_3, Str value_3) {
    Str body = "ADD " + name_1 + value_1 + ", " + name_2 + value_2 + ", " + name_3 + value_3;
    return body;
}

Str addimm(Str value, Str regname, Str regvalue) {
    Str body = "ADDi " + regname + regvalue + ", " + value;
    return body;
}

Str jump(Str labelcount) {
    Str body = "JUMP " + labelcount;
    return body;
}


} // namespace detail
} // namespace microcode
} // namespace gen
} // namespace pass
} // namespace diamond
} // namespace arch
} // namespace ql