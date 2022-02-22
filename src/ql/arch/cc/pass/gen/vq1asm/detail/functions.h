#pragma once

#include "ql/ir/ir.h"

#include "types.h"


namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {


class Functions {
public:
    Functions();
    ~Functions();

    void register_();
    void dispatch(const Str &name, ir::Any<ir::Expression> operands);


private:

    enum Profile {
        LR,     // int Literal, Reference
        RL,
        RR
    };
    Profile get_profile(ir::Any<ir::Expression> operands);
    UInt emit_bin_cast(ir::Any<ir::Expression> operands, Int expOpCnt);


    // helpers to ease nice assembly formatting
    void emit(const Str &labelOrComment, const Str &instr="");
    void emit(const Str &label, const Str &instr, const Str &ops, const Str &comment="");
    void emit(Int slot, const Str &instr, const Str &ops, const Str &comment="");

    UInt dest_reg();
    void emit_bin_cast();
    void emit_mnem2args();




private:
    // map name to function
};


} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
