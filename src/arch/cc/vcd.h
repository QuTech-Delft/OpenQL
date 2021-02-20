/**
 * @file   vcd.h
 * @date   20190515
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  generate Value Change Dump file for GTKWave viewer
 * @remark based on https://github.com/SanDisk-Open-Source/pyvcd/tree/master/vcd
 */

#pragma once

#include <string>
#include <sstream>
#include <map>

namespace ql {

class Vcd {
public:
    typedef enum { VT_INT, VT_STRING } tVarType;
    typedef enum { ST_MODULE } tScopeType;

public:
    void start();
    void scope(tScopeType type, const std::string &name);
    int registerVar(const std::string &name, tVarType type, tScopeType scope=ST_MODULE);
    void upscope();
    void change(int var, int timestamp, const std::string &value);
    void change(int var, int timestamp, int value);
    void finish();
    std::string getVcd();

private:
    typedef struct {
        int intVal;
        std::string strVal;
    } tValue;
    typedef std::map<int, tValue> tVarChangeMap;        // map variable 'id' to 'tValue'
    typedef std::map<int, tVarChangeMap> tTimestampMap; // map 'timestamp' to variables

private:
    int lastId;
    tTimestampMap timestampMap;
    std::stringstream vcd;
};

} // namespace ql
