/**
 * @file   unitary.cc
 * @date   12/2018
 * @author Imran Ashraf
 * @author Anneriet Krol
 * @brief  unitary matrix (decomposition) implementation
 */

#include <unitary.h>

#include <Eigen/MatrixFunctions>
#include <src/misc/lapacke.h>

#include <chrono>

namespace ql
{

// JvS: this was originally the class "unitary" itself, but compile times of
// Eigen are so excessive that I moved it into its own compile unit and
// provided a wrapper instead. It doesn't actually NEED to be wrapped like
// this, because the Eigen::Matrix<...> member is actually only used within
// the scope of a single method (calling other methods), but I'm not touching
// this code.
class UnitaryDecomposer
{
private:
    Eigen::Matrix<std::complex<double>, Eigen::Dynamic, Eigen::Dynamic> _matrix;

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

    UnitaryDecomposer() : name(""), is_decomposed(false) {}

    UnitaryDecomposer(std::string name, std::vector<std::complex<double>> array) : 
            name(name), array(array), is_decomposed(false)
    {
        DOUT("constructing unitary: " << name 
                  << ", containing: " << array.size() << " elements");
    }

    double size()
    {
        if(!array.empty())
            return (double) array.size();
        else
            return (double) _matrix.size();
    }

    complex_matrix getMatrix()
    {
        if (!array.empty())
        {
            int matrix_size = (int)std::pow(array.size(), 0.5);

            Eigen::Map<complex_matrix> matrix(array.data(), matrix_size, matrix_size);
            _matrix = matrix.transpose();
        }
        return _matrix;
    }

    void decompose()
    {
        DOUT("decomposing Unitary: " << name);

        getMatrix();
        int matrix_size = _matrix.rows();
        
        // compute the number of qubits: length of array is collumns*rows, so log2(sqrt(array.size))
        int numberofbits = uint64_log2(matrix_size);

        Eigen::MatrixXcd identity = Eigen::MatrixXcd::Identity(matrix_size, matrix_size);
        Eigen::MatrixXcd matmatadjoint = (_matrix.adjoint()*_matrix);
        // very little accuracy because of tests using printed-from-matlab code that does not have many digits after the comma    
        if( !matmatadjoint.isApprox(identity, 0.001))
        {
            //Throw an error
            EOUT("Unitary " << name <<" is not a unitary matrix!");

            throw ql::exception("Error: Unitary '"+ name+"' is not a unitary matrix. Cannot be decomposed!" + to_string(matmatadjoint), false);
        }
        // initialize the general M^k lookuptable
        genMk();

        decomp_function(_matrix, numberofbits); //needed because the matrix is read in columnmajor
 
        DOUT("Done decomposing");
        is_decomposed = true;
    }

    std::string to_string(complex_matrix m, std::string vector_prefix = "",
                            std::string elem_sep = ", ")
    {
        std::ostringstream ss;
        ss << m << "\n";
        return ss.str();
    }


    // std::chrono::duration<double> CSD_time;
    // std::chrono::duration<double> CSD_time2;
    // std::chrono::duration<double> CSD_time3;
    // std::chrono::duration<double> zyz_time;
    // std::chrono::duration<double> multiplexing_time;
    // std::chrono::duration<double> demultiplexing_time;

