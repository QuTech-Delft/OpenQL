
/**
 * @file   unitarydecomp.h
 * @date   11/2018
 * @author Anneriet Krol
 * @brief  unitary decomposition implementation
 */
#ifndef ANNERIET
#define ANNERIET
#include <fstream>
#include <iomanip>
#include <complex>

#include <string>
#include <sstream>
#include <map>

#include <ql/matrix.h>
#include <ql/json.h>

#include <ql/openql.h>
#include <ql/exception.h>

using json = nlohmann::json;

typedef std::string instruction_t;

namespace ql
{

typedef std::string qasm_inst_t;
typedef std::string ucode_inst_t;

typedef std::string string_t;
typedef std::vector<std::string> strings_t;
typedef std::vector<std::string> ucode_sequence_t;


typedef std::map<qasm_inst_t, ucode_inst_t> dep_instruction_map_t;

extern dep_instruction_map_t dep_instruction_map;

/**
 * anneriet
 
class anneriet : public gate
{
public:
    cmat_t 	m;
    size_t	parameters;
/*    anneriet(size_t q) : m(hadamard_c)
    {
        name = "anneriet";
        duration = 40;
        operands.push_back(q);
    }*/

    anneriet(size_t q, cmat_t m) : m(m)
    {
        name = "annerietname";
        duration = 40;
        //operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("anneriet2 q[" + std::to_string(operands[0]) + "]");
    }

    instruction_t micro_code()
    {
        // y90 + x180
        return instruction_t("  pulse 1100 0000 1100\n     wait 10\n     pulse 1001 0000 1001\n     wait 10");
    }

    gate_type_t type()
    {
        return __composite_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};*/
}
#endif
