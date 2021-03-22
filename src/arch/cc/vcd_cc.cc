/**
 * @file    arch/cc/vcd_cc.cc
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle generation of Value Change Dump file for GTKWave viewer
 * @note
 */

#include "vcd_cc.h"
#include "options.h"
#include "utils/filesystem.h"

namespace ql {
namespace arch {
namespace cc {

using namespace utils;

// NB: parameters qubitNumber and cycleTime originate from OpenQL variable 'platform'
void Vcd::programStart(UInt qubitNumber, Int cycleTime, Int maxGroups, const Settings &settings) {
    this->cycleTime = cycleTime;
    kernelStartTime = 0;

    // define header
    start();

    // define kernel variable
    scope(Vcd::Scope::MODULE, "kernel");
    vcdVarKernel = registerVar("kernel", Vcd::VarType::STRING);
    upscope();

    // define qubit variables
    scope(Vcd::Scope::MODULE, "qubits");
    vcdVarQubit.resize(qubitNumber);
    for (UInt q = 0; q < qubitNumber; q++) {
        Str name = "q" + to_string(q);
        vcdVarQubit[q] = registerVar(name, Vcd::VarType::STRING);
    }
    upscope();

    // define signal variables
    UInt instrsUsed = settings.getInstrumentsSize();
    scope(Vcd::Scope::MODULE, "sd.signal");
    vcdVarSignal.assign(instrsUsed, Vec<Int>(maxGroups, {0}));
    for (UInt instrIdx = 0; instrIdx < instrsUsed; instrIdx++) {
        const Json &instrument = settings.getInstrumentAtIdx(instrIdx);         // NB: always exists
        Str instrumentPath = QL_SS2S("instruments[" << instrIdx << "]");       // for JSON error reporting
        Str instrumentName = json_get<Str>(instrument, "name", instrumentPath);
        const utils::Json qubits = json_get<const utils::Json>(instrument, "qubits", instrumentPath);
        for (UInt group = 0; group < qubits.size(); group++) {
            Str name = instrumentName + "-" + to_string(group);
            vcdVarSignal[instrIdx][group] = registerVar(name, Vcd::VarType::STRING);
        }
    }
    upscope();

    // define codeword variables
    scope(Vcd::Scope::MODULE, "codewords");
    vcdVarCodeword.resize(qubitNumber);
    for(UInt instrIdx = 0; instrIdx < instrsUsed; instrIdx++) {
        const utils::Json &instrument = settings.getInstrumentAtIdx(instrIdx);         // NB: always exists
        Str instrumentPath = QL_SS2S("instruments[" << instrIdx << "]");       // for JSON error reporting
        Str instrumentName = utils::json_get<Str>(instrument, "name", instrumentPath);
        vcdVarCodeword[instrIdx] = registerVar(instrumentName, Vcd::VarType::STRING);
    }
    upscope();
}


void Vcd::programFinish(const Str &progName) {
    // generate VCD
    finish();

    // write VCD to file
    Str file_name(options::get("output_dir") + "/" + progName + ".vcd");
    QL_IOUT("Writing Value Change Dump to " << file_name);
    OutFile(file_name).write(getVcd());
}


void Vcd::kernelFinish(const Str &kernelName, UInt durationInCycles) {
    // NB: timing starts anew for every kernel
    UInt durationInNs = durationInCycles * cycleTime;
    change(vcdVarKernel, kernelStartTime, kernelName);          // start of kernel
    change(vcdVarKernel, kernelStartTime + durationInNs, "");   // end of kernel
    kernelStartTime += durationInNs;
}


void Vcd::bundleFinishGroup(
    UInt startCycle,
    UInt durationInCycles,
    Digital groupDigOut,
    const Str &signalValue,
    UInt instrIdx,
    Int group
) {
    // generate signal output for group
    UInt startTime = kernelStartTime + startCycle * cycleTime;
    UInt durationInNs = durationInCycles * cycleTime;
    Int var = vcdVarSignal[instrIdx][group];
    Str val = QL_SS2S(groupDigOut) + "=" + signalValue;
    change(var, startTime, val);                                // start of signal
    change(var, startTime + durationInNs, "");                  // end of signal
}


void Vcd::bundleFinish(UInt startCycle, Digital digOut, UInt maxDurationInCycles, UInt instrIdx) {
    // generate codeword output for instrument
    UInt startTime = kernelStartTime + startCycle * cycleTime;
    UInt durationInNs = maxDurationInCycles * cycleTime;
    Int var = vcdVarCodeword[instrIdx];
    Str val = QL_SS2S("0x" << std::hex << std::setfill('0') << std::setw(8) << digOut);
    change(var, startTime, val);                                // start of signal
    change(var, startTime+durationInNs, "");                    // end of signal
}


void Vcd::customGate(const Str &iname, const Vec<UInt> &qops, UInt startCycle, UInt durationInCycles) {
    // generate qubit VCD output
    UInt startTime = kernelStartTime + startCycle*cycleTime;
    UInt durationInNs = durationInCycles*cycleTime;
    for (UInt i = 0; i < qops.size(); i++) {
        Int var = vcdVarQubit[qops[i]];
        Str name = iname;                                       // FIXME: improve name for 2q gates
        change(var, startTime, name);                           // start of instruction
        change(var, startTime + durationInNs, "");              // end of instruction
    }
}

} // namespace cc
} // namespace arch
} // namespace ql
