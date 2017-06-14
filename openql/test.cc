
/*
 * Author: Imran Ashraf
 */

#include "openql.h"

int main()
{
    srand(0);

    vector<float> sweep_points = {2};  // sizes of the clifford circuits per randomization
    int num_circuits = 1;

    set_instruction_map_file("instructions.map");
    init();

    // create program
    Program p("aProg", 1);
    p.set_sweep_points(sweep_points, num_circuits);

    Kernel k("aKernel");
    k.prepz(0);
    k.measure(0);

    p.add_kernel(k);
    p.compile(false, true);
    p.schedule();

    return 0;
}