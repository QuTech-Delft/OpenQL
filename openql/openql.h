/*
 * Author: Imran Ashraf
 */

#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <time.h>
#include "ql/openql.h"

class Kernel
{
    public:
        ql::quantum_kernel *kernel;

        Kernel()
        {
            std::string name("myKernel");
            std::cout << "Kernel::Kernel()" << std::endl;
            kernel = new ql::quantum_kernel(name);
        }

        void prepz(size_t q0) { kernel->prepz(q0); }
        void x(size_t q0) { kernel->x(q0); }
        void y(size_t q0) { kernel->y(q0); }
        void cnot(size_t q0, size_t q1) { kernel->cnot(q0,q1); }
        void toffoli(size_t q0, size_t q1, size_t q2) { kernel->toffoli(q0,q1,q2); }
        void measure(size_t q0) { kernel->measure(q0); }

        ~Kernel()
        {
            std::cout << "Kernel::~Kernel()" << std::endl;
            delete(kernel);
        }
};

class Program
{
    private:
        ql::quantum_program *prog;

    public:
        Program()
        {
            std::cout << "Program::Program()" << std::endl;
            prog = new ql::quantum_program("prog",3);
            float sweep_points[] = {2};
            int   num_circuits   = 1;
            ql::init(ql::transmon_platform, "instructions.map");
            prog->set_sweep_points(sweep_points, num_circuits);
        }

        void Add(Kernel &k)
        {
            std::cout << "Program::Add()" << std::endl;
            prog->add( *(k.kernel) );
        }

        void Compile() { prog->compile(1); }
        void Schedule() { prog->schedule(); }

        ~Program()
        {
            std::cout << "Program::~Program()" << std::endl;
            delete(prog);
        }
};

#endif