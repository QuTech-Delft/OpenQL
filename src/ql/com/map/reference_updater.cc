#include "ql/com/map/reference_updater.h"

#include "ql/ir/ops.h"

namespace ql {
namespace com {
namespace map {

void ReferenceUpdater::visit_reference(ir::Reference &ref) {
    if (ref.target == platform->qubits && ref.data_type == platform->qubits->data_type) {
        QL_ASSERT(ref.indices.size() == 1);
        auto& virt = ref.indices[0].as<ir::IntLiteral>()->value;
        virt = mapping[virt];
        if (callback) {
            callback(virt);
        }
    }
}

void mapInstruction(const ir::PlatformRef &platform, const utils::Vec<utils::UInt> &mapping, const ir::CustomInstructionRef &instr, ReferenceUpdater::Callback callback) {
    QL_ASSERT(instr->instruction_type->generalization.empty() && "Instruction to map should be in the most generalized form, since it's using virtual qubit indices as operands");

    ReferenceUpdater visitor(platform, mapping, callback);
    instr->visit(visitor);

    specialize_instruction(instr);
}

void mapProgram(const ir::PlatformRef &platform, const utils::Vec<utils::UInt> &mapping, const ir::ProgramRef &program) {
    for (const auto& block: program->blocks) {
        for (const auto& st: block->statements) {
            auto cinsn = st.as<ir::CustomInstruction>();

            if (!cinsn.empty()) {
                mapInstruction(platform, mapping, cinsn);
            }
        }
    }
}


}
}
}