    void decomp_function(const Eigen::Ref<const complex_matrix>& matrix, int numberofbits)
    {          
        DOUT("decomp_function: \n" << to_string(matrix));         
        if(numberofbits == 1)
        {
            zyz_decomp(matrix);
        }
        else
        {
            int n = matrix.rows()/2;

            complex_matrix V(n,n);
            complex_matrix W(n,n);
            Eigen::VectorXcd D(n);
            // if q2 is zero, the whole thing is a demultiplexing problem instead of full CSD
            if(matrix.bottomLeftCorner(n,n).isZero(10e-14) && matrix.topRightCorner(n,n).isZero(10e-14))
            {
                DOUT("Optimization: q2 is zero, only demultiplexing will be performed.");
                instructionlist.push_back(200.0);
                if(matrix.topLeftCorner(n, n).isApprox(matrix.bottomRightCorner(n,n),10e-4))
                {
                    DOUT("Optimization: Unitaries are equal, skip one step in the recursion for unitaries of size: " << n << " They are both: " << matrix.topLeftCorner(n, n));
                    instructionlist.push_back(300.0);
                    decomp_function(matrix.topLeftCorner(n, n), numberofbits-1);
                }
                else
                {
                    demultiplexing(matrix.topLeftCorner(n, n), matrix.bottomRightCorner(n,n), V, D, W, numberofbits-1);

                    decomp_function(W, numberofbits-1);
                    multicontrolledZ(D, D.rows());
                    decomp_function(V, numberofbits-1);
                }
            }
            // Check to see if it the kronecker product of a bigger matrix and the identity matrix.
            // By checking if the first row is equal to the second row one over, and if thelast two rows are equal 
            // Which means the last qubit is not affected by this gate
            else if (matrix(Eigen::seqN(0, n, 2), Eigen::seqN(1, n, 2)).isZero()  && matrix(Eigen::seqN(1, n, 2), Eigen::seqN(0, n, 2)).isZero()  && matrix.block(0,0,1,2*n-1) == matrix.block(1,1,1,2*n-1) &&  matrix.block(2*n-2,0,1,2*n-1) ==  matrix.block(2*n-1,1,1,2*n-1))
            {
                DOUT("Optimization: last qubit is not affected, skip one step in the recursion.");
                // Code for last qubit not affected
                instructionlist.push_back(100.0);
                decomp_function(matrix(Eigen::seqN(0, n, 2), Eigen::seqN(0, n, 2)), numberofbits-1);

            }
            else
            {
            complex_matrix ss(n,n);
            complex_matrix L0(n,n);
            complex_matrix L1(n,n);
            complex_matrix R0(n,n);
            complex_matrix R1(n,n);
            // auto start = std::chrono::steady_clock::now();
            CSD(matrix, L0, L1, R0, R1, ss);
            // CSD_time += (std::chrono::steady_clock::now() - start);
            demultiplexing(R0, R1, V, D, W, numberofbits-1);
            decomp_function(W, numberofbits-1);
            multicontrolledZ(D, D.rows());
            decomp_function(V, numberofbits-1);

            multicontrolledY(ss.diagonal(), n);

            demultiplexing(L0, L1, V, D, W, numberofbits-1);
            decomp_function(W, numberofbits-1);
            multicontrolledZ(D, D.rows());
            decomp_function(V, numberofbits-1);
            }
        }
    }

