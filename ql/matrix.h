/**
 * @file   matrix.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  unitary matrix implementation (taken from qx simulator)
 */

#ifndef MATRIX_H
#define MATRIX_H

#include <iomanip>
#include <iostream>
#include <complex>

namespace ql
{
   /**
    * \brief matrix
    */
   template <typename __T, size_t __N>
      class matrix
      {

	 public:

	    __T m[__N * __N] __attribute__((aligned(16))); 

	    /**
	     * default ctor
	     */
	    matrix() 
	    {
	       for (size_t i=0; i<(__N*__N); ++i)
		  m[i] = 0;
	    }

	    /**
	     * ctor
	     */
	    matrix(const __T * pm) 
	    {
	       for (size_t i=0; i<(__N*__N); ++i)
		  m[i] = pm[i];
	    }

	    __T& operator()(uint32_t r, uint32_t c)
	    {
	       return m[r*__N+c];
	    }

	    uint32_t size() const
	    {
	       return __N;
	    }

	    /**
	     * debug
	     */
	    void dump() const
	    {
	       std::cout << "[i] ---[matrix]-----------------------------------------------------" << std::endl;
	       std::cout << std::fixed;
	       for (int32_t r=0; r<__N; ++r)
	       {
		  for (int32_t c=0; c<__N; ++c)
		     std::cout << std::showpos << std::setw(5) << m[r*__N+c] << "\t";
		  std::cout << std::endl;
	       }
	       std::cout << "[i] ----------------------------------------------------------------" << std::endl;
	    }


      };

   typedef std::complex<double> complex_t;
   typedef matrix<complex_t,2>  cmat_t;

}

#endif // MATRIX_H
