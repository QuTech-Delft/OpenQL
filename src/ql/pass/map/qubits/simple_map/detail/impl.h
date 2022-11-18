#include "ql/utils/num.h"

#include "ql/ir/ir.h"
#include "ql/utils/pairhash.h"
#include <unordered_map>
#include <vector>
#include <set>

namespace ql {
namespace com{
    class Topology;
}

namespace pass {
namespace map {
namespace qubits {
namespace simple_map {
namespace detail {

using UInt = utils::UInt;

class Impl {
public:
    using ShortestPaths = std::unordered_map<std::pair<UInt, UInt>, std::vector<utils::List<UInt>>, utils::PairHash>;

    static ShortestPaths computeShortestPaths(
        UInt numQubits,
        std::function<utils::List<UInt>(UInt)> getNeighbors
    );
};

void simpleMap(const ir::Ref &ir);

}
}
}
}
}
}