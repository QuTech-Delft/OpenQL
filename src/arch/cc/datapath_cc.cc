/**
 * @file    arch/cc/datapath_cc.cc
 * @date    20201119
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handling of Central Controller datapath (input MUX, Distributed Shared Memory, output PL)
 * @note
 */

#include <iomanip>

#include "datapath_cc.h"

namespace ql {
namespace arch {
namespace cc {

using namespace utils;

// math helpers
static unsigned int roundUp(unsigned int val, unsigned int mult) { return (val+mult-1)/mult*mult; }
static unsigned int alignSm(unsigned int bitAddr, unsigned int bits) { return bitAddr/bits*(bits/8); }

void Datapath::programStart() {
    datapathSection << std::left;    // assumed by emit()
    emit(".DATAPATH", "");
}

void Datapath::programFinish() {
    emit(".END", "");
}

UInt Datapath::allocateSmBit(UInt breg_operand, UInt instrIdx) {
    // Some requirements from hardware:
    // - different instruments must use SM bits located in different DSM transfers
    // - the current maximum required DSM transfer size is 16 bit (using a ZI UHFQA). The
    //   hardware maximum is 32 bit (and may be utilized by e.g. the ZI SHF)
    // - all DSM bits used for the conditional gates of a single bundle must reside in
    //   a 128 bit window, aligned on 128 bit (16 byte)
    // - DSM size is 1024 bits (128 bytes)
    // Other notes:
    // - we don't attempt to be smart about DSM transfer size allocation
    // - new allocations to the same breg_operand overwrite the old mapping
    // - we don't reuse SM bits (thus wasting space)

    UInt smBit = 0;
    if (!mapBregToSmBit.empty()) {    // not first alloc
        // perform allocation
        if (instrIdx != smBitLastInstrIdx) {
            smBit = roundUp(lastSmBit + 1, MAX_DSM_XFER_SIZE);
        } else {
            smBit = lastSmBit + 1;
        }
        if (smBit >= SM_BIT_CNT) {
            QL_FATAL("Exceeded available Shared memory space of " << SM_BIT_CNT << " bits");
        }

        auto it = mapBregToSmBit.find(breg_operand);
        if (it != mapBregToSmBit.end()) {
            QL_IOUT("Overwriting mapping of breg_operand " << it->second);
        }
    }

    QL_IOUT("Mapping breg_operand " << breg_operand << " to smBit " << smBit);
    mapBregToSmBit.set(breg_operand) = smBit;    // created on demand

    smBitLastInstrIdx = instrIdx;
    lastSmBit = smBit;

    return smBit;
}

// NB: bit_operand can be breg_operand or cond_operand, depending on context of caller
UInt Datapath::getSmBit(UInt bit_operand, UInt instrIdx) {
    UInt smBit;

    auto it = mapBregToSmBit.find(bit_operand);
    if (it != mapBregToSmBit.end()) {
        smBit = it->second;
        QL_DOUT("Found mapping: bit_operand " << bit_operand << " to smBit " << smBit);
    } else {
        QL_FATAL("Request for DSM bit of bit_operand " << bit_operand << " that was never assigned by measurement");        // NB: message refers to user perspective (and thus calling semantics)
    }
    return smBit;
}

UInt Datapath::getOrAssignMux(UInt instrIdx, const FeedbackMap &feedbackMap) {
    // We need a different MUX for every new combination of simultaneous readouts (per instrument)
    UInt mux = lastMux[instrIdx]++;    // FIXME: no reuse of identical combinations yet
    if (mux == MUX_CNT) {
        QL_FATAL("Maximum number of available CC datapath MUXes exceeded");
    }

    return mux;
}


UInt Datapath::getOrAssignPl(UInt instrIdx, const CondGateMap &condGateMap) {
    // We need a different PL for every new combination of simultaneous gate conditions (per instrument)
    UInt pl = lastPl[instrIdx]++;    // FIXME: no reuse of identical combinations yet
    if (pl == PL_CNT) {
        QL_FATAL("Maximum number of available CC datapath PLs exceeded");
    }

    return pl;
}


UInt Datapath::getSizeTag(UInt numReadouts) {
    UInt sizeTag;

    if (numReadouts == 0) {
        QL_FATAL("inconsistency in number of readouts");    // FIXME: message refers to caller, this assumes particular semantics for calling this function
    } else if (numReadouts <= 8) {
        sizeTag = 0;            // 0=byte
    } else if (numReadouts <= 16) {
        sizeTag = 1;
    } else if (numReadouts <= 32) {    // NB: should currently not occur since we have a maximum of 16 inputs on UHFQA
        sizeTag = 2;
    } else {
        QL_FATAL("inconsistency detected: too many readouts");
    }
    return sizeTag;
}


static Str cond_qasm(cond_type_t condition, const Vec<UInt> &cond_operands) {
    // FIXME: hack
    custom_gate g("foo");
    g.condition = condition;
    g.cond_operands = cond_operands;
    return g.cond_qasm();
}


void Datapath::emitMux(Int mux, const FeedbackMap &feedbackMap, UInt instrIdx, Int slot) {
    if (feedbackMap.empty()) {
        QL_FATAL("feedbackMap must not be empty");
    }

    emit(selString(slot) + QL_SS2S(".MUX " << mux), "");    // NB: no white space before ".MUX"

    for (const auto &feedback : feedbackMap) {
        FeedbackInfo fi = feedback.second;

        unsigned int winBit = fi.smBit % MUX_SM_WIN_SIZE;

        emit(
            slot,
            QL_SS2S("SM[" << winBit << "] := I[" << fi.bit << "]"),
            QL_SS2S("# cop " /*FIXME << fi.bi->creg_operands[0]*/ << " = readout(q" << fi.bi->operands[0] << ")")
        );
    }
}


UInt Datapath::getMuxSmAddr(const FeedbackMap &feedbackMap) {
    UInt minSmBit = MAX;
    UInt maxSmBit = 0;

    if (feedbackMap.empty()) {
        QL_FATAL("feedbackMap must not be empty");
    }

    for (auto &feedback : feedbackMap) {
        FeedbackInfo fi = feedback.second;

        minSmBit = min(minSmBit, fi.smBit);
        maxSmBit = max(maxSmBit, fi.smBit);
    }

    // perform checks
    if (alignSm(minSmBit, MUX_SM_WIN_SIZE) != alignSm(maxSmBit, MUX_SM_WIN_SIZE)) {
        QL_FATAL("Cannot access DSM bits " << minSmBit << " and " << maxSmBit << " in single MUX configuration");
    }
    return alignSm(minSmBit, MUX_SM_WIN_SIZE);
}


// FIXME: split like emitMux/getMuxSmAddr
UInt Datapath::emitPl(UInt pl, const CondGateMap &condGateMap, UInt instrIdx, Int slot) {
    Bool minMaxValid = false;    // we might not access SM
    UInt minSmBit = MAX;
    UInt maxSmBit = 0;

    if (condGateMap.empty()) {
        QL_FATAL("condGateMap must not be empty");
    }

    emit(selString(slot) + QL_SS2S(".PL " << pl), "");    // NB: no white space before ".PL"

    for (auto &cg : condGateMap) {
        Int group = cg.first;
        CondGateInfo cgi = cg.second;

        // emit comment for group
        Str condition = cond_qasm(cgi.condition, cgi.cond_operands);
        emit(
            slot,
            QL_SS2S("# group " << group << ", digOut=0x" << std::hex << std::setfill('0') << std::setw(8) << cgi.groupDigOut << ", condition='" << condition << "'")
        );

        // shorthand
        auto winBit = [this, cgi, instrIdx, &minMaxValid, &minSmBit, &maxSmBit](int i)
        {
            UInt smBit = getSmBit(cgi.cond_operands[i], instrIdx);
            minMaxValid = true;
            minSmBit = min(minSmBit, smBit);
            maxSmBit = max(maxSmBit, smBit);
            return smBit % PL_SM_WIN_SIZE;
        };

        // compute RHS of PL expression
        Str inv;
        StrStrm rhs;
        switch (cgi.condition) {
            // 0 operands:
            case cond_always:
                rhs << "1";
                break;
            case cond_never:
                rhs << "0";
                break;

            // 1 operand:
            case cond_not:
                inv = "/";
                // fall through
            case cond_unary:
                rhs << "SM[" << winBit(0) << "]";
                break;

            // 2 operands
            case cond_nand:
                inv = "/";
                // fall through
            case cond_and:
                rhs << "SM[" << winBit(0) << "] & SM[" << winBit(1) << "]";
                break;

            case cond_nor:
                inv = "/";
                // fall through
            case cond_or:
                rhs << "SM[" << winBit(0) << "] | SM[" << winBit(1) << "]";
                break;

            case cond_nxor:
                inv = "/";
                // fall through
            case cond_xor:
                rhs << "SM[" << winBit(0) << "] ^ SM[" << winBit(1) << "]";
                break;
        }

        // emit PL logic
        for (UInt bit = 0; bit < 32; bit++) {
            if ((1ul << bit) & cgi.groupDigOut) {
                emit(
                    slot,
                    QL_SS2S(inv << "O[" << bit << "] := " << rhs.str())
                );
            }
        }
    }

    // perform checks
    if (minMaxValid) {
        if (alignSm(minSmBit, PL_SM_WIN_SIZE) != alignSm(maxSmBit, PL_SM_WIN_SIZE)) {
            QL_FATAL("Cannot access DSM bits " << minSmBit << " and " << maxSmBit << " in single PL configuration");
        }
    }
    return alignSm(minSmBit, PL_SM_WIN_SIZE);    // NB: irrelevant if !minMaxValid since SM is not accessed in that case
}

} // namespace cc
} // namespace arch
} // namespace ql
