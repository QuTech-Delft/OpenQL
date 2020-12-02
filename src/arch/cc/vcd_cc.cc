/**
 * @file    vcd_cc.cc
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle generation of Value Change Dump file for GTKWave viewer
 * @note
 */

#include "vcd_cc.h"
#include "options.h"
#include "utils/filesystem.h"

namespace ql {

// NB: parameters qubitNumber and cycleTime originate from OpenQL variable 'platform'
void vcd_cc::programStart(int qubitNumber, int cycleTime, int maxGroups, const settings_cc &settings)
{
    this->cycleTime = cycleTime;
    kernelStartTime = 0;

    // define header
    vcd.start();

    // define kernel variable
    vcd.scope(vcd.ST_MODULE, "kernel");
    vcdVarKernel = vcd.registerVar("kernel", Vcd::VT_STRING);
    vcd.upscope();

    // define qubit variables
    vcd.scope(vcd.ST_MODULE, "qubits");
    vcdVarQubit.resize(qubitNumber);
    for(size_t q=0; q<qubitNumber; q++) {
        std::string name = "q"+std::to_string(q);
        vcdVarQubit[q] = vcd.registerVar(name, Vcd::VT_STRING);
    }
    vcd.upscope();

    // define signal variables
    size_t instrsUsed = settings.getInstrumentsSize();
    vcd.scope(vcd.ST_MODULE, "sd.signal");
    vcdVarSignal.assign(instrsUsed, std::vector<int>(maxGroups, {0}));
    for(size_t instrIdx=0; instrIdx<instrsUsed; instrIdx++) {
        const utils::Json &instrument = settings.getInstrumentAtIdx(instrIdx);         // NB: always exists
        std::string instrumentPath = QL_SS2S("instruments[" << instrIdx << "]");       // for JSON error reporting
        std::string instrumentName = utils::json_get<std::string>(instrument, "name", instrumentPath);
        const utils::Json qubits = utils::json_get<const utils::Json>(instrument, "qubits", instrumentPath);
        for(size_t group=0; group<qubits.size(); group++) {
            std::string name = instrumentName+"-"+std::to_string(group);
            vcdVarSignal[instrIdx][group] = vcd.registerVar(name, Vcd::VT_STRING);
        }
    }
    vcd.upscope();

    // define codeword variables
    vcd.scope(vcd.ST_MODULE, "codewords");
    vcdVarCodeword.resize(qubitNumber);
    for(size_t instrIdx=0; instrIdx<instrsUsed; instrIdx++) {
        const utils::Json &instrument = settings.getInstrumentAtIdx(instrIdx);         // NB: always exists
        std::string instrumentPath = QL_SS2S("instruments[" << instrIdx << "]");       // for JSON error reporting
        std::string instrumentName = utils::json_get<std::string>(instrument, "name", instrumentPath);
        vcdVarCodeword[instrIdx] = vcd.registerVar(instrumentName, Vcd::VT_STRING);
    }
    vcd.upscope();
}


void vcd_cc::programFinish(const std::string &progName)
{
    // generate VCD
    vcd.finish();

    // write VCD to file
    std::string file_name(options::get("output_dir") + "/" + progName + ".vcd");
    QL_IOUT("Writing Value Change Dump to " << file_name);
    utils::OutFile(file_name).write(vcd.getVcd());
}


void vcd_cc::kernelFinish(const std::string &kernelName, size_t durationInCycles)
{
    // NB: timing starts anew for every kernel
    unsigned int durationInNs = durationInCycles*cycleTime;
    vcd.change(vcdVarKernel, kernelStartTime, kernelName);          // start of kernel
    vcd.change(vcdVarKernel, kernelStartTime + durationInNs, "");   // end of kernel
    kernelStartTime += durationInNs;
}


void vcd_cc::bundleFinishGroup(size_t startCycle, unsigned int durationInCycles, uint32_t groupDigOut, const std::string &signalValue, int instrIdx, int group)
{
    // generate signal output for group
    unsigned int startTime = kernelStartTime + startCycle*cycleTime;
    unsigned int durationInNs = durationInCycles*cycleTime;
    int var = vcdVarSignal[instrIdx][group];
    std::string val = QL_SS2S(groupDigOut) + "=" + signalValue;
    vcd.change(var, startTime, val);                                // start of signal
    vcd.change(var, startTime+durationInNs, "");                    // end of signal
}


void vcd_cc::bundleFinish(size_t startCycle, uint32_t digOut, size_t maxDurationInCycles, int instrIdx)
{
    // generate codeword output for instrument
    unsigned int startTime = kernelStartTime + startCycle*cycleTime;
    unsigned int durationInNs = maxDurationInCycles*cycleTime;
    int var = vcdVarCodeword[instrIdx];
    std::string val = QL_SS2S("0x" << std::hex << std::setfill('0') << std::setw(8) << digOut);
    vcd.change(var, startTime, val);                                // start of signal
    vcd.change(var, startTime+durationInNs, "");                    // end of signal
}


void vcd_cc::customGate(const std::string &iname, const utils::Vec<utils::UInt> &qops, size_t startCycle, size_t durationInCycles)
{
    // generate qubit VCD output
    unsigned int startTime = kernelStartTime + startCycle*cycleTime;
    unsigned int durationInNs = durationInCycles*cycleTime;
    for(size_t i=0; i<qops.size(); i++) {
        int var = vcdVarQubit[qops[i]];
        std::string name = iname;                                   // FIXME: improve name for 2q gates
        vcd.change(var, startTime, name);                           // start of instruction
        vcd.change(var, startTime+durationInNs, "");                // end of instruction
    }
}

} // namespace ql
