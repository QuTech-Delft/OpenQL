/**
 * @file   openql.h
 * @author Imran Ashraf
 * @brief  header file for python interface
 */

#ifndef PYOPENQL_H
#define PYOPENQL_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <time.h>
#include "ql/openql.h"

std::string instruction_map_file = "instructions.map";

void set_instruction_map_file(std::string fname="instructions.map")
{
    instruction_map_file = fname;
}

std::string get_instruction_map_file()
{
    return instruction_map_file;
}

void init()
{
    ql::init(ql::transmon_platform, instruction_map_file);
}

class Kernel
{
    public:
        std::string name;
        ql::quantum_kernel *kernel;

        Kernel(std::string kname)
        {
            // std::cout << "Kernel::Kernel()" << std::endl;
            name = kname;
            kernel = new ql::quantum_kernel(name);
        }
        // std::string name() {return kernel_name;}

        void identity(size_t q0) { kernel->identity(q0); }
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
        void load_custom_instructions(std::string fname="instructions.json") { kernel->load_custom_instructions(fname); }
        void print_custom_instructions() { kernel->print_gates_definition(); }
        void gate(std::string name, std::vector<size_t> qubits) { kernel->gate(name, qubits); }
        void gate(std::string name, size_t qubit) { kernel->gate(name, qubit); }

        ~Kernel()
        {
            //std::cout << "Kernel::~Kernel()" << std::endl;
            delete(kernel);
        }
};

class Program
{
    private:
        ql::quantum_program *prog;

    public:
        std::string name;
        Program(std::string pname, size_t nqubits)
        {
            name = pname;
            // std::cout << "Program::Program()" << std::endl;
            prog = new ql::quantum_program(name, nqubits);
        }

        void set_sweep_points( std::vector<float> sweep_points, size_t num_circuits)
        {
            float* sp = &sweep_points[0];
            prog->set_sweep_points(sp, num_circuits);
        }

        void add_kernel(Kernel &k)
        {
            prog->add( *(k.kernel) );
        }

        void compile(bool optimize=false, bool verbose=false) { prog->compile(optimize, verbose); }
        void schedule(std::string scheduler="ASAP", bool verbose=false) { prog->schedule(scheduler, verbose); }
        std::string qasm() {return prog->qasm(); }
        std::string microcode() {return prog->microcode(); }

        ~Program()
        {
            // std::cout << "Program::~Program()" << std::endl;
            delete(prog);
        }
};

#endif
