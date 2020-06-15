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
    std::vector<std::complex<double>> array;
    std::vector<std::complex<double>> SU;
    double delta; // JvS: is this even used?
    double alpha;
    double beta;
    double gamma;
    bool is_decomposed;
    std::vector<double> instructionlist;

    unitary();
    unitary(std::string name, std::vector<std::complex<double>> array);
    double size();
    void decompose();
};

}

#endif // _UNITARY_H
