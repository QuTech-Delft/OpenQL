/**
 * @file   unitary.h
 * @date   12/2018
 * @author Imran Ashraf
 * @author Anneriet Krol
 * @brief  unitary matrix (decomposition) implementation
 */

#ifndef _UNITARY_H
#define _UNITARY_H

#include <complex>
#include <string>

#include <utils.h>
#include <str.h>
#include <gate.h>
#include <exception.h>

namespace ql
{

class unitary
{
public:
    std::string name;
    std::vector<std::complex<double>> matrix;
    bool is_decomposed;

    unitary() : name(""), is_decomposed(false) {}
    unitary(std::string name, std::vector<std::complex<double>> matrix) : 
            name(name), matrix(matrix), is_decomposed(false)
    {
        DOUT("constructing unitary: " << name 
                  << ", containing: " << matrix.size() << " elements");
        utils::print_vector(matrix,"[openql] unitary elements :"," , ");

        // TODO: add sanity checks for supplied arguments
    }

    void decompose()
    {

        DOUT("decomposing Unitary: " << name);

        // TODO: add decomposition code here


        is_decomposed = true;
    }

    ~unitary()
    {
        // destroy unitary
        DOUT("destructing unitary: " << name);
    }
};


}

#endif // _UNITARY_H
