/** \file
 * Virt2Real: map of a virtual qubit index to its real qubit index.
 *
 * See header file for more information.
 */

#include "ql/com/virt2real.h"

#include "ql/utils/logger.h"
#include "ql/com/options.h"

namespace ql {
namespace com {
namespace virt2real {

using namespace utils;

// map real qubit to the virtual qubit index that is mapped to it (i.e. backward map);
// when none, return UNDEFINED_QUBIT;
// a second vector next to v2rMap (i.e. an r2vMap) would speed this up;
UInt Virt2Real::GetVirt(UInt r) const {
    QL_ASSERT(r != UNDEFINED_QUBIT);
    for (UInt v = 0; v < nq; v++) {
        if (v2rMap[v] == r) {
            return v;
        }
    }
    return UNDEFINED_QUBIT;
}

realstate_t Virt2Real::GetRs(UInt q) const {
    return rs[q];
}

void Virt2Real::SetRs(UInt q, realstate_t rsvalue) {
    rs[q] = rsvalue;
}

// expand to desired size
//
// mapping starts off undefined for all virtual qubits
// (unless option mapinitone2one is set, then virtual qubit i maps to real qubit i for all qubits)
//
// real qubits are assumed to have a garbage state
// (unless option mapassumezeroinitstate was set,
//  then all real qubits are assumed to have a state suitable for replacing swap by move)
//
// the rs initializations are done only once, for a whole program
void Virt2Real::Init(UInt n) {
    auto mapinitone2oneopt = com::options::get("mapinitone2one");
    auto mapassumezeroinitstateopt = com::options::get("mapassumezeroinitstate");

    QL_DOUT("Virt2Real::Init: mapinitone2oneopt=" << mapinitone2oneopt);
    QL_DOUT("Virt2Real::Init: mapassumezeroinitstateopt=" << mapassumezeroinitstateopt);

    nq = n;
    if (mapinitone2oneopt == "yes") {
        QL_DOUT("Virt2Real::Init(n=" << nq << "), initializing 1-1 mapping");
    } else {
        QL_DOUT("Virt2Real::Init(n=" << nq << "), initializing on demand mapping");
    }
    if (mapassumezeroinitstateopt == "yes") {
        QL_DOUT("Virt2Real::Init(n=" << nq << "), assume all qubits in initialized state");
    } else {
        QL_DOUT("Virt2Real::Init(n=" << nq << "), assume all qubits in garbage state");
    }
    v2rMap.resize(nq);
    rs.resize(nq);
    for (UInt i = 0; i < nq; i++) {
        if (mapinitone2oneopt == "yes") {
            v2rMap[i] = i;
        } else {
            v2rMap[i] = UNDEFINED_QUBIT;
        }
        if (mapassumezeroinitstateopt == "yes") {
            rs[i] = rs_wasinited;
        } else {
            rs[i] = rs_nostate;
        }
    }
}

// map virtual qubit index to real qubit index
UInt &Virt2Real::operator[](UInt v) {
    QL_ASSERT(v < nq);   // implies v != UNDEFINED_QUBIT
    return v2rMap[v];
}

const UInt &Virt2Real::operator[](UInt v) const {
    QL_ASSERT(v < nq);   // implies v != UNDEFINED_QUBIT
    return v2rMap[v];
}

// allocate a new real qubit for an unmapped virtual qubit v (i.e. v2rMap[v] == UNDEFINED_QUBIT);
// note that this may consult the grid or future gates to find a best real
// and thus should not be in Virt2Real but higher up
UInt Virt2Real::AllocQubit(UInt v) {
    // check all real indices for being in v2rMap
    // first one that isn't, is free and is returned
    for (UInt r = 0; r < nq; r++) {
        UInt vt;
        for (vt = 0; vt < nq; vt++) {
            if (v2rMap[vt] == r) {
                break;
            }
        }
        if (vt >= nq) {
            // real qubit r was not found in v2rMap
            // use it to map v
            v2rMap[v] = r;
            QL_ASSERT(rs[r] == rs_wasinited || rs[r] == rs_nostate);
            QL_DOUT("AllocQubit(v=" << v << ") in r=" << r);
            return r;
        }
    }
    QL_ASSERT(0);    // number of virt qubits <= number of real qubits
    return UNDEFINED_QUBIT;
}

// r0 and r1 are real qubit indices;
// by execution of a swap(r0,r1), their states are exchanged at runtime;
// so when v0 was in r0 and v1 was in r1, then v0 is now in r1 and v1 is in r0;
// update v2r accordingly
void Virt2Real::Swap(UInt r0, UInt r1) {
    QL_ASSERT(r0 != r1);
    UInt v0 = GetVirt(r0);
    UInt v1 = GetVirt(r1);
    // QL_DOUT("... swap between ("<< v0<<"<->"<<r0<<","<<v1<<"<->"<<r1<<") and ("<<v0<<"<->"<<r1<<","<<v1<<"<->"<<r0<<" )");
    // DPRINT("... before swap");
    QL_ASSERT(v0 != v1);         // also holds when vi == UNDEFINED_QUBIT

    if (v0 == UNDEFINED_QUBIT) {
        QL_ASSERT(rs[r0] != rs_hasstate);
    } else {
        QL_ASSERT(v0 < nq);
        v2rMap[v0] = r1;
    }

    if (v1 == UNDEFINED_QUBIT) {
        QL_ASSERT(rs[r1] != rs_hasstate);
    } else {
        QL_ASSERT(v1 < nq);
        v2rMap[v1] = r0;
    }

    realstate_t ts = rs[r0];
    rs[r0] = rs[r1];
    rs[r1] = ts;
    // DPRINT("... after swap");
}

void Virt2Real::DPRINTReal(UInt r) const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        PrintReal(r);
    }
}

void Virt2Real::PrintReal(UInt r) const {
    std::cout << " (r" << r;
    switch (rs[r]) {
        case rs_nostate:
            std::cout << ":no";
            break;
        case rs_wasinited:
            std::cout << ":in";
            break;
        case rs_hasstate:
            std::cout << ":st";
            break;
        default:
        QL_ASSERT(0);
    }
    UInt v = GetVirt(r);
    if (v == UNDEFINED_QUBIT) {
        std::cout << "<-UN)";
    } else {
        std::cout << "<-v" << v << ")";
    }
}

void Virt2Real::PrintVirt(UInt v) const {
    std::cout << " (v" << v;
    UInt r = v2rMap[v];
    if (r == UNDEFINED_QUBIT) {
        std::cout << "->UN)";
    } else {
        std::cout << "->r" << r;
        switch (rs[r]) {
            case rs_nostate:
                std::cout << ":no)";
                break;
            case rs_wasinited:
                std::cout << ":in)";
                break;
            case rs_hasstate:
                std::cout << ":st)";
                break;
            default:
            QL_ASSERT(0);
        }
    }
}

void Virt2Real::DPRINTReal(const Str &s, UInt r0, UInt r1) const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        PrintReal(s, r0, r1);
    }
}

