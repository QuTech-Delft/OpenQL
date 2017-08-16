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

    float sweep_points[]     = { 2, 4, 8, 16 };  // sizes of the clifford circuits per randomization
    int   num_circuits       = 4;

    // ql::init(ql::transmon_platform, "instructions.map");

    ql::init();
    // ql::init(ql::transmon_platform, "instructions.map");

    // create platform
    ql::quantum_platform starmon("starmon","test_cfg_cbox.json");

    // print info
    starmon.print_info();

    // set platform
    ql::set_platform(starmon);

    // create program
    ql::quantum_program prog("prog", 7, starmon);
    prog.set_sweep_points(sweep_points, num_circuits);

    ql::quantum_kernel kernel("kernel7",starmon);

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
