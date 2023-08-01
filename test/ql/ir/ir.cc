#include "ql/com/ddg/build.h"
#include "ql/com/ddg/dot.h"
#include "ql/com/ddg/ops.h"
#include "ql/com/ddg/types.h"
#include "ql/com/sch/scheduler.h"
#include "ql/ir/cqasm/read.h"
#include "ql/ir/cqasm/write.h"
#include "ql/ir/describe.h"
#include "ql/ir/ir.h"
#include "ql/ir/new_to_old.h"
#include "ql/ir/old_to_new.h"
#include "ql/ir/ops.h"
#include "ql/pass/io/cqasm/report.h"
#include "ql/rmgr/manager.h"
#include "ql/utils/filesystem.h"

#include <gtest/gtest.h>

using namespace ql;


TEST(ql_ir, ir) {
    auto plat = ir::compat::Platform::build("test_plat", utils::Str("cc_light"));
    auto program = utils::make<ir::compat::Program>("test_prog", plat, 7, 32, 10);

    auto kernel = utils::make<ir::compat::Kernel>("static_kernel", plat, 7, 32, 10);
    kernel->x(0);
    kernel->classical(ir::compat::ClassicalRegister(1), 0);
    kernel->classical(ir::compat::ClassicalRegister(2), 10);
    program->add(kernel);

    auto sub_program = utils::make<ir::compat::Program>("x", plat, 7, 32, 10);
    kernel = utils::make<ir::compat::Kernel>("inner_loop_kernel", plat, 7, 32, 10);
    kernel->y(0);
    sub_program->add_for(kernel, 10);

    kernel = utils::make<ir::compat::Kernel>("outer_loop_kernel", plat, 7, 32, 10);
    kernel->z(0);
    kernel->classical(ir::compat::ClassicalRegister(3), 1);
    kernel->classical(ir::compat::ClassicalRegister(1), ir::compat::ClassicalOperation(
        ir::compat::ClassicalRegister(1), "+", ir::compat::ClassicalRegister(3)
    ));
    sub_program->add(kernel);

    program->add_do_while(sub_program, ir::compat::ClassicalOperation(
        ir::compat::ClassicalRegister(1), "<", ir::compat::ClassicalRegister(2)
    ));

    kernel = utils::make<ir::compat::Kernel>("if_a", plat, 7, 32, 10);
    kernel->x(1);
    auto kernel2 = utils::make<ir::compat::Kernel>("else", plat, 7, 32, 10);
    kernel2->y(1);
    program->add_if_else(kernel, kernel2, ir::compat::ClassicalOperation(
        ir::compat::ClassicalRegister(1), "==", ir::compat::ClassicalRegister(2)
    ));

    kernel = utils::make<ir::compat::Kernel>("if_b", plat, 7, 32, 10);
    kernel->z(1);
    program->add_if(kernel, ir::compat::ClassicalOperation(
        ir::compat::ClassicalRegister(1), ">", ir::compat::ClassicalRegister(2)
    ));

    auto ir = ir::convert_old_to_new(program);
    //ir->dump_seq();

    ir->program->objects.emplace<ir::TemporaryObject>("", ir->platform->default_bit_type);
    ir->program->objects.emplace<ir::VariableObject>("hello", ir::add_type<ir::IntType>(ir, "int64", true, 64));

    utils::StrStrm ss;
    ir::cqasm::write(ir, {}, ss);
    ir->program.reset();
    ir::cqasm::read(ir, ss.str());
    ss << "\n*** after read/write ***\n\n";

    ir::cqasm::WriteOptions wo;
    wo.include_statistics = true;
    ir::cqasm::write(ir, wo, ss);
    //ss << "\n*** after conversion to old and back to new ***\n\n";
    //ir = ir::convert_old_to_new(ir::convert_new_to_old(ir));
    //ir::cqasm::write(ir, {}, ss);

    std::cout << ss.str() << std::endl;
}