    void CSD(const Eigen::Ref<const complex_matrix>& U, Eigen::Ref<complex_matrix> u1, Eigen::Ref<complex_matrix> u2, Eigen::Ref<complex_matrix> v1, Eigen::Ref<complex_matrix> v2, Eigen::Ref<complex_matrix> s)
    {
        // auto start = std::chrono::steady_clock::now();        
        //Cosine sine decomposition
        // U = [q1, U01] = [u1    ][c  s][v1  ]
        //     [q2, U11] = [    u2][-s c][   v2]
        int n = U.rows();
        // complex_matrix c(n,n); // c matrix is not needed for the higher level
        // complex_matrix q1 = U.topLeftCorner(n/2,m/2);

        Eigen::BDCSVD<complex_matrix> svd(n/2,n/2);
        svd.compute(U.topLeftCorner(n/2,n/2), Eigen::ComputeThinU | Eigen::ComputeThinV); // possible because it's square anyway
        

        // thinCSD: q1 = u1*c*v1.adjoint()
        //          q2 = u2*s*v1.adjoint()
        int p = n/2;
        // complex_matrix z = Eigen::MatrixXd::Identity(p, p).colwise().reverse();
        complex_matrix c(svd.singularValues().reverse().asDiagonal());
        u1.noalias() = svd.matrixU().rowwise().reverse();
        v1.noalias() = svd.matrixV().rowwise().reverse(); // Same v as in matlab: u*s*v.adjoint() = q1

        complex_matrix q2 = U.bottomLeftCorner(p,p)*v1;      

        int k = 0;
        for(int j = 1; j < p; j++)
        {
            if(c(j,j).real() <= 0.70710678119)
            {
                k = j;
            }
        }
        //complex_matrix b = q2.block( 0,0, p, k+1);

        Eigen::HouseholderQR<complex_matrix> qr(p,k+1);
        qr.compute(q2.block( 0,0, p, k+1));
        u2 = qr.householderQ();
        s.noalias() = u2.adjoint()*q2;
        if(k < p-1)
        {
            DOUT("k is smaller than size of q1 = "<< p << ", adjustments will be made, k = " << k);
            k = k+1;
            Eigen::BDCSVD<complex_matrix> svd2(p-k, p-k);
            svd2.compute(s.block(k, k, p-k, p-k), Eigen::ComputeThinU | Eigen::ComputeThinV);
            s.block(k, k, p-k, p-k) = svd2.singularValues().asDiagonal();
            c.block(0,k, p,p-k) = c.block(0,k, p,p-k)*svd2.matrixV();
            u2.block(0,k, p,p-k) = u2.block(0,k, p,p-k)*svd2.matrixU();
            v1.block(0,k, p,p-k) = v1.block(0,k, p,p-k)*svd2.matrixV();
            
            Eigen::HouseholderQR<complex_matrix> qr2(p-k, p-k);

            qr2.compute(c.block(k,k, p-k,p-k));
            c.block(k,k,p-k,p-k) = qr2.matrixQR().triangularView<Eigen::Upper>();
            u1.block(0,k, p,p-k) = u1.block(0,k, p,p-k)*qr2.householderQ(); 
        }
        // CSD_time2 += (std::chrono::steady_clock::now() - start);

        // auto start2 = std::chrono::steady_clock::now();



        std::vector<int> c_ind;
        std::vector<int> s_ind;
        for(int j = 0; j < p; j++)
        {
            if(c(j,j).real() < 0)
            {
                c_ind.push_back(j);
            }
            if(s(j,j).real() < 0)
            {
                s_ind.push_back(j);
            } 
        }

        c(c_ind,c_ind) = -c(c_ind,c_ind);
        u1(Eigen::all, c_ind) = -u1(Eigen::all, c_ind);

        //s.diagonal()(s_ind) = -s.diagonal()(s_ind);
        s(s_ind,s_ind) = -s(s_ind,s_ind);
        u2(Eigen::all, s_ind) = -u2(Eigen::all, s_ind);

        if(!U.topLeftCorner(p,p).isApprox(u1*c*v1.adjoint(), 10e-8) || !U.bottomLeftCorner(p,p).isApprox(u2*s*v1.adjoint(), 10e-8))
        {
            if(U.topLeftCorner(p,p).isApprox(u1*c*v1.adjoint(), 10e-8))
            {
                DOUT("q1 is correct");
            }
            else
            {
                DOUT("q1 is not correct! (is not usually an issue");
                DOUT("q1: \n" << U.topLeftCorner(p,p));
                DOUT("reconstructed q1: \n" << u1*c*v1.adjoint());

            }
            if(U.bottomLeftCorner(p,p).isApprox(u2*s*v1.adjoint(), 10e-8))
            {
                DOUT("q2 is correct");
            }
            else
            {
                DOUT("q2 is not correct! (is not usually an issue)");
                DOUT("q2: " << U.bottomLeftCorner(p,p));
                DOUT("reconstructed q2: " << u2*s*v1.adjoint());
            }
        }
        v1.adjointInPlace(); // Use this instead of = v1.adjoint (to avoid aliasing issues)
        s = -s;

        complex_matrix tmp_s = u1.adjoint()*U.topRightCorner(p,p);
        complex_matrix tmp_c = u2.adjoint()*U.bottomRightCorner(p,p);

        // std::vector<int> c_ind_row;
        // std::vector<int> s_ind_row;
        for(int i = 0; i < p; i++)
        {
            if(std::abs(s(i,i)) > std::abs(c(i,i)))
            {
                // std::vector<int> s_ind_row;
                v2.row(i).noalias() = tmp_s.row(i)/s(i,i);                
            }
            else
            {
                // c_ind_row.push_back(i);
                v2.row(i).noalias() = tmp_c.row(i)/c(i,i);
            }
        }
        

        // v2(s_ind_row, Eigen::all) = tmp_s(s_ind_row, Eigen::all).rowwise() / s(s_ind_row, s_ind_row).array();
        // v2(c_ind_row, Eigen::all) = tmp_c(c_ind_row, Eigen::all).rowwise() / c(c_ind_row, c_ind_row).array();
        // U = [q1, U01] = [u1    ][c  s][v1  ]
        //     [q2, U11] = [    u2][-s c][   v2]
    
        complex_matrix tmp(n,n);
        tmp.topLeftCorner(p,p) = u1*c*v1;
        tmp.bottomLeftCorner(p,p) = -u2*s*v1;
        tmp.topRightCorner(p,p) = u1*s*v2;
        tmp.bottomRightCorner(p,p) = u2*c*v2;
        // Just to see if it kinda matches
        if(!tmp.isApprox(U, 10e-2))
        {
            throw ql::exception("CSD of unitary '"+ name+"' is wrong! Failed at matrix: \n"+to_string(tmp) + "\nwhich should be: \n" + to_string(U), false);
        }
            // CSD_time3 += (std::chrono::steady_clock::now() - start2);

    }


