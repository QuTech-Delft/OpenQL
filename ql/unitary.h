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
#include <Eigen>

namespace ql
{

class unitary
{
public:
    std::string name;
    std::vector<std::complex<double>> array;
    std::vector<std::complex<double>> SU;
    double delta;
    double alpha;
    double beta;
    double gamma;
    bool is_decomposed;
    std::vector<double> instructionlist;

    typedef Eigen::Matrix<std::complex<double>, Eigen::Dynamic, Eigen::Dynamic> complex_matrix ;
    Eigen::Matrix<std::complex<double>, Eigen::Dynamic, Eigen::Dynamic> matrix;

    unitary() : name(""), is_decomposed(false) {}
    unitary(std::string name, std::vector<std::complex<double>> array) : 
            name(name), array(array), is_decomposed(false)
    {
        DOUT("constructing unitary: " << name 
                  << ", containing: " << array.size() << " elements");
        utils::print_vector(array,"[openql] unitary elements :"," , ");
        // TODO: add sanity checks for supplied arguments
    }

    double size()
    {
        return (double) array.size();
    }

    void decompose()
    {
        DOUT("decomposing Unitary: " << name);


        int matrix_size = (int)std::pow(array.size(),0.5);
        // compute the number of qubits: length of array is collumns*rows, so log2(sqrt(array.size))
        int numberofbits = (int) log2(matrix_size);


    

        Eigen::Map<complex_matrix> matrix(array.data(), matrix_size, matrix_size);
         //      DOUT("constructing unitary: " << name << ", containing: " << matrix << " elements");
        


        Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> identity = Eigen::MatrixXd::Identity(matrix_size, matrix_size);
        if(matrix.conjugate().transpose()*matrix != identity)
        {
            //Throw an error
            EOUT("Unitary " << name <<" is not a unitary matrix!");;
            throw ql::exception("Unitary '"+ name+"' is not a unitary matrix. Cannot be decomposed!", false);
        }
        else
        {
             utils::print_vector(array,"[openql] matrix is unitary :"," , ");
             std::cout << "size: " << numberofbits<< std::endl;
        }
  
        if(numberofbits == 1)
        {
            std::vector<double> tmp(4);
            tmp = zyz_decomp(array);
            delta = tmp[0];
            alpha = tmp[1];
            beta  = tmp[2];
            gamma = tmp[3];
        }
        else
        {
            //Eigen::JacobiSVD<Eigen::MatrixXd> svd;
            //svd.compute(matrix, Eigen::ComputeThinU | Eigen::ComputeThinV);
            //DOUT(svd.matrixU() );
            //DOUT(svd.matrixV()) ;
            matrix = CSD(matrix);
        }
        
        DOUT("Done decomposing");
        is_decomposed = true;
    }

    complex_matrix CSD(complex_matrix U)
    {

        std::cout << "u_rows " << U.rows() << std::endl;
        complex_matrix q1 = U.topLeftCorner(U.rows()/2,U.cols()/2);
        complex_matrix q2 = U.bottomLeftCorner(U.rows()/2,U.cols()/2);

        std::cout << "U: " << U << std::endl;
        std::cout << "q1: " << q1 << std::endl;
        std::cout << "q2: " << q2 << std::endl;
        Eigen::BDCSVD<complex_matrix> svd;
        if(q1.rows() > 1 && q1.cols() > 1)
        {          
        svd.compute(q1, Eigen::ComputeFullU | Eigen::ComputeFullV);
        }
        complex_matrix u1 = svd.matrixU();
        complex_matrix c = Eigen::Diagonal(svd.singularValues());
        complex_matrix v = svd.matrixV();
        std::cout << "u1: " << u1 << std::endl;
        std::cout << "c: " << c << std::endl;
        std::cout << "v: " << v << std::endl;
        Eigen::MatrixXd z = Eigen::MatrixXd::Identity(q1.rows(), q1.cols());
        return U;
    }

    std::vector<double> zyz_decomp(std::vector<std::complex<double>> matrix)
    {
         //TODO: make this efficient again
        ql::complex_t det = matrix[0]*matrix[3]-matrix[2]*matrix[1];
        //utils::print_vector(matrix, "matrix: " + std::to_string(matrix[0].real()) + ", " + std::to_string(matrix[1].real()) + ", "+ std::to_string(matrix[2].real()) + ", "+ std::to_string(matrix[3].real()) + ", "+ std::to_string(matrix[4].real()), "; ");

        double delta = atan2(det.imag(), det.real())/matrix.size();
        std::complex<double> com_two(0,1);
        std::complex<double> A = exp(-com_two*delta)*matrix[0];
        std::complex<double> B = exp(-com_two*delta)*matrix[1];
        double sw = sqrt(pow((double) B.imag(),2) + pow((double) B.real(),2) + pow((double) A.imag(),2));
        double wx = 0;
        double wy = 0;
        double wz = 0;
        if(sw > 0)
        {
        wx = B.imag()/sw;
        wy = B.real()/sw;
        wz = A.imag()/sw;
        }

        double t1 = atan2(A.imag(),A.real());
        double t2 = atan2(B.imag(), B.real());
        alpha = t1+t2;
        gamma = t1-t2;
        beta = 2*atan2(sw*sqrt(pow((double) wx,2)+pow((double) wy,2)),sqrt(pow((double) A.real(),2)+pow((wz*sw),2)));
        instructionlist.push_back(delta);
        instructionlist.push_back(alpha);
        instructionlist.push_back(beta);
        instructionlist.push_back(gamma);
        return instructionlist;
    }

    ~unitary()
    {
        // destroy unitary
        DOUT("destructing unitary: " << name);
    }
};


}

#endif // _UNITARY_H
