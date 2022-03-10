/**
 * @file    arch/cc/pass/gen/vq1asm/detail/datapath.h
 * @date    20201119
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handling of Central Controller datapath (input MUX, Distributed Shared Memory, output PL)
 * @note
 */

#pragma once

#include <iomanip>

#include "types.h"
#include "bundle_info.h"

#include "ql/utils/logger.h"
#include "ql/ir/compat/compat.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

// NB: types shared with codegen_cc. FIXME: move
struct MeasResultRealTimeInfo {                             // information for RT measurement results on single instrument group
    UInt smBit;
    UInt bit;

    // original instruction, copied from BundleInfo
    Str describe;
};

using MeasResultRealTimeMap = Map<Int, MeasResultRealTimeInfo>;   // NB: key is instrument group

struct CondGateInfo {   // information for conditional gate on single instrument group
    tInstructionCondition instructionCondition;
    tDigital groupDigOut;
};

using CondGateMap = Map<Int, CondGateInfo>;                 // NB: key is instrument group



class Datapath {
public: // types

public: // functions
    Datapath() = default;
    ~Datapath() = default;

    void programStart();
    void programFinish();

    UInt allocateSmBit(UInt breg_operand, UInt instrIdx);
    UInt getSmBit(UInt bit_operand) const;
    UInt getOrAssignMux(UInt instrIdx, const MeasResultRealTimeMap &measResultRealTimeMap);
    UInt getOrAssignPl(UInt instrIdx, const CondGateMap &condGateMap);
    static UInt getSizeTag(UInt numReadouts);
    void emitMux(Int mux, const MeasResultRealTimeMap &measResultRealTimeMap, Int slot);
    static UInt getMuxSmAddr(const MeasResultRealTimeMap &measResultRealTimeMap);
    UInt emitPl(UInt pl, const CondGateMap &condGateMap, UInt instrIdx, Int slot);

    Str getDatapathSection() const { return datapathSection.str(); }

    void comment(const Str &cmnt, Bool verboseCode) {
        if (verboseCode) datapathSection << cmnt << std::endl;
    }

private:    // functions
    Str selString(Int sel) { return QL_SS2S("[" << sel << "]"); }

    void emit(const Str &sel, const Str &statement, const Str &comment="") {
        datapathSection << std::setw(16) << sel << std::setw(16) << statement << std::setw(24) << comment << std::endl;
    }
    void emit(Int sel, const Str &statement, const Str &comment="") {
        emit(selString(sel), statement, comment);
    }

private:    // vars
    static const UInt MUX_CNT = 512;                            // number of MUX configurations
    static const UInt MUX_SM_WIN_SIZE = 16;                     // number of MUX bits in single view (currently, using a ZI UHFQA)
    static const UInt PL_CNT = 512;                             // number of PL configurations
    static const UInt PL_SM_WIN_SIZE = 128;                     // number of SM bits in single view
    static const UInt SM_BIT_CNT = 1024;                        // number of SM bits
    static const UInt MAX_DSM_XFER_SIZE = 16;                   // current max (using a ZI UHFQA)

    StrStrm datapathSection;                                    // the data path configuration generated

    // state for allocateSmBit/getSmBit
    UInt lastSmBit = 0;
    UInt smBitLastInstrIdx = 0;
    Map<UInt, UInt> mapBregToSmBit;

    // other state
    Vec<UInt> lastMux = Vec<UInt>(MAX_INSTRS, 0);
    Vec<UInt> lastPl = Vec<UInt>(MAX_INSTRS, 0);
}; // class

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
