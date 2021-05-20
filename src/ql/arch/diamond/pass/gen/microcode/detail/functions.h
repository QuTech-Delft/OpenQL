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


} // namespace detail
} // namespace microcode
} // namespace gen
} // namespace pass
} // namespace diamond
} // namespace arch
} // namespace ql
