%module openql

%include "std_string.i"
%include "std_vector.i"

namespace std {
   %template(vectori) vector<int>;
   %template(vectorf) vector<float>;
   %template(vectord) vector<double>;
};

%{
#include "openql.h"
%}

// Include the header file with above prototypes
%include "openql.h"
