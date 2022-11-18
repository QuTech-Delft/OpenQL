#include "ql/com/map/reference_updater.h"

namespace ql {
namespace com {
namespace map {

void ReferenceUpdater::visit_node(ir::Node &node) {}

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

}
}
}
