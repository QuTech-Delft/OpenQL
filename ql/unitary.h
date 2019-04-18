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
#include <unsupported/Eigen/MatrixFunctions>

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

        Eigen::Map<complex_matrix> matrix(array.data(), matrix_size, matrix_size);
         //      DOUT("constructing unitary: " << name << ", containing: " << matrix << " elements");
        
        // compute the number of qubits: length of array is collumns*rows, so log2(sqrt(array.size))
        int numberofbits = (int) log2(matrix_size);

        // Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> identity = Eigen::MatrixXi::Identity(matrix_size, matrix_size);
        // if( ((Eigen::MatrixXcd) matrix*matrix.adjoint()) != identity)
        // {
        //     //Throw an error
        //     EOUT("Unitary " << name <<" is not a unitary matrix!");

        //     throw ql::exception("Unitary '"+ name+"' is not a unitary matrix. Cannot be decomposed!"+to_string(matrix*matrix.adjoint()), false);
        // }

        decomp_function(matrix, numberofbits);
        utils::print_vector(instructionlist, "Instruction list: ", "; ");

        
        DOUT("Done decomposing");
        is_decomposed = true;
    }

    std::string to_string(complex_matrix m, std::string vector_prefix = "",
                            std::string elem_sep = ", ")
    {
        std::ostringstream ss;
        ss << m << "\n";
        // ss << vector_prefix << " [";
        // Eigen::VectorXcd v = Eigen::Map<Eigen::VectorXcd>(m.data, m.size());
        // size_t sz = v.size();
        // if(sz > 0)
        // {
        //     size_t i;
        //     for (i=0; i<sz*sz-1; ++i)
        //         ss << v[i] << elem_sep;
        //     ss << v[i];
        // }

        // ss << "]";
        return ss.str();
    }


    void decomp_function(complex_matrix matrix, int numberofbits)
    {           
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
            complex_matrix L0;
            complex_matrix L1;
            complex_matrix R0;
            complex_matrix R1;
            complex_matrix cc;
            complex_matrix ss;
            CSD(matrix, L0, L1, R0,R1,cc,ss);
            demultiplexing(R0,R1, numberofbits-1);
            multicontrolledY(ss,numberofbits-1);
            demultiplexing(L0,L1, numberofbits-1);
        }
    }

    void CSD(complex_matrix U, complex_matrix &u1, complex_matrix &u2, complex_matrix &v1, complex_matrix &v2, complex_matrix &c, complex_matrix &s)
    {
        //Cosine sine decomposition
        // U = [q1, U01] = [u1    ][c  s][v1  ]
        //     [q2, U11] = [    u2][-s c][   v2]
        int n = U.rows();
        int m = U.cols();
        // std::cout << "u_rows " << n << std::endl;
        complex_matrix q1 = U.topLeftCorner(n/2,m/2);
        complex_matrix q2 = U.bottomLeftCorner(n/2,m/2);

        // std::cout << "U: " << U << std::endl;
        // std::cout << "q1: " << q1 << std::endl;
        // std::cout << "q2: " << q2 << std::endl;
        Eigen::BDCSVD<complex_matrix> svd;

        if(q1.rows() > 1 && q1.cols() > 1)
        {          
        svd.compute(q1, Eigen::ComputeFullU | Eigen::ComputeFullV);
        }
        u1 = svd.matrixU();
        c = svd.singularValues().asDiagonal();
        // std::cout << "c: " << c << std::endl;

        // thinCSD: q1 = u1*c*v1.adjoint()
        //          q2 = u2*s*v1.adjoint()
        v1 = svd.matrixV();
        int p = q1.rows();
        complex_matrix z = Eigen::MatrixXd::Identity(p, p).colwise().reverse();
        // std::cout << "u1: " << u1 << std::endl;
        // std::cout << "c: " << c << std::endl;
        // std::cout << "z: " << z << std::endl;
        u1 = u1*z;
        v1 = v1*z;
        q2 = q2*v1;
        c = z*c*z;
        int k = 0;
        for(int j = 1; j < p; j++)
        {
            if(c(j,j).real() <= 0.70710678119)
            {
                k = j;
            }
        }
        complex_matrix b = q2.block( 0,0, p, k);

        Eigen::HouseholderQR<complex_matrix> qr;
        qr.compute(b);        
        //complex_matrix r = qr.matrixQR();
        u2 = qr.householderQ();
        s = u2.adjoint();
        // std::cout << "s: " << s << std::endl;
        if(k < p)
        {
            // std::cout << "k: " << k << std::endl;
            svd.compute(s.block(k, k, p-k, p-k));
            // std::cout << "singular values: \n" << ((complex_matrix) svd.singularValues()) << std::endl;
            s.block(k, k, p-k, p-k) = svd.singularValues().asDiagonal();
            // std::cout << "s: " << s << std::endl;
            c.block(0,k, p,p-k) = c.block(0,k, p,k)*svd.matrixV();
            u2.block(0,k, p,p-k) = u2.block(0,k, p,p-k)*svd.matrixU();
            v1.block(0,k, p,p-k) = v1.block(0,k, p,p-k)*svd.matrixV();
            qr.compute(c.block(k,k, p-k,p-k));
            c.block(k,k,p-k,p-k) = qr.matrixQR();
            // std::cout << "c: " << c << std::endl;
            u1.block(0,k, p,p-k) = u1.block(0,k, p,p-k)*qr.householderQ(); 
            
            // std::cout << "z: " << ((complex_matrix) qr.householderQ()) << std::endl;
            // std::cout << "u1: " << u1 << std::endl;
            // std::cout << "u2: " << u2 << std::endl;
            // std::cout << "v1: " << v1 << std::endl;
        }
        for(int j = 0; j < p; j++)
        {
            if(c(j,j).real() < 0)
            {
                c(j,j) = -c(j,j);
                u1(j,j) = -u1(j,j);
            }
            if(s(j,j).real() < 0)
            {
                s(j,j) = -s(j,j);
                u2(j,j) = -u2(j,j);
            }
        }
        std::cout << "reconstructed q1: " << u1*c*v1.adjoint() << std::endl;
        std::cout << "reconstructed q2: " << u2*s*v1.adjoint() << std::endl;
        v2 = complex_matrix(n/2, n/2);
        v1 = v1.adjoint();
        s = -s;
        for(int i = 0; i < n/2; i++)
        {
            if(std::abs(s(i,i)) > std::abs(c(i,i)))
            {
                complex_matrix tmp = u1.adjoint()*U.block(n/2,n/2, 0, n/2);
                v2.row(i) = tmp.row(i)/s(i,i);
            }
            else
            {
                complex_matrix tmp = u2.adjoint()*U.block(n/2,n/2, n/2, n/2);
                v2.row(i) = tmp.row(i)/c(i,i);
            }
        }
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
        //instructionlist.push_back(delta);
        instructionlist.push_back(alpha);
        instructionlist.push_back(beta);
        instructionlist.push_back(gamma);
        return instructionlist;
    }

    void zyz_decomp(complex_matrix matrix)
    {
        ql::complex_t det = matrix(0,0)*matrix(1,1)-matrix(1,0)*matrix(0,1);
        //utils::print_vector(matrix, "matrix: " + std::to_string(matrix[0].real()) + ", " + std::to_string(matrix[1].real()) + ", "+ std::to_string(matrix[2].real()) + ", "+ std::to_string(matrix[3].real()) + ", "+ std::to_string(matrix[4].real()), "; ");

        double delta = atan2(det.imag(), det.real())/matrix.size();
        std::complex<double> com_two(0,1);
        std::complex<double> A = exp(-com_two*delta)*matrix(0,0);
        std::complex<double> B = exp(-com_two*delta)*matrix(0,1); //to comply with the other y-gate definition?
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
        //instructionlist.push_back(delta); //this is not used for the total decomposition
        instructionlist.push_back(alpha);
        instructionlist.push_back(beta);
        instructionlist.push_back(gamma);
    }


    void demultiplexing(complex_matrix U1, complex_matrix U2, int numberofcontrolbits)
    {
        if(U1 == U2)
        {
            if(U1.rows() == 2)
            {
                zyz_decomp(U1);
                //if U1 2x2, then the total gate is 4x4 = 2 qubit gates, which is a total of 18 rotation gates = 18 angles -> need to put 15 zeroes so the count is the same (and optimize them out later)
                for(int i = 0; i < 15; i++)
                {
                    instructionlist.push_back(0);
                }
            }
            else
            {
            EOUT("Unitaries are equal: optimization not implemented yet!");;
            throw ql::exception("Unitaries are equal: optimization not implemented yet!", false);
            }
        }
        else
        {
            Eigen::ComplexEigenSolver<Eigen::MatrixXcd> eigslv(U1*U2.adjoint());
            complex_matrix d = eigslv.eigenvalues().asDiagonal();
            complex_matrix V = eigslv.eigenvectors();

            std::cout << "d: " << d << std::endl;
            complex_matrix D = d.sqrt();
            std::cout << "D: " << D << std::endl;
            complex_matrix W = D*V*U2;
            if(W.rows() == 2)
            {
                zyz_decomp(W);
            }
            else
            {
                decomp_function(W, std::log2(W.rows()));
            }
            multicontrolledZ(D,numberofcontrolbits);
            if(V.rows() == 2)
            {
                zyz_decomp(V);
            }
            else
            {
                decomp_function(V, std::log2(V.rows()));
            }
        }
    }

    // returns M^k = (-1)^(b_(i-1)*g_(i-1)), where * is bitwise inner product, g = binary gray code, b = binary code.
    Eigen::MatrixXd genMk(int n)
    {
        //int b = n;
        //int g = n^(n>>1);
        Eigen::MatrixXd Mk(n,n);
        for(int i = 0; i < n; i++)
        {
            for(int j = 0; j < n ;j++)
            {
                Mk(i,j) =std::pow(-1, i*(j^(j>>1)));
            }
        }
        return Mk;
    }

    void multicontrolledY(complex_matrix ss, int numberofcontrolbits)
    {
        Eigen::VectorXd temp =  2*Eigen::asin(ss.real().diagonal().array());
        Eigen::VectorXd tr = (genMk(std::pow(2,numberofcontrolbits))).colPivHouseholderQr().solve(temp);
        for(int i = 0; i < std::pow(2, numberofcontrolbits); i++)
        {
            instructionlist.push_back(-tr[i]);
        }
    }

    void multicontrolledZ(complex_matrix D, int numberofcontrolbits)
    {
        Eigen::VectorXd temp =  (2*Eigen::log(D.diagonal().array())/(std::complex<double>(0,2))).real();
        Eigen::VectorXd tr = ((genMk(std::pow(2,numberofcontrolbits))).colPivHouseholderQr().solve(temp));
        for(int i = 0; i < std::pow(2, numberofcontrolbits); i++)
        {
            instructionlist.push_back(-tr[i]);
        }
    }

    ~unitary()
    {
        // destroy unitary
        DOUT("destructing unitary: " << name);
    }
};


}

#endif // _UNITARY_H
