#include "ql/ir/old_to_new.h"

using namespace ql;

int main() {
    auto plat = ir::compat::Platform::build("test_plat", utils::Str("cc_light"));
    ir::compat::ProgramRef program;
    program.emplace("test_prog", plat, 7);
    ir::convert_old_to_new(program);
    return 0;
}
