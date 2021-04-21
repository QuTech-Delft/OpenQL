/**
 * @file   openql.i
 * @author Imran Ashraf
 * @brief  swig interface file
 */
%define DOCSTRING
"`OpenQL` is a C++/Python framework for high-level quantum programming. The framework provides a compiler for compiling and optimizing quantum code. The compiler produces the intermediate quantum assembly language in cQASM (Common QASM) and the compiled eQASM (executable QASM) for various target platforms. While the eQASM is platform-specific, the quantum assembly code (QASM) is hardware-agnostic and can be simulated on the QX simulator."
%enddef

%module(docstring=DOCSTRING) openql
%feature("autodoc", "1");

%include "std_vector.i"
%include "std_map.i"
%include "exception.i"
%include "std_string.i"
%include "std_complex.i"

namespace std {
   %template(vectori) vector<int>;
   %template(vectorui) vector<size_t>;
   %template(vectorf) vector<float>;
   %template(vectord) vector<double>;
   %template(vectorc) vector<std::complex<double>>;
   %template(mapss) map<std::string, std::string>;
};

%{
#include "ql/api/api.h"
%}

%exception {
    try {
        $action
        if (PyErr_Occurred()) SWIG_fail;
    } catch (ql::utils::Exception &e) {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
    SWIG_CATCH_STDEXCEPT
    catch (...) {
        SWIG_exception(SWIG_UnknownError, "Unknown C++ exception");
    }
}

// Include the header file with above prototypes
%include "ql/api/api.h"

namespace std {
   %template(vectorp) vector<ql::api::Pass>;
};
