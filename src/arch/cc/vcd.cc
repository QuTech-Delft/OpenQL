/**
 * @file   vcd.cc
 * @date   20190515
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  generate Value Change Dump file for GTKWave viewer
 * @remark based on https://github.com/SanDisk-Open-Source/pyvcd/tree/master/vcd
 */

#define OPT_DEBUG_VCD   0


#include "vcd.h"

#include <iostream>

namespace ql {

void Vcd::start()
{
    vcd << "$date today $end" << std::endl;
    vcd << "$timescale 1 ns $end" << std::endl;
}


void Vcd::scope(tScopeType type, const std::string &name)
{
    // FIXME: handle type
    vcd << "$scope " << "module" << " " << name << " $end" << std::endl;
}


int Vcd::registerVar(const std::string &name, tVarType type, tScopeType scope)
{
    // FIXME: incomplete
    const int width = 20;

    vcd << "$var string " << width << " " << lastId << " " << name << " $end" << std::endl;

    return lastId++;
}

void Vcd::upscope()
{
    vcd << "$upscope $end" << std::endl;
}


void Vcd::finish()
{
    vcd << "$enddefinitions $end" << std::endl;

    for(auto &t: timestampMap) {
        vcd << "#" << t.first << std::endl;      // timestamp
        for(auto &v: t.second) {
            vcd << "s" << v.second.strVal << " " << v.first << std::endl;
        }
    }
}


std::string Vcd::getVcd()
{
    return vcd.str();
}


void Vcd::change(int var, int timestamp, const std::string &value)
{
    auto tsIt = timestampMap.find(timestamp);
    if(tsIt != timestampMap.end()) {    // timestamp found
        tVarChangeMap &vcm = tsIt->second;

        auto vcmIt = vcm.find(var);
        if(vcmIt != vcm.end()) {        // var found
#if OPT_DEBUG_VCD
            std::cout << "ts=" << tsIt->first
                << ", var " << vcmIt->first
                << " overwritten with '" << value << "'" << std::endl;
#endif
            tValue &val = vcmIt->second;
            val.strVal = value;         // overwrite previous value. FIXME: only if it was empty?
        } else {                        // var not found
#if OPT_DEBUG_VCD
            std::cout << "ts=" << tsIt->first
                << ", var " << var
                << " not found, wrote value '" << value << "'" << std::endl;
#endif
            tValue val;
            val.strVal = value;
            vcm.insert({var, val});
        }
    } else {                            // timestamp not found
#if OPT_DEBUG_VCD
        std::cout << "ts=" << timestamp
            << " not found, wrote var " << var
            << " with value '" << value << "'" << std::endl;
#endif
        tValue val;
        val.strVal = value;

        tVarChangeMap varChange{{var, val}};
        timestampMap.insert({timestamp, varChange});
    }
}

void Vcd::change(int var, int timestamp, int value)
{
    // FIXME
}

} // namespace ql
