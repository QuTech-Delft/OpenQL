
/**
 * @file   unitarydecomp.h
 * @date   11/2018
 * @author Anneriet Krol
 * @brief  unitary decomposition implementation
 */
#ifndef UNITARYDECOMP
#define UNITARYDECOMP
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
//using json = nlohmann::json;

//typedef std::string instruction_t;

namespace ql{
class unitary
{
    public:
    std::string name;
    ql::cmat_t mat;
    vector<size_t> qubits;
    
    unitary(std::string name, ql::cmat_t mat) : name(name), mat(mat)
    {
    }

    vector<instruction_t> decompose(std::string option)
    {
        ql::complex_t det = mat.m[0]*mat.m[4]-mat.m[3]*mat.m[2];
        return {instruction_t("(det.real).toString()")};
        
    };

    ~unitary()
    {
    }

};
};

#endif
