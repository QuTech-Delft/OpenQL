/**
 * @file   openql.h
 * @author Imran Ashraf
 * @brief  header file for python interface
 */

#ifndef PYOPENQL_H
#define PYOPENQL_H

#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <time.h>

#include <ql/openql.h>
#include <ql/qasm_loader.h>

void set_option(std::string option_name, std::string option_value)
{
    ql::options::set(option_name, option_value);
}

std::string get_option(std::string option_name)
{
    return ql::options::get(option_name);
}

void print_options()
{
    ql::options::print();
}

/**
 * quantum program interface
 */
class Platform
{
public:

    ql::quantum_platform * ql_platform;
    std::string            p_name;
    std::string            config_file;

    Platform() {}
    Platform(std::string name, std::string config_file) : p_name(name), config_file(config_file)
    {
        ql_platform = new ql::quantum_platform(name, config_file);
    }
    size_t get_qubit_number()
    {
        return ql_platform->get_qubit_number();
    }
};


/**
 * quantum kernel interface
 */
class Kernel
{
public:
    std::string name;
    ql::quantum_kernel * ql_kernel;

    Kernel(std::string kname, Platform platform)
    {
        name = kname;
        ql_kernel = new ql::quantum_kernel(name, *(platform.ql_platform));
    }
    void identity(size_t q0)
    {
        ql_kernel->identity(q0);
    }
    void hadamard(size_t q0)
    {
        ql_kernel->hadamard(q0);
    }
    void s(size_t q0)
    {
        ql_kernel->s(q0);
    }
    void sdag(size_t q0)
    {
        ql_kernel->sdag(q0);
    }
    void t(size_t q0)
    {
        ql_kernel->t(q0);
    }
    void tdag(size_t q0)
    {
        ql_kernel->tdag(q0);
    }
    void x(size_t q0)
    {
        ql_kernel->x(q0);
    }
    void y(size_t q0)
    {
        ql_kernel->y(q0);
    }
    void z(size_t q0)
    {
        ql_kernel->z(q0);
    }
    void rx90(size_t q0)
    {
        ql_kernel->rx90(q0);
    }
    void mrx90(size_t q0)
    {
        ql_kernel->mrx90(q0);
    }
    void rx180(size_t q0)
    {
        ql_kernel->rx180(q0);
    }
    void ry90(size_t q0)
    {
        ql_kernel->ry90(q0);
    }
    void mry90(size_t q0)
    {
        ql_kernel->mry90(q0);
    }
    void ry180(size_t q0)
    {
        ql_kernel->ry180(q0);
    }
    void rx(size_t q0, double angle)
    {
        ql_kernel->rx(q0, angle);
    }
    void ry(size_t q0, double angle)
    {
        ql_kernel->ry(q0, angle);
    }
    void rz(size_t q0, double angle)
    {
        ql_kernel->rz(q0, angle);
    }
    void measure(size_t q0)
    {
        ql_kernel->measure(q0);
    }
    void prepz(size_t q0)
    {
        ql_kernel->prepz(q0);
    }
    void cnot(size_t q0, size_t q1)
    {
        ql_kernel->cnot(q0,q1);
    }
    void cphase(size_t q0, size_t q1)
    {
        ql_kernel->cphase(q0,q1);
    }
    void cz(size_t q0, size_t q1)
    {
        ql_kernel->cz(q0,q1);
    }
    void toffoli(size_t q0, size_t q1, size_t q2)
    {
        ql_kernel->toffoli(q0,q1,q2);
    }
    void clifford(size_t id, size_t q0)
    {
        ql_kernel->clifford(id, q0);
    }
    void wait(std::vector<size_t> qubits, size_t duration)
    {
        ql_kernel->wait(qubits, duration);
    }
    void barrier(std::vector<size_t> qubits)
    {
        ql_kernel->wait(qubits, 0);
    }
    std::string get_custom_instructions()
    {
        return ql_kernel->get_gates_definition();
    }
    void display()
    {
        ql_kernel->display();
    }

    void gate(std::string name, std::vector<size_t> qubits, size_t duration=0, double angle=0.0)
    {
        ql_kernel->gate(name, qubits, duration, angle);
    }

    void classical(std::string name, std::vector<size_t> qubits)
    {
        ql_kernel->classical(name, qubits);
    }

    void controlled(Kernel &k,
        std::vector<size_t> control_qubits,
        std::vector<size_t> ancilla_qubits)
    {
        ql_kernel->controlled(k.ql_kernel, control_qubits, ancilla_qubits);
    }

    void conjugate(Kernel &k)
    {
        ql_kernel->conjugate(k.ql_kernel);
    }

    ~Kernel()
    {
        delete(ql_kernel);
    }
};


/**
 * quantum program interface
 */
class Program
{
public:
    std::string name;
    size_t qubits;
    Platform platf;
    ql::quantum_program *prog;

    Program() {}

    Program(std::string pname, size_t nqubits, Platform & platform)
    {
        name = pname;
        qubits = nqubits;
        platf = platform;
        prog = new ql::quantum_program(pname, nqubits, *(platform.ql_platform));
    }

    void set_sweep_points( std::vector<float> sweep_points, size_t num_sweep_points)
    {
        float* sp = &sweep_points[0];
        prog->set_sweep_points(sp, num_sweep_points);
    }

    void add_kernel(Kernel& k)
    {
        prog->add_for( *(k.ql_kernel), 1);
    }

    void add_program(Program& p)
    {
        prog->add_program(*(p.prog));
    }

    void add_if(Kernel& k, size_t condition_variable)
    {
        prog->add_if( *(k.ql_kernel), condition_variable);
    }

    void add_if(Program& p, size_t condition_variable)
    {
        prog->add_if( *(p.prog), condition_variable);
    }

    void add_if_else(Kernel& k_if, Kernel& k_else, size_t condition_variable)
    {
        prog->add_if_else( *(k_if.ql_kernel), *(k_else.ql_kernel), condition_variable);
    }

    void add_if_else(Program& p_if, Program& p_else, size_t condition_variable)
    {
        prog->add_if_else( *(p_if.prog), *(p_else.prog), condition_variable);
    }

    void add_while(Kernel& k, size_t condition_variable)
    {
        prog->add_while( *(k.ql_kernel), condition_variable );
    }

    void add_while(Program& p, size_t condition_variable)
    {
        prog->add_while( *(p.prog), condition_variable);
    }

    void add_for(Kernel& k, size_t iterations)
    {
        prog->add_for( *(k.ql_kernel), iterations);
    }

    void add_for(Program& p, size_t count)
    {
        prog->add_for( *(p.prog), count);
    }

    void compile()
    {
        prog->compile();
    }

    std::string qasm()
    {
        return prog->qasm();
    }

    std::string microcode()
    {
        return prog->microcode();
    }

    void print_interaction_matrix()
    {
        prog->print_interaction_matrix();
    }

    void write_interaction_matrix()
    {
        prog->write_interaction_matrix();
    }

    ~Program()
    {
        // std::cout << "program::~program()" << std::endl;
        delete(prog);
    }
};


/**
 * qasm code loader
 */
class QASM_Loader
{
   public:
      qx::qasm_loader      * loader;
      std::string            file_name;

      /**
       * constructor
       **/
      QASM_Loader(std::string file_name) : file_name(file_name)
      {
         loader = new qx::qasm_loader(file_name);
      }

      /**
       * read and parse the qasm file
       * @return 0 if success else error code
       **/
      size_t load()
      {
         return loader->parse();
      }

      /**
       * destructor
       */
      ~QASM_Loader()
      {
         delete loader;
      }
};


#endif
