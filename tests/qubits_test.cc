// #include "ql/openql.h"

// int main(int argc, char ** argv)
// {
//     // initialize the target platform
//     ql::init(ql::transmon_platform, "instructions.map");
//     ql::quantum_program p("aProgram"); // create program
//     ql::quantum_kernel k("aKernel"); // create kernel

//     // populate kernel
//     for(int i=0;i<5;i++)
//         k.prepz(i);

//     for(int i=0;i<3;i++)
//         k.hadamard(i);

//     for (int i=2; i>=0; i--)
//         for (int j=3; j<5; j++)
//             k.cnot(i,j);

//     p.add(k);       // add kernel to program
//     p.compile();    // compile program

//     cout << p.qasm() << endl;
//     cout << p.microcode() << endl;
// }

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

    float sweep_points[] = {2};
    int   num_circuits   = 1;

    // initialize the target platform
    ql::init(ql::transmon_platform, "instructions.map");

    // create program
    ql::quantum_program prog("aProgram", 7);
    prog.set_sweep_points(sweep_points, num_circuits);

    // create kernel
    ql::quantum_kernel kernel("aKernel");

    // describe kernel
    for(int i=0;i<5;i++)
        kernel.prepz(i);

    for(int i=0;i<3;i++)
        kernel.hadamard(i);

    for (int i=2; i>=0; i--)
        for (int j=3; j<5; j++)
            kernel.cnot(i,j);

    prog.add(kernel);   // add kernel to program
    prog.compile(1);    // compile program

    ql::quantum_program sprog = prog;
    sprog.schedule();

    return 0;
}

