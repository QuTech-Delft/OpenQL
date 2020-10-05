/**
 * @file    vcd_cc.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle generation of Value Change Dump file for GTKWave viewer
 * @note
 */

#ifndef ARCH_CC_VCD_CC_H
#define ARCH_CC_VCD_CC_H

#include "vcd.h"
#include "settings_cc.h"

#include <vector>

class vcd_cc
{
public:     // funcs
    vcd_cc() = default;
    ~vcd_cc() = default;

    void programStart(int qubitNumber, int cycleTime, int maxGroups, const settings_cc &settings);
    void programFinish(const std::string &progName);
    void kernelFinish(const std::string &kernelName, size_t durationInCycles);
    void bundleFinishGroup(size_t startCycle, unsigned int durationInNs, uint32_t groupDigOut, const std::string &signalValue, int instrIdx, int group);
    void bundleFinish(size_t startCycle, uint32_t digOut, size_t maxDurationInCycles, int instrIdx);
    void customGate(const std::string &iname, const std::vector<size_t> &qops, size_t startCycle, size_t durationInNs);

private:    // vars
    Vcd vcd;
    int cycleTime;
    unsigned int kernelStartTime;
    int vcdVarKernel;
    std::vector<int> vcdVarQubit;
    std::vector<std::vector<int>> vcdVarSignal;
    std::vector<int> vcdVarCodeword;
};

#endif // ndef ARCH_CC_VCD_CC_H
