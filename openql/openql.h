/**
 * @file   openql.h
 * @author Imran Ashraf
 * @brief  header file for python interface
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

        Kernel(std::string name)
        {
            std::cout << "Kernel::Kernel()" << std::endl;
            kernel = new ql::quantum_kernel(name);
        }

        void hadamard(size_t q0) { kernel->hadamard(q0); }
        void s(size_t q0) { kernel->s(q0); }
        void sdag(size_t q0) { kernel->sdag(q0); }
        void x(size_t q0) { kernel->x(q0); }
        void y(size_t q0) { kernel->y(q0); }
        void z(size_t q0) { kernel->z(q0); }
        void rx90(size_t q0) { kernel->rx90(q0); }
        void mrx90(size_t q0) { kernel->mrx90(q0); }
        void rx180(size_t q0) { kernel->rx180(q0); }
        void ry90(size_t q0) { kernel->ry90(q0); }
        void mry90(size_t q0) { kernel->mry90(q0); }
        void ry180(size_t q0) { kernel->ry180(q0); }
        void measure(size_t q0) { kernel->measure(q0); }
        void prepz(size_t q0) { kernel->prepz(q0); }
        void cnot(size_t q0, size_t q1) { kernel->cnot(q0,q1); }
        void cphase(size_t q0, size_t q1) { kernel->cphase(q0,q1); }
        void toffoli(size_t q0, size_t q1, size_t q2) { kernel->toffoli(q0,q1,q2); }
        void clifford(size_t id, size_t q0) { kernel->clifford(id, q0); }

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
        Program(std::string name, size_t nqubits)
        {
            std::cout << "Program::Program()" << std::endl;
            prog = new ql::quantum_program(name,nqubits);
            ql::init(ql::transmon_platform, "instructions.map");
        }

        void set_sweep_points( std::vector<float> sweep_points, size_t num_circuits)
        {
            float* sp = &sweep_points[0];
            prog->set_sweep_points(sp, num_circuits);
        }

        void add_kernel(Kernel &k)
        {
            std::cout << "Program::Add()" << std::endl;
            prog->add( *(k.kernel) );
        }

        void compile() { prog->compile(1); }
        void schedule() { prog->schedule(); }
        std::string qasm() {return prog->qasm(); }
        std::string microcode() {return prog->microcode(); }

        ~Program()
        {
            std::cout << "Program::~Program()" << std::endl;
            delete(prog);
        }
};

#endif