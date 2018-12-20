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

#include <ql/utils.h>
#include <ql/str.h>
#include <ql/gate.h>
#include <ql/exception.h>

namespace ql
{

class unitary
{
public:
    std::string name;
    std::vector<std::complex<double>> matrix;
    std::vector<std::complex<double>> SU;
    double delta;
    double alpha;
    double beta;
    double gamma;
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
        ql::complex_t det = matrix[1]*matrix[4]-matrix[3]*matrix[2];
        double delta = atan2(det.imag(), det.real())/matrix.size();
        std::complex<double> com_two(0,1);
        std::complex<double> A = exp(-com_two*delta)*matrix[0,0];
        std::complex<double> B = exp(-com_two*delta)*matrix[0,1];
        double sw = sqrt(pow((double) B.imag(),2) + pow((double) B.real(),2) + pow((double) A.imag(),2));
        double wx = B.imag()/sw;
        double wy = B.real()/sw;
        double wz = A.imag()/sw;
        double t1 = atan2(A.imag(),A.real());
        double t2 = atan2(B.imag(), B.real());
        alpha = t1+t2;
        gamma = t1-t2;
        beta = 2*atan2(sw*sqrt(pow((double) wx,2)+pow((double) wy,2)),sqrt(pow((double) A.real(),2)+pow((wz*sw),2)));
        DOUT("Done decomposing");
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
