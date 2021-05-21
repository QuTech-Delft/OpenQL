#pragma once

namespace ql {
namespace arch {
namespace diamond {
namespace pass {
namespace gen {
namespace microcode {
namespace detail {

using namespace utils;

Str switchOn( unsigned long qubit_path );

Str switchOff( unsigned long qubit_path );

Str loadimm(Str value, Str reg_name, Str reg_value);

Str mov(Str reg1_name, Str reg1_value, Str reg2_name, Str reg2_value);

Str excite_mw(Str envelope, Str duration, Str frequency, Str phase, utils::Vec<utils::UInt> qubit);

Str branch(Str name_1, Str value_1, Str comparison, Str name_2, Str value_2, Str target_name, Str target_value);

Str label(Str labelcount);

Str qgate(Str gatename, utils::Vec<utils::UInt> operand);

Str qgate2(Str gatename, utils::Vec<utils::UInt> operand);

Str store(Str reg_name1, Str reg_value1, Str reg_name2, Str reg_value2, Str memaddr);

Str add(Str name_1, Str value_1, Str name_2, Str value_2, Str name_3, Str value_3);

Str addimm(Str value, Str regname, Str regvalue);
} // namespace detail
} // namespace microcode
} // namespace gen
} // namespace pass
} // namespace diamond
} // namespace arch
} // namespace ql
