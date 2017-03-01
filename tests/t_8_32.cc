#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include "ql/openql.h"

int main(int argc, char ** argv)
{
    srand(0);

    float sweep_points[]     = {2};  // sizes of the clifford circuits per randomization
    int   num_circuits       = 1;

    ql::init(ql::transmon_platform, "instructions.map");

    // create program
    ql::quantum_program prog("prog", 8);
    prog.set_sweep_points(sweep_points, num_circuits);

    ql::quantum_kernel kernel("kernel8");

    kernel.sdag(6);
    kernel.cphase(2,6);
    kernel.y(3);
    kernel.cnot(7,0);
    kernel.x(0);
    kernel.cphase(7,5);
    kernel.x(4);
    kernel.cphase(0,1);
    kernel.cnot(2,0);
    kernel.cphase(4,1);
    kernel.cnot(4,1);
    kernel.cnot(0,6);
    kernel.cnot(0,3);
    kernel.hadamard(0);
    kernel.hadamard(5);
    kernel.cnot(4,1);
    kernel.hadamard(7);
    kernel.cphase(4,6);
    kernel.hadamard(7);
    kernel.cnot(2,5);
    kernel.cphase(3,1);
    kernel.x(5);
    kernel.cphase(1,4);
    kernel.cnot(4,1);
    kernel.z(1);
    kernel.hadamard(2);
    kernel.hadamard(7);
    kernel.hadamard(5);
    kernel.hadamard(7);
    kernel.cnot(0,7);
    kernel.hadamard(0);
    kernel.x(1);

    prog.add(kernel);
    prog.compile( /*verbose*/ 1 );
    // println(prog.qasm());

    ql::quantum_program sprog = prog;
    sprog.schedule();
    // println(sprog.qasm());

    return 0;
}

/*
Sdag(q6);
CZ(q2,q6);
Y(q3);
CNOT(q7,q0);
X(q0);
CZ(q7,q5);
X(q4);
CZ(q0,q1);
CNOT(q2,q0);
CZ(q4,q1);
CNOT(q4,q1);
CNOT(q0,q6);
CNOT(q0,q3);
H(q0);
H(q5);
CNOT(q4,q1);
H(q7);
CZ(q4,q6);
H(q7);
CNOT(q2,q5);
CZ(q3,q1);
X(q5);
CZ(q1,q4);
CNOT(q4,q1);
Z(q1);
H(q2);
H(q7);
H(q5);
H(q7);
CNOT(q0,q7);
H(q0);
X(q1);
*/