void Virt2Real::PrintReal(const Str &s, UInt r0, UInt r1) const {
    // QL_DOUT("v2r.PrintReal ...");
    std::cout << s << ":";
//  std::cout << "... real2Virt(r<-v) " << s << ":";

    PrintReal(r0);
    PrintReal(r1);
    std::cout << std::endl;
}

void Virt2Real::DPRINT(const Str &s) const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        Print(s);
    }
}

void Virt2Real::Print(const Str &s) const {
    // QL_DOUT("v2r.Print ...");
    std::cout << s << ":";
//  std::cout << "... virt2Real(r<-v) " << s << ":";
    for (UInt v = 0; v < nq; v++) {
        PrintVirt(v);
    }
    std::cout << std::endl;

    std::cout << "... real2virt(r->v) " << s << ":";
    for (UInt r = 0; r < nq; r++) {
        PrintReal(r);
    }
    std::cout << std::endl;
}

void Virt2Real::Export(Vec<UInt> &kv2rMap) const {
    kv2rMap = v2rMap;
}

void Virt2Real::Export(Vec<Int> &krs) const {
    krs.resize(rs.size());
    for (UInt i = 0; i < rs.size(); i++) {
        krs[i] = (Int)rs[i];
    }
}

} // namespace virt2real
} // namespace com
} // namespace ql
