#pragma once

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
Str switchOn(UInt qubit_path );

/**
 * Function for switching off the optical path
 */
Str switchOff(UInt qubit_path );

/**
 * Function for load immediate (LDi)
 */
Str loadimm(Str value, Str reg_name, Str reg_value);

/**
 * Function for move (mov)
 */
Str mov(Str reg1_name, Str reg1_value, Str reg2_name, Str reg2_value);

/**
 * Function for exciting the qubit with a custom laser (excite_mw)
 */
Str excite_mw(Str envelope, Str duration, Str frequency, Str phase, Str amp, UInt qubit);

/**
 * Function for branch instructions (br)
 */
Str branch(Str name_1, Str value_1, Str comparison, Str name_2, Str value_2, Str target_name, Str target_value);

/**
 * Function for creating a label (label)
 */
Str label(Str labelcount);

/**
 * Function for single qubit gate (qgate)
 */
Str qgate(Str gatename, UInt operand);

/**
 * Function for two-qubit gate (qgate2)
 */
Str qgate2(Str gatename, Str operand_1, Str operand_2);

/**
 * Function for storing information in memory (ST)
 */
Str store(Str reg_name1, Str reg_value1, Str reg_name2, Str reg_value2, Str memaddr);

/**
 * Function for addition (ADD)
 */
Str add(Str name_1, Str value_1, Str name_2, Str value_2, Str name_3, Str value_3);

/**
 * Function for immediate addition (ADDi)
 */
Str addimm(Str value, Str regname, Str regvalue);

/**
 * Function for unconditional jump (jump)
 */
Str jump(Str labelcount);
} // namespace detail
} // namespace microcode
} // namespace gen
} // namespace pass
} // namespace diamond
} // namespace arch
} // namespace ql
