/**
 * @file    arch/cc/vcd_cc.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle generation of Value Change Dump file for GTKWave viewer
 * @note
 */

#pragma once

#include "utils/vcd.h"

#include "types_cc.h"
#include "settings_cc.h"

namespace ql {
namespace arch {
namespace cc {

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

} // namespace cc
} // namespace arch
} // namespace ql
