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

#include <ql/openql.h>

void set_output_dir(std::string dir="output")
{
    ql::utils::set_output_dir(dir);
}

std::string get_output_dir()
{
    return ql::utils::get_output_dir();
}

class Platform
{
   public:

     ql::quantum_platform * ql_platform;
     std::string            p_name;
     std::string            config_file;

     Platform(std::string name, std::string config_file) : p_name(name), config_file(config_file)
     {
	ql_platform = new ql::quantum_platform(name,config_file);
     }
};

class Kernel
{
    public:
        std::string name;
        ql::quantum_kernel * ql_kernel;

        Kernel(std::string kname, Platform p)
        {
            // std::cout << "kernel::kernel()" << std::endl;
            name = kname;
            ql_kernel = new ql::quantum_kernel(name, *(p.ql_platform));
        }
        // std::string name() {return ql_kernel_name;}

        void identity(size_t q0) { ql_kernel->identity(q0); }
        void hadamard(size_t q0) { ql_kernel->hadamard(q0); }
        void s(size_t q0) { ql_kernel->s(q0); }
        void sdag(size_t q0) { ql_kernel->sdag(q0); }
        void t(size_t q0) { ql_kernel->t(q0); }
        void tdag(size_t q0) { ql_kernel->tdag(q0); }
        void x(size_t q0) { ql_kernel->x(q0); }
        void y(size_t q0) { ql_kernel->y(q0); }
        void z(size_t q0) { ql_kernel->z(q0); }
        void rx90(size_t q0) { ql_kernel->rx90(q0); }
        void mrx90(size_t q0) { ql_kernel->mrx90(q0); }
        void rx180(size_t q0) { ql_kernel->rx180(q0); }
        void ry90(size_t q0) { ql_kernel->ry90(q0); }
        void mry90(size_t q0) { ql_kernel->mry90(q0); }
        void ry180(size_t q0) { ql_kernel->ry180(q0); }
        void measure(size_t q0) { ql_kernel->measure(q0); }
        void prepz(size_t q0) { ql_kernel->prepz(q0); }
        void cnot(size_t q0, size_t q1) { ql_kernel->cnot(q0,q1); }
        void cphase(size_t q0, size_t q1) { ql_kernel->cphase(q0,q1); }
        void toffoli(size_t q0, size_t q1, size_t q2) { ql_kernel->toffoli(q0,q1,q2); }
        void clifford(size_t id, size_t q0) { ql_kernel->clifford(id, q0); }
        // void load_custom_instructions(std::string fname="instructions.json") { ql_kernel->load_custom_instructions(fname); }
        void print_custom_instructions() { ql_kernel->print_gates_definition(); }
        void gate(std::string name, std::vector<size_t> qubits) { ql_kernel->gate(name, qubits); }
        void gate(std::string name, size_t qubit) { ql_kernel->gate(name, qubit); }

        ~Kernel()
        {
            //std::cout << "kernel::~kernel()" << std::endl;
            delete(ql_kernel);
        }
};

class Program
{
    private:
        ql::quantum_program *prog;

    public:
        std::string name;
        Program(std::string pname, size_t nqubits, Platform p)
        {
            name = pname;
            // std::cout << "program::program()" << std::endl;
            prog = new ql::quantum_program(name, nqubits, *(p.ql_platform));
        }

        void set_sweep_points( std::vector<float> sweep_points, size_t num_circuits)
        {
            float* sp = &sweep_points[0];
            prog->set_sweep_points(sp, num_circuits);
        }

        void add_kernel(Kernel& k)
        {
            prog->add( *(k.ql_kernel) );
        }

        void compile(bool optimize=false, bool verbose=false) 
        {
            prog->compile(optimize, verbose); 
            prog->schedule("ALAP", verbose);
        }

        void schedule(std::string scheduler="ASAP", bool verbose=false) { prog->schedule(scheduler, verbose); }
        std::string qasm() {return prog->qasm(); }
        std::string microcode() {return prog->microcode(); }
        void print_interaction_matrix() { prog->print_interaction_matrix(); }
        void write_interaction_matrix() { prog->write_interaction_matrix(); }

        ~Program()
        {
            // std::cout << "program::~program()" << std::endl;
            delete(prog);
        }
};

#endif


// %feature("docstring") set_instruction_map_file
// """ Sets instruction map file.

// Parameters
// ----------
// arg1 : str
//     Releative path of instruction map file, default is instructions.map in current directory.
// """

// %feature("docstring") get_instruction_map_file
// """ Returns the path of current set instruction map file.

// Parameters
// ----------
// None

// Returns
// -------
// str
//     Path of instruction map file. """
