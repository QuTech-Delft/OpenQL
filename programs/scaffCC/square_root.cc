#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include "ql/openql.h"

#define n 6 // problem size (log of database size)
#define pi 3.141592653589793238462643383279502884197

#define ABASE (0)
#define BBASE (ABASE+n)
#define XBASE (BBASE+n)
#define TBASE (BBASE+n)

void Sqr(ql::quantum_kernel &kernel)
{
    for(int i=0; i<=(n-1)/2; i++)
    {
        // CNOT(b[k], a[i]);
        kernel.cnot( ABASE+i, BBASE+2*i );
    }

    for(int i=(n+1)/2; i<n; i++)
    {
        // CNOT(b[k],   a[i]);
        kernel.cnot( ABASE+i, BBASE+(2*i) - n);
        // CNOT(b[k+1], a[i]);
        kernel.cnot( ABASE+i, BBASE+(2*i) - n + 1);
    }
}

void EQxMark(ql::quantum_kernel &kernel, int tF)
{
    int j;
    // Change b to reflect testing for the polynomial x
    for(j = 0; j < n; j++)
        if(j!=1)
            kernel.x(BBASE+j);

    // Compute x[n-2] = b[0] and b[1] and ... and b[n-1]
    for(j=0; j<n-1; j++)
        kernel.prepz(XBASE+j);

    kernel.toffoli( XBASE+0, BBASE+1, XBASE+0 );

    for(j = 1; j < n-1; j++)
        kernel.toffoli(XBASE+j, XBASE+j-1, BBASE+j+1);

    // Either return result in t or Phase flip conditioned on x[n-2]
    if(tF!=0)
    {
        kernel.cnot(XBASE+n-2, TBASE+0 ); // Returns result in t
    }
    else
    {
        kernel.z( XBASE+n-2 );  // Phase Flip==Z if b=01...0 == 'x', i.e. x[n-2]==1
    }

    // Undo the local registers
    for(j = n-2; j > 0; j--)
    {
        kernel.toffoli(BBASE+j+1, XBASE+j-1, XBASE+j);
    }

    kernel.toffoli(BBASE+0, BBASE+1, XBASE+0);

    // Restore b
    for(j = 0; j < n; j++)
        if(j!=1)
            kernel.x( BBASE+j );
}

void diffuse(ql::quantum_kernel &kernel)
{
    int j;

    // Hadamard applied to q
    for(j=0; j<n; j++) kernel.hadamard( ABASE+j );

    // Want to phase flip on q = 00...0
    // So invert q to compute q[0] and q[1] and ... and q[n-1]
    for(j = 0; j < n; j++) kernel.x(ABASE+j);

    // Compute x[n-2] = q[0] and q[1] and ... and q[n-1]
    for(j=0; j<n-1; j++) kernel.prepz(XBASE+j);

    kernel.toffoli(ABASE+0, ABASE+1, XBASE+0);

    for(j = 1; j < n-1; j++)
        kernel.toffoli(ABASE+j+1, XBASE+j-1, XBASE+j);

    // Phase flip conditioned on x[n-2]
    kernel.z(XBASE+n-2);  // Phase Flip==Z if q=00...0, i.e. x[n-2]==1

    // Undo the local registers
    for(j = n-2; j > 0; j--)
        kernel.toffoli(ABASE+j+1, XBASE+j-1, XBASE+j);

    kernel.toffoli(ABASE+0, ABASE+1, XBASE+0);

    // Restore q
    for(j = 0; j < n; j++) kernel.x( ABASE+j );

    // Complete the diffusion
    for(j=0; j<n; j++) kernel.hadamard( ABASE+j );
}

int main(int argc, char ** argv)
{
    srand(0);

    float sweep_points[] = {2};  // sizes of the clifford circuits per randomization
    int   num_circuits   = 1;

    ql::init(ql::transmon_platform, "instructions.map");

    // create program
    ql::quantum_program prog("prog", n*3+1);
    prog.set_sweep_points(sweep_points, num_circuits);
    ql::quantum_kernel kernel("kernelSqRoot");

    /***************************/
    // Grover parameters and step index
    int i, istep, N = pow(2,n);
    int nstep = floor((pi/4)*sqrt(N));

    for(i=0; i<n; i++)
        kernel.hadamard(i);

    for(istep=1; istep<=nstep; istep++)
    {
      Sqr(kernel);         // Sets b(x) = a(x) * a(x)
      EQxMark(kernel, 0);    // Tests if b(x) == x and if so Phase Flips
      Sqr(kernel);         // Note: Sqr is it's own inverse
      diffuse(kernel);       // Diffuse
    }

    // For the final measurement, compute causal state
    Sqr(kernel);
    EQxMark(kernel, 1);   // Note; 1 implies test result b(x)==x is returned in t

    // Now measure and report
    for(i=0; i<n; i++)
        kernel.measure(i);

    /***************************/

    prog.add(kernel);
    prog.compile( /*verbose*/ 1 );
    // println(prog.qasm());

    ql::quantum_program sprog = prog;
    sprog.schedule();
    // println(sprog.qasm());

    return 0;
}
