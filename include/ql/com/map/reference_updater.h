#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace map {

/*
This class is a visitor for changing virtual qubit indices to real qubit indices, given a mapping.
*/

class ReferenceUpdater : public ir::RecursiveVisitor {
public:
    using Callback = std::function<void(utils::UInt)>;

    /*
        m: a vector represening the virtual->real qubit mapping.
           m[i] is the real qubit index corresponding to virtual qubit i.
        
        c: a callback to be called for every mapped virtual qubit index.
    */
    ReferenceUpdater(ir::PlatformRef p, const utils::Vec<utils::UInt> &m,
        Callback c = {}) : platform(p), mapping(m), callback(c) {}

    void visit_node(ir::Node &node) override {};

    void visit_reference(ir::Reference &ref) override;

    // Gate operands may be virtual qubits, but in the instruction type it's always real qubit indices.
    void visit_instruction_type(ir::InstructionType &t) override {};

private:
    ir::PlatformRef platform;
    const utils::Vec<utils::UInt> &mapping;
    Callback callback{};
};

void mapInstruction(const ir::PlatformRef &platform, const utils::Vec<utils::UInt> &mapping, const ir::CustomInstructionRef &instr, ReferenceUpdater::Callback callback = {});

void mapProgram(const ir::PlatformRef &platform, const utils::Vec<utils::UInt> &mapping, const ir::ProgramRef &program);

}
}
}