#include "options.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

std::ostream &operator<<(std::ostream &os, Heuristic h) {
    switch (h) {
        case Heuristic::BASE:               os << "base";          break;
        case Heuristic::MIN_EXTEND:         os << "min_extend";    break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, LookaheadMode lm) {
    switch (lm) {
        case LookaheadMode::DISABLED:             os << "disabled";             break;
        case LookaheadMode::ONE_QUBIT_GATE_FIRST: os << "one_qubit_gate_first"; break;
        case LookaheadMode::NO_ROUTING_FIRST:     os << "no_routing_first";     break;
        case LookaheadMode::ALL:                  os << "all";                  break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, PathSelectionMode psm) {
    switch (psm) {
        case PathSelectionMode::ALL:     os << "all";     break;
        case PathSelectionMode::BORDERS: os << "borders"; break;
        case PathSelectionMode::RANDOM:  os << "random";  break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, SwapSelectionMode ssm) {
    switch (ssm) {
        case SwapSelectionMode::ONE:      os << "one";      break;
        case SwapSelectionMode::ALL:      os << "all";      break;
        case SwapSelectionMode::EARLIEST: os << "earliest"; break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, TieBreakMethod tbm) {
    switch (tbm) {
        case TieBreakMethod::FIRST:    os << "first";    break;
        case TieBreakMethod::LAST:     os << "last";     break;
        case TieBreakMethod::RANDOM:   os << "random";   break;
        case TieBreakMethod::CRITICAL: os << "critical"; break;
    }
    return os;
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
