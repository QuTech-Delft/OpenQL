#include "ql/utils/num.h"

#include <unordered_map>
#include <vector>
#include <set>
#include <vector>

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace simple_map {
namespace detail {

/*
"Tetris solver"
Given a set of routing paths (extremities being the operands of a 2q gate) and qubit unavailability (duration),
computes where to split the path for optimal scheduling.
*/


// A path is a vector of unique real qubit indices.
// Early optimization is the root of all evil but might be better as an inline fixed-size vector. Allocation is costly!
using Path = std::vector<std::uint64_t>;

class Schedule {
public:
    Schedule(std::unordered_map<std::uint64_t, std::uint64_t>) {

    }

    Schedule(std::uint64_t aSwapDuration) : swapDuration(aSwapDuration) {};

    void add(std::unordered_map<std::uint64_t, std::uint64_t>);

    void schedulePath(const Path &path);

    std::uint64_t getTotalDuration();

    void schedule2QGate(std::uint64_t op1, std::uint64_t op2, std::uint64_t duration = 1);

private:
    std::uint64_t& at(std::uint64_t q) {
        return schedule[q]; // default value-initialization with 0
    }

    std::unordered_map<std::uint64_t, std::uint64_t> schedule;
    std::uint64_t swapDuration = 0;
};

class PathSolver {
public:

    // A split path where the end of the first path is nearest-neighbor with the end of the second.
    using SplitPath = std::pair<Path, Path>;

    // A "qubit occupation" is a map from real qubit index to duration of unavailability (e.g. running other gates).
    // This only works for 1q gates! So they don't have inter-dependencies. No resource.
    using QubitOccupation = std::unordered_map<std::uint64_t, std::uint64_t>;

    static std::pair<Path, Path> splitPath(Path p, std::uint64_t i);

    PathSolver(std::uint64_t aSwapDuration) : swapDuration(aSwapDuration) {};

    // Multiple routing paths can be added for lookahead.
    void addRoutingPath(const QubitOccupation &occupation, const Path &path);

    std::uint64_t computeTotalDuration(std::vector<std::uint64_t> splitIndices);

    std::vector<SplitPath> compute();

private:
    std::uint64_t swapDuration = 0;
    std::vector<Path> paths;
    std::vector<QubitOccupation> occupations;
};

}
}
}
}
}
}