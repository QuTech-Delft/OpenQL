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

    int   num_circuits       = 13;
    float sweep_points[]     = { 2, 4, 8, 16, 32, 64, 128, 256, 512, 512.25, 512.75, 513.25, 513.75 };  // sizes of the clifford circuits per randomization

    ql::init(ql::transmon_platform, "instructions.map");

    // create program
    ql::quantum_program prog("prog", 7);
    prog.set_sweep_points(sweep_points, num_circuits);

    ql::quantum_kernel kernel("kernel7");

    for (int j=0; j<1; j++)
        kernel.hadamard(j);

    kernel.cnot(3,4);
    kernel.cnot(3,5);

    for (int j=3; j<6; j++)
        kernel.cnot(2,j);

    for (int i=1; i>=0; i--)
        for (int j=4; j<7; j++)
            kernel.cnot(i,j);

    prog.add(kernel);
    prog.compile( /*verbose*/ 1 );
    // println(prog.qasm());

    ql::quantum_program sprog = prog;
    sprog.schedule();
    // println(sprog.qasm());

    return 0;
}




// kernel.hadamard(0);
// kernel.hadamard(1);
// kernel.hadamard(2);
// kernel.cnot(3,4);
// kernel.cnot(3,5);
// kernel.cnot(2,3);
// kernel.cnot(2,4);
// kernel.cnot(2,6);
// kernel.cnot(1,3);
// kernel.cnot(1,5);
// kernel.cnot(1,6);
// kernel.cnot(0,3);
// kernel.cnot(0,5);
// kernel.cnot(0,6);
