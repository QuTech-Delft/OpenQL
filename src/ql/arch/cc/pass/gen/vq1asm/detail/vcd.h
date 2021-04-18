/**
 * @file    arch/cc/pass/gen/vq1asm/detail/vcd_cc.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle generation of Value Change Dump file for GTKWave viewer
 * @note
 */

#pragma once

#include "ql/utils/vcd.h"

#include "types.h"
#include "settings.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

class Vcd : private utils::Vcd {
public:     // funcs
    Vcd() = default;
    ~Vcd() = default;

    void programStart(UInt qubitNumber, Int cycleTime, Int maxGroups, const Settings &settings);
    void programFinish(const Str &progName);
    void kernelFinish(const Str &kernelName, UInt durationInCycles);
    void bundleFinishGroup(UInt startCycle, UInt durationInCycles, Digital groupDigOut, const Str &signalValue, UInt instrIdx, Int group);
    void bundleFinish(UInt startCycle, Digital digOut, UInt maxDurationInCycles, UInt instrIdx);
    void customGate(const Str &iname, const Vec<UInt> &qops, UInt startCycle, UInt durationInCycles);

private:    // vars
    UInt cycleTime = 1;
    UInt kernelStartTime = 0;
    Int vcdVarKernel = 0;
    Vec<Int> vcdVarQubit;
    Vec<Vec<Int>> vcdVarSignal;
    Vec<Int> vcdVarCodeword;
};

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
