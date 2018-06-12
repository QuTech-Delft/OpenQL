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

enum class operation_type_t
{
    ARITHMATIC, RELATIONAL, BITWISE
};

class operation
{
public:
    std::string operation_name;
    std::string inv_operation_name;
    operation_type_t operation_type;
    std::vector<size_t> operands;

    operation() {}
    operation(size_t l, std::string op, size_t r)
    {
        operands.push_back(l);
        operands.push_back(r);
        if(op == "+")
        {
            operation_name = "add";
            operation_type = ql::operation_type_t::ARITHMATIC;
        }
        else if(op == "-")
        {
            operation_name = "sub";
            operation_type = ql::operation_type_t::ARITHMATIC;
        }
        else if(op == "&")
        {
            operation_name = "and";
            operation_type = ql::operation_type_t::BITWISE;
        }
        else if(op == "|")
        {
            operation_name = "or";
            operation_type = ql::operation_type_t::BITWISE;
        }
        else if(op == "^")
        {
            operation_name = "xor";
            operation_type = ql::operation_type_t::BITWISE;
        }
        else if(op == "==")
        {
            operation_name = "eq";
            inv_operation_name = "ne";
            operation_type = ql::operation_type_t::RELATIONAL;
        }
        else if(op == "!=")
        {
            operation_name = "ne";
            inv_operation_name = "eq";
            operation_type = ql::operation_type_t::RELATIONAL;
        }
        else if(op == "<")
        {
            operation_name = "lt";
            inv_operation_name = "ge";
            operation_type = ql::operation_type_t::RELATIONAL;
        }
        else if(op == ">")
        {
            operation_name = "gt";
            inv_operation_name = "le";
            operation_type = ql::operation_type_t::RELATIONAL;
        }
        else if(op == "<=")
        {
            operation_name = "le";
            inv_operation_name = "gt";
            operation_type = ql::operation_type_t::RELATIONAL;
        }
        else if(op == ">=")
        {
            operation_name = "ge";
            inv_operation_name = "lt";
            operation_type = ql::operation_type_t::RELATIONAL;
        }
        else
        {
            EOUT("Unknown binary operation '" << op );
            throw ql::exception("Unknown binary operation '"+op+"' !", false);
        }
    }

    // used for assign
    operation(size_t l)
    {
        operation_name = "assign";
        operation_type = ql::operation_type_t::ARITHMATIC;
        operands.push_back(l);
    }

    // // used for initializing with an imm 
    // operation(int val): lop(0), rop(0)
    // {
    //     operation_name = "assign_imm";
    //     operation_type = ql::operation_type_t::ARITHMATIC;
    // }

    operation(std::string op, size_t r)
    {
        if(op == "~")
        {
            operation_name = "not";
            operation_type = ql::operation_type_t::BITWISE;
            operands.push_back(r);
        }
        else
        {
            EOUT("Unknown unary operation '" << op );
            throw ql::exception("Unknown unary operation '"+op+"' !", false);
        }
    }

};


class classical : public gate
{
public:
    cmat_t m;
    int imm_value;

    classical(size_t dest, operation & oper)
    {
        name = oper.operation_name;
        duration = 20;
        operands.push_back(dest);
        for(auto & op : oper.operands)
            operands.push_back(op);
    }

    classical(std::string operation)
    {
        str::lower_case(operation);
        name=operation;
        duration = 20;
        if( (name == "nop") )
        {
            DOUT("Adding 0 operand operation: " << name);
        }
        else
        {
            EOUT("Unknown classical operation '" << name << "' with '0' operands!");
            throw ql::exception("Unknown classical operation'"+name+"' with'0' operands!", false);
        }
    }

    instruction_t qasm()
    {
        std::string iopers;
        int sz = operands.size();
        for(int i=0; i<sz; ++i)
        {
            if(i==sz-1)
                iopers += " r" + std::to_string(operands[i]);
            else
                iopers += " r" + std::to_string(operands[i]) + ",";
        }

        if(name == "assign")
        {
            return "mov" + iopers;
        }
        else if(name == "assign_imm")
        {
            iopers += ", " + std::to_string(imm_value);
            return "set" + iopers;
        }
        else if(name == "fmr")
        {
            return name + " r" + std::to_string(operands[0]) +
                          ", q" + std::to_string(operands[1]);
        }
        else
            return name + iopers;
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
