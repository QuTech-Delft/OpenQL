/**
 * @file   vcd.cc
 * @date   20190515
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  generate Value Change Dump file for GTKWave viewer
 * @remark based on https://github.com/SanDisk-Open-Source/pyvcd/tree/master/vcd
 */


#include "vcd.h"

#include <iostream>






void Vcd::start()
{
    vcd << "$date today $end" << std::endl;
    vcd << "$timescale 1 ns $end" << std::endl;
    vcd << "$scope module a $end" << std::endl;
    vcd << "$scope module b $end" << std::endl;
    vcd << "$scope module c $end" << std::endl;
}


void Vcd::finish()
{
    vcd << "$upscope $end" << std::endl;
    vcd << "$upscope $end" << std::endl;
    vcd << "$upscope $end" << std::endl;
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


int Vcd::registerVar(std::string name, tVarType type, tScopeType scope)
{
    // FIXME: incomplete
    const int width = 20;

    vcd << "$var string " << width << " " << lastId << " " << name << " $end" << std::endl;

    return lastId++;
}

void Vcd::change(int var, int timestamp, std::string value)
{
    auto tsIt = timestampMap.find(timestamp);
    if(tsIt != timestampMap.end()) {    // timestamp found
        tVarChangeMap &vcm = tsIt->second;

        auto vcmIt = vcm.find(var);
        if(vcmIt != vcm.end()) {        // var found
            std::cout << "ts=" << tsIt->first
                << ", var " << vcmIt->first
                << " overwritten with '" << value << "'" << std::endl;

#if 1
            tValue val = vcmIt->second;
            val.strVal = value;         // overwrite previous value
#else
            vcm.erase(vcmIt);
            tValue val;
            val.strVal = value;
            vcm.insert({var, val});
#endif
        } else {                        // var not found
            std::cout << "ts=" << tsIt->first
                << ", var " << var
                << " not found, wrote value '" << value << "'" << std::endl;

            tValue val;
            val.strVal = value;
            vcm.insert({var, val});
        }
    } else {                            // timestamp not found
        std::cout << "ts=" << timestamp
            << " not found, wrote var " << var
            << " with value '" << value << "'" << std::endl;

        tValue val;
        val.strVal = value;

        tVarChangeMap varChange{{var, val}};
        timestampMap.insert({timestamp, varChange});
    }
}

void Vcd::change(int var, int timestamp, int value)
{
}






