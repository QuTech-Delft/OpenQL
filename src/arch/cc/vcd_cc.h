/**
 * @file    vcd_cc.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle generation of Value Change Dump file for GTKWave viewer
 * @note
 */

#pragma once

#include "vcd.h"
#include "settings_cc.h"
#include "utils/vec.h"

namespace ql {

class vcd_cc
{
public:     // funcs
    vcd_cc() = default;
    ~vcd_cc() = default;

    void programStart(int qubitNumber, int cycleTime, int maxGroups, const settings_cc &settings);
    void programFinish(const std::string &progName);
    void kernelFinish(const std::string &kernelName, size_t durationInCycles);
    void bundleFinishGroup(size_t startCycle, unsigned int durationInCycles, uint32_t groupDigOut, const std::string &signalValue, int instrIdx, int group);
    void bundleFinish(size_t startCycle, uint32_t digOut, size_t maxDurationInCycles, int instrIdx);
    void customGate(const std::string &iname, const utils::Vec<utils::UInt> &qops, size_t startCycle, size_t durationInCycles);

private:    // vars
    Vcd vcd;
    int cycleTime;
    unsigned int kernelStartTime;
    int vcdVarKernel;
    std::vector<int> vcdVarQubit;
    std::vector<std::vector<int>> vcdVarSignal;
    std::vector<int> vcdVarCodeword;
};

} // namespace ql