    void zyz_decomp(const Eigen::Ref<const complex_matrix>& matrix)
    {
        // auto start = std::chrono::steady_clock::now();

        ql::complex_t det = matrix.determinant();// matrix(0,0)*matrix(1,1)-matrix(1,0)*matrix(0,1);

        double delta = atan2(det.imag(), det.real())/matrix.rows();
        std::complex<double> A = exp(std::complex<double>(0,-1)*delta)*matrix(0,0);
        std::complex<double> B = exp(std::complex<double>(0,-1)*delta)*matrix(0,1); //to comply with the other y-gate definition
        
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
        instructionlist.push_back(-gamma);
        instructionlist.push_back(-beta);
        instructionlist.push_back(-alpha);
        // zyz_time += (std::chrono::steady_clock::now() - start);
    }

    void demultiplexing(const Eigen::Ref<const complex_matrix> &U1, const Eigen::Ref<const complex_matrix> &U2,  Eigen::Ref<complex_matrix> V,  Eigen::Ref<Eigen::VectorXcd> D,  Eigen::Ref<complex_matrix> W, int numberofcontrolbits)
    {
        // [U1 0 ]  = [V 0][D 0 ][W 0]
        // [0  U2]    [0 V][0 D*][0 W]
        // auto start = std::chrono::steady_clock::now(); 
        complex_matrix check = U1*U2.adjoint();
        // complex_matrix D;
        // complex_matrix V;
        // complex_matrix W;
        if(check == check.adjoint())
        {
            IOUT("Demultiplexing matrix is self-adjoint()");
            Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> eigslv(check);
            D.noalias() = ((complex_matrix) eigslv.eigenvalues()).cwiseSqrt();
            V.noalias() = eigslv.eigenvectors();
            W.noalias() = D.asDiagonal()*V.adjoint()*U2;
        }
        else
        {
            if (numberofcontrolbits < 5) //schur is faster for small matrices
            {
                Eigen::ComplexSchur<complex_matrix> decomposition(check);
                D.noalias() = decomposition.matrixT().diagonal().cwiseSqrt();
                V.noalias() = decomposition.matrixU();
                W.noalias() = D.asDiagonal() * V.adjoint() * U2;
            }
            else
            {
                Eigen::ComplexEigenSolver<complex_matrix> decomposition(check);
                D.noalias() = decomposition.eigenvalues().cwiseSqrt();
                V.noalias() = decomposition.eigenvectors();
                W.noalias() = D.asDiagonal() * V.adjoint() * U2;
            }
        }
    
        // demultiplexing_time += (std::chrono::steady_clock::now() - start);
        if(!(V*V.adjoint()).isApprox(Eigen::MatrixXd::Identity(V.rows(), V.rows()), 10e-3))
        {
            DOUT("Eigenvalue decomposition incorrect: V is not unitary, adjustments will be made");
            Eigen::BDCSVD<complex_matrix> svd3(V.block(0,0,V.rows(),2), Eigen::ComputeFullU);
            V.block(0,0,V.rows(),2) = svd3.matrixU();
            svd3.compute(V(Eigen::all,Eigen::seq(Eigen::last-1,Eigen::last)), Eigen::ComputeFullU);
            V(Eigen::all,Eigen::seq(Eigen::last-1,Eigen::last)) = svd3.matrixU();
             
        }


        complex_matrix Dtemp = D.asDiagonal();
        if(!U1.isApprox(V*Dtemp*W, 10e-2) || !U2.isApprox(V*Dtemp.adjoint()*W, 10e-2))
        {
            EOUT("Demultiplexing not correct!");
            throw ql::exception("Demultiplexing of unitary '"+ name+"' not correct! Failed at matrix U1: \n"+to_string(U1)+ "and matrix U2: \n" +to_string(U2) + "\nwhile they are: \n" + to_string(V*D.asDiagonal()*W) + "\nand \n" + to_string(V*D.conjugate().asDiagonal()*W), false);
        }


        
    }


    std::vector<Eigen::MatrixXd> genMk_lookuptable;

