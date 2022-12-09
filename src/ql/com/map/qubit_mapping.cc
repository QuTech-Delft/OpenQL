/** \file
 * Virtual to real qubit mapping state tracker.
 */

#include "ql/com/map/qubit_mapping.h"

#include "ql/utils/logger.h"

namespace ql {
namespace com {
namespace map {

using namespace utils;

/**
 * Converts QubitState to a string.
 */
std::ostream &operator<<(std::ostream &os, QubitState qs) {
    switch (qs) {
        case QubitState::NONE:          os << "none";           break;
        case QubitState::INITIALIZED:   os << "initialized";    break;
        case QubitState::LIVE:          os << "live";           break;
    }
    return os;
}

/**
 * Creates a virtual to real qubit map with the given number of qubits.
 *
 * The mapping starts off undefined for all virtual qubits, unless
 * one_to_one is set, in which case virtual qubit i maps to real qubit i for
 * all qubits. The state of the qubits is initialized as specified.
 */
QubitMapping::QubitMapping(
    utils::UInt num_qubits,
    utils::Bool one_to_one,
    QubitState initial_state
) {
    resize(num_qubits, one_to_one, initial_state);
}

/**
 * Resizes/reinitializes the map.
 *
 * Newly added qubits start off with an undefined mapping, unless one_to_one
 * is set, in which case virtual qubit i maps to real qubit i for all
 * qubits. The state of the new qubits is initialized as specified.
 */
void QubitMapping::resize(
    utils::UInt num_qubits,
    utils::Bool one_to_one,
    QubitState initial_state
) {
    virt_to_real.resize(num_qubits);
    real_state.resize(num_qubits);
    for (UInt i = nq; i < num_qubits; i++) {
        if (one_to_one) {
            virt_to_real[i] = i;
        } else {
            virt_to_real[i] = UNDEFINED_QUBIT;
        }
        real_state[i] = initial_state;
    }
    nq = num_qubits;
}

/**
 * Map virtual qubit index to real qubit index.
 */
UInt &QubitMapping::operator[](UInt v) {
    QL_ASSERT(v < nq);   // implies v != UNDEFINED_QUBIT
    return virt_to_real[v];
}

/**
 * Map virtual qubit index to real qubit index.
 */
const UInt &QubitMapping::operator[](UInt v) const {
    QL_ASSERT(v < nq);   // implies v != UNDEFINED_QUBIT
    return virt_to_real[v];
}

/**
 * Returns the underlying virtual to real qubit vector.
 */
const utils::Vec<utils::UInt> &QubitMapping::get_virt_to_real() const {
    return virt_to_real;
}

/**
 * Map real qubit to the virtual qubit index that is mapped to it (i.e.
 * backward map). When none, return UNDEFINED_QUBIT. This currently loops
 * over all qubits, so it isn't particularly fast.
 */
UInt QubitMapping::get_virtual(UInt real) const {
    QL_ASSERT(real != UNDEFINED_QUBIT);
    for (UInt virt = 0; virt < nq; virt++) {
        if (virt_to_real[virt] == real) {
            return virt;
        }
    }
    return UNDEFINED_QUBIT;
}

/**
 * Returns the current state for the given real qubit.
 */
QubitState QubitMapping::get_state(UInt real) const {
    return real_state[real];
}

/**
 * Sets the state for the given real qubit.
 */
void QubitMapping::set_state(UInt real, QubitState state) {
    real_state[real] = state;
}

/**
 * Returns the underlying qubit state vector.
 */
const utils::Vec<QubitState> &QubitMapping::get_state() const {
    return real_state;
}

/**
 * Allocate a real qubit for the given unmapped virtual qubit.
 */
UInt QubitMapping::allocate(UInt virt) {
    QL_ASSERT(virt_to_real[virt] == UNDEFINED_QUBIT);
    // check all real indices for being in v2rMap
    // first one that isn't, is free and is returned
    for (UInt real = 0; real < nq; real++) {
        if (get_virtual(real) == UNDEFINED_QUBIT) {
            // real qubit r was not found in v2rMap
            // use it to map v
            virt_to_real[virt] = real;
            QL_ASSERT(real_state[real] == QubitState::INITIALIZED || real_state[real] == QubitState::NONE);
            QL_DOUT("allocate(v=" << virt << ") in r=" << real);
            return real;
        }
    }
    QL_ASSERT(0);    // number of virt qubits <= number of real qubits
    return UNDEFINED_QUBIT;
}

/**
 * Updates the mapping to reflect a swap for the given real qubit indices,
 * so when v0 was in r0 and v1 was in r1, then v0 is now in r1 and v1 is now
 * in r0.
 */
void QubitMapping::swap(UInt r0, UInt r1) {
    QL_ASSERT(r0 != r1);
    UInt v0 = get_virtual(r0);
    UInt v1 = get_virtual(r1);
    // QL_DOUT("... swap between ("<< v0<<"<->"<<r0<<","<<v1<<"<->"<<r1<<") and ("<<v0<<"<->"<<r1<<","<<v1<<"<->"<<r0<<" )");
    // DPRINT("... before swap");
    QL_ASSERT(v0 != v1);         // also holds when vi == UNDEFINED_QUBIT

    if (v0 == UNDEFINED_QUBIT) {
        QL_ASSERT(real_state[r0] != QubitState::LIVE);
    } else {
        QL_ASSERT(v0 < nq);
        virt_to_real[v0] = r1;
    }

    if (v1 == UNDEFINED_QUBIT) {
        QL_ASSERT(real_state[r1] != QubitState::LIVE);
    } else {
        QL_ASSERT(v1 < nq);
        virt_to_real[v1] = r0;
    }

    QubitState ts = real_state[r0];
    real_state[r0] = real_state[r1];
    real_state[r1] = ts;
    // DPRINT("... after swap");
}

/**
 * Returns a string representation of the state of the given real qubit.
 */
utils::Str QubitMapping::real_to_string(utils::UInt real) const {
    StrStrm ss;
    ss << " (r" << real;
    switch (real_state[real]) {
        case QubitState::NONE:          ss << ":no"; break;
        case QubitState::INITIALIZED:   ss << ":in"; break;
        case QubitState::LIVE:          ss << ":st"; break;
    }
    UInt v = get_virtual(real);
    if (v == UNDEFINED_QUBIT) {
        ss << "<-UN)";
    } else {
        ss << "<-v" << v << ")";
    }
    return ss.str();
}

/**
 * Returns a string representation of the state of the given virtual qubit.
 */
utils::Str QubitMapping::virtual_to_string(utils::UInt virt) const {
    StrStrm ss;
    ss << " (v" << virt;
    UInt real = virt_to_real[virt];
    if (real == UNDEFINED_QUBIT) {
        ss << "->UN)";
    } else {
        ss << "->r" << real;
        switch (real_state[real]) {
            case QubitState::NONE:          ss << ":no)"; break;
            case QubitState::INITIALIZED:   ss << ":in)"; break;
            case QubitState::LIVE:          ss << ":st)"; break;
        }
    }
    return ss.str();
}

/**
 * Returns a string representation of the virtual to physical qubit mapping.
 */
Str QubitMapping::mapping_to_string() const {
    StrStrm ss;
    Bool any = false;
    for (UInt virt = 0; virt < nq; virt++) {
        auto real = virt_to_real[virt];
        if (real != UNDEFINED_QUBIT) {
            if (!any) {
                any = true;
            } else {
                ss << ", ";
            }
            ss << virt << " => " << real;
        }
    }
    if (!any) {
        return "empty";
    } else {
        return ss.str();
    }
}

/**
 * Dumps the state of this mapping to the given stream.
 */
void QubitMapping::dump_state(std::ostream &os, const utils::Str &line_prefix) const {
    os << line_prefix << "virtual qubits:\n";
    for (UInt virt = 0; virt < nq; virt++) {
        os << line_prefix << "  " << virtual_to_string(virt) << "\n";
    }
    os << line_prefix << "\n";
    os << line_prefix << "real qubits:\n";
    for (UInt real = 0; real < nq; real++) {
        os << line_prefix << "  " << real_to_string(real) << "\n";
    }
}

} // namespace map
} // namespace com
} // namespace ql
