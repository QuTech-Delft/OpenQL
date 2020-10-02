/**
 * @file    vcd_cc.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle VCD file generation for the CC backend
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
    void programStart(int qubit_number, int cycle_time, int maxGroups, const settings_cc &settings);
    void programFinish(const std::string &progName);
    void kernelFinish(const std::string &kernelName, size_t durationInCycles);
    void bundleFinishGroup(size_t startCycle, unsigned int durationInNs, uint32_t groupDigOut, const std::string &signalValue, int instrIdx, int group);
    void bundleFinish(size_t startCycle, uint32_t digOut, size_t maxDurationInCycles, int instrIdx);
    void customgate(const std::string &iname, const std::vector<size_t> &qops, size_t startCycle, size_t durationInNs);

private:    // vars
    int cycle_time;
    unsigned int kernelStartTime;
    Vcd vcd;
    int vcdVarKernel;
    std::vector<int> vcdVarQubit;
    std::vector<std::vector<int>> vcdVarSignal;
    std::vector<int> vcdVarCodeword;
};

#endif // ndef ARCH_CC_VCD_CC_H
