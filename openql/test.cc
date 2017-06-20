
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
    Program p("aProg", 2);
    p.set_sweep_points(sweep_points, num_circuits);

    Kernel k("aKernel");
    k.prepz(0);
    k.prepz(1);
    k.cnot(0,1);
    k.cnot(0,1);
    k.cnot(0,1);
    k.measure(0);
    k.measure(1);

    p.add_kernel(k);
    p.compile(false, true);
    p.schedule();

    p.print_interaction_matrix();
    p.write_interaction_matrix();

    return 0;
}