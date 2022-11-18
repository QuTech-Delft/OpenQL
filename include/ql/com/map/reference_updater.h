#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace map {

class ReferenceUpdater : public ir::RecursiveVisitor {
public:
    using Callback = std::function<void(utils::UInt)>;

    ReferenceUpdater(ir::PlatformRef p, const utils::Vec<utils::UInt> &m,
        Callback c = {}) : platform(p), mapping(m), callback(c) {}

    void visit_node(ir::Node &node) override;

    void visit_reference(ir::Reference &ref) override;

private:
    ir::PlatformRef platform;
    const utils::Vec<utils::UInt> &mapping;
    Callback callback{};
};

}
}
}