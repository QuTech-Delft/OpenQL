// #include "src/openql.h"

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

#include <src/openql.h>

int main(int argc, char ** argv)
{
    srand(0);

    float sweep_points[] = {2};

    // initialize the target platform

    // ql::init();
    // ql::init(ql::transmon_platform, "instructions.map");

    // create platform
    ql::quantum_platform starmon("starmon","test_cfg_cbox.json");

    // print info
    starmon.print_info();

    // set platform
    ql::set_platform(starmon);



    // create program
    ql::quantum_program prog("a_program", 7, starmon);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // create kernel
    ql::quantum_kernel kernel("a_kernel",starmon);

    // describe kernel
    for(int i=0;i<5;i++)
        kernel.prepz(i);

    for(int i=0;i<3;i++)
        kernel.hadamard(i);

    for (int i=2; i>=0; i--)
        for (int j=3; j<5; j++)
            kernel.cnot(i,j);

    prog.add(kernel);   // add kernel to program
    prog.compile();    // compile program

    ql::quantum_program sprog = prog;
    sprog.schedule();

    return 0;
}

