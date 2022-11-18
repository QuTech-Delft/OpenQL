#include "impl.h"

#include "ql/com/topology.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace simple_map {
namespace detail {

Impl::ShortestPaths Impl::computeShortestPaths(
    UInt numQubits,
    std::function<utils::List<UInt>(UInt)> getNeighbors
) {
    Impl::ShortestPaths result;

    std::set<UInt> visited{};

    QL_ASSERT(numQubits >= 1);
    std::set<UInt> to_visit = {0};

    while(visited.size() < numQubits) {
        QL_ASSERT(!to_visit.empty());
        auto q = *to_visit.begin();
        to_visit.erase(to_visit.begin());

        auto neighbors = getNeighbors(q);
        for (auto n: neighbors) {
            if (visited.count(n) >= 1) {
                for (auto v: visited) {
                    const auto& paths_from_q_to_v = result[std::make_pair(q, v)];

                    for (auto p: paths_from_q_to_v) {
                        p.push_front(q);
                        result[std::make_pair(q, v)].push_back(p);
                    }
                }
                result[std::make_pair(q, n)].push_back({q, n});

                continue;
            }

            for (auto v: visited) {
                const auto& paths_from_v_to_q = result[std::make_pair(v, q)];

                for (auto p: paths_from_v_to_q) {
                    p.push_back(n);
                    result[std::make_pair(v, n)].push_back(p);
                }
            }

            result[std::make_pair(q, n)].push_back({q, n});

            to_visit.insert(n);
        }

        visited.insert(q);
    }

    return result;
}

void simpleMap(const ir::Ref &ir) {

};

}
}
}
}
}
}