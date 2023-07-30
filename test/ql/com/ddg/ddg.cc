#include "ql/com/ddg/build.h"
#include "ql/com/ddg/consistency.h"
#include "ql/com/ddg/dot.h"
#include "ql/com/ddg/ops.h"
#include "ql/ir/compat/compat.h"
#include "ql/ir/old_to_new.h"

#include <gtest/gtest.h>

using namespace ql;


TEST(ql_com_ddg, ddg) {
    auto plat = ir::compat::Platform::build("test_plat", utils::Str("cc_light"));
    auto program = utils::make<ir::compat::Program>("test_prog", plat, 7, 32, 10);

    auto kernel = utils::make<ir::compat::Kernel>("static_kernel", plat, 7, 32, 10);
    kernel->x(0);
    kernel->x(0);
    kernel->y(0);
    kernel->y(0);
    kernel->z(0);
    kernel->z(0);
    kernel->x(0);
    kernel->x(0);
    kernel->y(0);
    kernel->y(0);
    kernel->z(0);
    kernel->z(0);
    program->add(kernel);

    auto ir = ir::convert_old_to_new(program);

    com::ddg::build(ir, ir->program->blocks[0]);
    com::ddg::check_consistency(ir->program->blocks[0]);
    com::ddg::dump_dot(ir->program->blocks[0]);
    com::ddg::reverse(ir->program->blocks[0]);
    com::ddg::check_consistency(ir->program->blocks[0]);
    com::ddg::dump_dot(ir->program->blocks[0]);
}
