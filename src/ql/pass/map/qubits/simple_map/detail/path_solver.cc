#include "path_solver.h"

#include "ql/utils/exception.h"

#include <algorithm>

namespace ql {   
namespace pass { 
namespace map {
namespace qubits {
namespace simple_map {
namespace detail {

void PathSolver::addRoutingPath(const QubitOccupation &occupation, const Path &path) {
    occupations.push_back(occupation);
    paths.push_back(path);
}

void Schedule::add(std::unordered_map<std::uint64_t, std::uint64_t> occupation) {
    // Assumes independent execution between elements of the map.
    // So assumes the operations in occupation are schedulable right away.

    for (auto kv: occupation) {
        at(kv.first) += kv.second;
    }
}

void Schedule::schedulePath(const Path &path) {
    std::uint64_t min = 0;
    for (std::uint64_t i = 0; i < path.size() - 1; ++i) {
        // Edge is from path[i] to path[i+1]

        // Assumes swap is rectangle (both qubits occupied the same time)
        auto q1 = path[i];
        auto q2 = path[i+1];

        auto maxq1q2 = std::max(at(q1), at(q2));

        min = std::max(min, maxq1q2) + swapDuration;

        at(q1) = min;
        at(q2) = min;
    }
}

std::uint64_t Schedule::getTotalDuration() {
    std::uint64_t max = 0;

    for (auto kv: schedule) {
        max = std::max(max, kv.second);
    }

    return max;
}

void Schedule::schedule2QGate(std::uint64_t op1, std::uint64_t op2, std::uint64_t duration = 1) {
    QL_ASSERT(op1 != op2);
    
    at(op1) = std::max(at(op1), at(op2)) + duration;
    at(op2) = at(op1);
}

// FIXME: use std::span, but it's C++20
std::pair<Path, Path> PathSolver::splitPath(Path p, std::uint64_t i) {
    QL_ASSERT(i < p.size() - 1); // i is the edge index

    Path first{p.begin(), p.begin() + i + 1};
    Path second{p.begin() + i + 1, p.end()};
    std::reverse(second.begin(), second.end());

    return std::make_pair(first, second);
}

// FIXME: inline vector, no allocation
std::uint64_t PathSolver::computeTotalDuration(std::vector<std::uint64_t> splitIndices) {
    QL_ASSERT(splitIndices.size() == paths.size());

    Schedule schedule(swapDuration);

    for (std::uint64_t i = 0; i < paths.size(); ++i) {
        schedule.add(occupations[0]);

        auto twoPaths = splitPath(paths[i], splitIndices[i]);

        schedule.schedulePath(twoPaths.first);
        schedule.schedulePath(twoPaths.second);
        schedule.schedule2QGate(twoPaths.first.back(), twoPaths.second.back());
    }

    return schedule.getTotalDuration();
};

std::vector<PathSolver::SplitPath> PathSolver::compute() {
    // Brute-force implementation.

    std::uint64_t minTotalDuration = -1; // FIXME, numeric_limits
    std::vector<std::uint64_t> argMinTotalDuration;

    std::vector<std::uint64_t> splitIndices(paths.size(), 0);

    std::uint64_t incrIndex = 0;

    while(true) {
        while(incrIndex < splitIndices.size() && (paths[incrIndex].size() < 2 || splitIndices[incrIndex] >= paths[incrIndex].size() - 2)) {
            splitIndices[incrIndex] = 0;
            ++incrIndex;
        }

        if (incrIndex >= splitIndices.size()) {
            break;
        }

        splitIndices[incrIndex] += 1;
        incrIndex = 0;

        auto currentDuration = computeTotalDuration(splitIndices);
        if (minTotalDuration < 0 || currentDuration < minTotalDuration) {
            minTotalDuration = currentDuration;
            argMinTotalDuration = splitIndices;
        };
    }

    std::vector<PathSolver::SplitPath> result;
    for (std::size_t i = 0; i < paths.size(); ++i) {
        auto splitIndex = argMinTotalDuration[i];
        result.push_back(splitPath(paths[i], splitIndex));
    }

    return result;
}

}
}
}
}
}
}