    // returns M^k = (-1)^(b_(i-1)*g_(i-1)), where * is bitwise inner product, g = binary gray code, b = binary code.
    void genMk()
    {
        int numberqubits = uint64_log2(_matrix.rows());
        for(int n = 1; n <= numberqubits; n++)
        {
            int size=1<<n;
            Eigen::MatrixXd Mk(size,size);
            for(int i = 0; i < size; i++)
            {
                for(int j = 0; j < size ;j++)
                {
                    Mk(i,j) =std::pow(-1, bitParity(i&(j^(j>>1))));
                }
            }
        genMk_lookuptable.push_back(Mk);
        }
        
        // return genMk_lookuptable[numberqubits-1];
    }

    // source: https://stackoverflow.com/questions/994593/how-to-do-an-integer-log2-in-c user Todd Lehman
    int uint64_log2(uint64_t n)
    {
    #define S(k) if (n >= (UINT64_C(1) << k)) { i += k; n >>= k; }

    int i = -(n == 0); S(32); S(16); S(8); S(4); S(2); S(1); return i;

    #undef S
    }

    int bitParity(int i)
    {
        if (i < 2 << 16)
        {
            i = (i >> 16) ^ i;
            i = (i >> 8) ^ i;
            i = (i >> 4) ^ i;
            i = (i >> 2) ^ i;
            i = (i >> 1) ^ i;
            return i % 2;
        }
        else
        {
            throw ql::exception("Bit parity number too big!", false);
        }
    }

    void multicontrolledY(const Eigen::Ref<const Eigen::VectorXcd> &ss, int halfthesizeofthematrix)
    {
        // auto start = std::chrono::steady_clock::now();
        Eigen::VectorXd temp =  2*Eigen::asin(ss.array()).real();
        Eigen::CompleteOrthogonalDecomposition<Eigen::MatrixXd> dec(genMk_lookuptable[uint64_log2(halfthesizeofthematrix)-1]);
        Eigen::VectorXd tr = dec.solve(temp);
        // Check is very approximate to account for low-precision input matrices
        if(!temp.isApprox(genMk_lookuptable[uint64_log2(halfthesizeofthematrix)-1]*tr, 10e-2))
        {
                EOUT("Multicontrolled Y not correct!");
                throw ql::exception("Demultiplexing of unitary '"+ name+"' not correct! Failed at demultiplexing of matrix ss: \n"  + to_string(ss), false);
        }

        instructionlist.insert(instructionlist.end(), &tr[0], &tr[halfthesizeofthematrix]);
        // multiplexing_time += std::chrono::steady_clock::now() - start;
    }

    void multicontrolledZ(const Eigen::Ref<const Eigen::VectorXcd> &D, int halfthesizeofthematrix)
    {
        // auto start = std::chrono::steady_clock::now();
        
        Eigen::VectorXd temp =  (std::complex<double>(0,-2)*Eigen::log(D.array())).real();
        Eigen::CompleteOrthogonalDecomposition<Eigen::MatrixXd> dec(genMk_lookuptable[uint64_log2(halfthesizeofthematrix)-1]);
        Eigen::VectorXd tr = dec.solve(temp);
        // Check is very approximate to account for low-precision input matrices
        if(!temp.isApprox(genMk_lookuptable[uint64_log2(halfthesizeofthematrix)-1]*tr, 10e-2))
        {
                EOUT("Multicontrolled Z not correct!");
                throw ql::exception("Demultiplexing of unitary '"+ name+"' not correct! Failed at demultiplexing of matrix D: \n"+ to_string(D), false);
        }
        

        instructionlist.insert(instructionlist.end(), &tr[0], &tr[halfthesizeofthematrix]);
        // multiplexing_time += std::chrono::steady_clock::now() - start;

    }
    ~UnitaryDecomposer()
    {
        // destroy unitary
        DOUT("destructing unitary: " << name);
    }
};

unitary::unitary() : name(""), is_decomposed(false) {}

unitary::unitary(std::string name, std::vector<std::complex<double>> array) :
name(name), array(array), is_decomposed(false) {}

double unitary::size() {
    // JvS: Note that the original unitary::size() used
    // Eigen::Matrix::size() if the array is empty. However, if the array
    // is empty, _matrix is never initialized beyond its default ctor,
    // which "allocates" a 0x0 matrix, and is thus size 0, exactly what
    // array.size() would return.
    // Don't get me started about why this returns a double.
    return (double) array.size();
}

void unitary::decompose() {
    UnitaryDecomposer decomposer(name, array);
    decomposer.decompose();
    SU = decomposer.SU;
    alpha = decomposer.alpha;
    beta = decomposer.beta;
    gamma = decomposer.gamma;
    is_decomposed = decomposer.is_decomposed;
    instructionlist = decomposer.instructionlist;
}

}

