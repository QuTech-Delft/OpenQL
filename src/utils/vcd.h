/**
 * @file   vcd.h
 * @date   20190515
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  generate Value Change Dump file for GTKWave viewer
 * @remark based on https://github.com/SanDisk-Open-Source/pyvcd/tree/master/vcd
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/map.h"

namespace ql {
namespace utils {

class Vcd {
public:
    enum class VarType { INT, STRING };
    enum class Scope { MODULE };

public:
    void start();
    void scope(Scope type, const Str &name);
    int registerVar(const Str &name, VarType type, Scope scope=Scope::MODULE);
    void upscope();
    void change(Int var, Int timestamp, const Str &value);
    void change(Int var, Int timestamp, Int value);
    void finish();
    Str getVcd();

private:
    struct Value {
        Int intVal;
        Str strVal;
    };
    typedef Map<Int, Value> VarChangeMap;        // map variable 'id' to 'tValue'
    typedef Map<Int, VarChangeMap> TimestampMap; // map 'timestamp' to variables

private:
    Int lastId;
    TimestampMap timestampMap;
    StrStrm vcd;
};

} // namespace utils
} // namespace ql
