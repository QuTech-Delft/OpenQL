/**
 * @file   classical.h
 * @date   05/2018
 * @author Imran Ashraf
 * @brief  classical operation implementation
 */

#ifndef _CLASSICAL_H
#define _CLASSICAL_H

#include <fstream>
#include <iomanip>
#include <complex>

#include <string>
#include <sstream>
#include <map>

#include <ql/utils.h>
#include <ql/str.h>
#include <ql/gate.h>
#include <ql/exception.h>


namespace ql
{

class classical : public gate
{
public:
    cmat_t m;
    classical(std::string operation, std::vector<size_t> opers)
    {
        str::lower_case(operation);
        name=operation;
        operands=opers;
        duration = 20;
        int sz = operands.size();
        if(((name == "add") | (name == "sub") | (name == "mul") | (name == "div") | 
           (name == "and") | (name == "or") | (name == "xor") |
           (name == "eq") | (name == "ne") | (name == "lt") | (name == "gt") |
           (name == "le") | (name == "ge")) && (sz == 3))
        {
            DOUT("Adding 3 operand operation: " << name);
        }
        else if(((name == "not")) && (sz == 2))
        {
            DOUT("Adding 2 operand operation: " << name);
        }
        else
        {
            EOUT("Unknown classical operation '" << name << "' with '"<<sz<< "' operands!");
            throw ql::exception("Unknown classical operation'"+name+"' with'"+std::to_string(sz)+"' operands!", false);
        }
    }

    instruction_t qasm()
    {
        std::string ins = name;
        int sz = operands.size();
        for(int i=0; i<sz; ++i)
        {
            if(i==sz-1)
                ins += " r" + std::to_string(operands[i]);
            else
                ins += " r" + std::to_string(operands[i]) + ",";
        }
        return ins;
    }

    instruction_t micro_code()
    {
        return ql::dep_instruction_map["nop"];
    }

    gate_type_t type()
    {
        return __classical_gate__;
    }

    cmat_t mat()
    {
        return m;
    }

};

}

#endif // _CLASSICAL_H
