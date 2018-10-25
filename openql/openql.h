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

#include <ql/version.h>
#include <ql/openql.h>
#include <ql/classical.h>
#include <ql/qasm_loader.h>

static std::string get_version()
{
    return OPENQL_VERSION_STRING;
}

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
    std::string            name;
    std::string            config_file;
    ql::quantum_platform * platform;

    Platform() {}
    Platform(std::string name, std::string config_file) : name(name), config_file(config_file)
    {
        platform = new ql::quantum_platform(name, config_file);
    }
    size_t get_qubit_number()
    {
        return platform->get_qubit_number();
    }
};

class CReg
{
public:
    ql::creg* creg;
    CReg()
    {
        creg = new ql::creg();
    }
    ~CReg()
    {
        delete(creg);
    }
};

class Operation
{
public:
    ql::operation * operation;
    Operation(CReg& lop, std::string op, CReg& rop)
    {
        operation = new ql::operation(*(lop.creg), op, *(rop.creg));
    }
    Operation(std::string op, CReg& rop)
    {
        operation = new ql::operation(op, *(rop.creg));
    }
    Operation(CReg& lop)
    {
        operation = new ql::operation(*(lop.creg));
    }
    Operation(int val)
    {
        operation = new ql::operation(val);
    }
    ~Operation()
    {
        delete(operation);
    }
};


/**
 * quantum kernel interface
 */
class Kernel
{
public:
    std::string name;
    Platform platform;    
    size_t qubit_count;
    size_t creg_count;
    ql::quantum_kernel * kernel;

    Kernel(std::string name, Platform platform, size_t qubit_count, size_t creg_count=0):
        name(name), platform(platform), qubit_count(qubit_count), creg_count(creg_count)
    {
        kernel = new ql::quantum_kernel(name, *(platform.platform), qubit_count, creg_count);
    }
    void identity(size_t q0)
    {
        kernel->identity(q0);
    }
    void hadamard(size_t q0)
    {
        kernel->hadamard(q0);
    }
    void s(size_t q0)
    {
        kernel->s(q0);
    }
    void sdag(size_t q0)
    {
        kernel->sdag(q0);
    }
    void t(size_t q0)
    {
        kernel->t(q0);
    }
    void tdag(size_t q0)
    {
        kernel->tdag(q0);
    }
    void x(size_t q0)
    {
        kernel->x(q0);
    }
    void y(size_t q0)
    {
        kernel->y(q0);
    }
    void z(size_t q0)
    {
        kernel->z(q0);
    }
    void rx90(size_t q0)
    {
        kernel->rx90(q0);
    }
    void mrx90(size_t q0)
    {
        kernel->mrx90(q0);
    }
    void rx180(size_t q0)
    {
        kernel->rx180(q0);
    }
    void ry90(size_t q0)
    {
        kernel->ry90(q0);
    }
    void mry90(size_t q0)
    {
        kernel->mry90(q0);
    }
    void ry180(size_t q0)
    {
        kernel->ry180(q0);
    }
    void rx(size_t q0, double angle)
    {
        kernel->rx(q0, angle);
    }
    void ry(size_t q0, double angle)
    {
        kernel->ry(q0, angle);
    }
    void rz(size_t q0, double angle)
    {
        kernel->rz(q0, angle);
    }
    void measure(size_t q0)
    {
        kernel->measure(q0);
    }
    void prepz(size_t q0)
    {
        kernel->prepz(q0);
    }
    void cnot(size_t q0, size_t q1)
    {
        kernel->cnot(q0,q1);
    }
    void cphase(size_t q0, size_t q1)
    {
        kernel->cphase(q0,q1);
    }
    void cz(size_t q0, size_t q1)
    {
        kernel->cz(q0,q1);
    }
    void toffoli(size_t q0, size_t q1, size_t q2)
    {
        kernel->toffoli(q0,q1,q2);
    }
    void clifford(size_t id, size_t q0)
    {
        kernel->clifford(id, q0);
    }
    void wait(std::vector<size_t> qubits, size_t duration)
    {
        kernel->wait(qubits, duration);
    }
    void barrier(std::vector<size_t> qubits)
    {
        kernel->wait(qubits, 0);
    }
    std::string get_custom_instructions()
    {
        return kernel->get_gates_definition();
    }
    void display()
    {
        kernel->display();
    }

    void gate(std::string name, std::vector<size_t> qubits, 
        size_t duration=0, double angle=0.0)
    {
        kernel->gate(name, qubits, {}, duration, angle);
    }

    void gate(std::string name, std::vector<size_t> qubits, CReg & destination)
    {
        kernel->gate(name, qubits, {(destination.creg)->id} );
    }

    void classical(CReg & destination, Operation& operation)
    {
        kernel->classical(*(destination.creg), *(operation.operation));
    }

    void classical(std::string operation)
    {
        kernel->classical(operation);
    }

    void controlled(Kernel &k,
        std::vector<size_t> control_qubits,
        std::vector<size_t> ancilla_qubits)
    {
        kernel->controlled(k.kernel, control_qubits, ancilla_qubits);
    }

    void conjugate(Kernel &k)
    {
        kernel->conjugate(k.kernel);
    }

    ~Kernel()
    {
        delete(kernel);
    }
};


/**
 * quantum program interface
 */
class Program
{
public:
    std::string name;
    Platform platform;
    size_t qubit_count;
    size_t creg_count;
    ql::quantum_program *program;

    Program() {}

    Program(std::string name, Platform & platform, size_t qubit_count, size_t creg_count=0):
        name(name), platform(platform), qubit_count(qubit_count), creg_count(creg_count)
    {
        program = new ql::quantum_program(name, *(platform.platform), qubit_count, creg_count);
    }

    void set_sweep_points(std::vector<float> sweep_points, size_t num_sweep_points)
    {
        WOUT("This will soon be deprecated in favor of set_sweep_points(sweep_points)");
        float* sp = &sweep_points[0];
        program->set_sweep_points(sp, num_sweep_points);
    }

    void set_sweep_points(std::vector<float> sweep_points)
    {
        program->sweep_points = sweep_points;
    }

    std::vector<float> get_sweep_points()
    {
        return program->sweep_points;
    }

    void add_kernel(Kernel& k)
    {
        program->add( *(k.kernel));
    }

    void add_program(Program& p)
    {
        program->add_program(*(p.program));
    }

    void add_if(Kernel& k, Operation& operation)
    {
        program->add_if( *(k.kernel), *(operation.operation));
    }

    void add_if(Program& p, Operation& operation)
    {
        program->add_if( *(p.program), *(operation.operation));
    }

    void add_if_else(Kernel& k_if, Kernel& k_else, Operation& operation)
    {
        program->add_if_else( *(k_if.kernel), *(k_else.kernel), *(operation.operation));
    }

    void add_if_else(Program& p_if, Program& p_else, Operation& operation)
    {
        program->add_if_else( *(p_if.program), *(p_else.program), *(operation.operation));
    }

    void add_do_while(Kernel& k, Operation& operation)
    {
        program->add_do_while( *(k.kernel), *(operation.operation));
    }

    void add_do_while(Program& p, Operation& operation)
    {
        program->add_do_while( *(p.program), *(operation.operation));
    }

    void add_for(Kernel& k, size_t iterations)
    {
        program->add_for( *(k.kernel), iterations);
    }

    void add_for(Program& p, size_t iterations)
    {
        program->add_for( *(p.program), iterations);
    }

    void compile()
    {
        program->compile();
    }

    std::string qasm()
    {
        return program->qasm();
    }

    std::string microcode()
    {
        return program->microcode();
    }

    void print_interaction_matrix()
    {
        program->print_interaction_matrix();
    }

    void write_interaction_matrix()
    {
        program->write_interaction_matrix();
    }

    ~Program()
    {
        // std::cout << "program::~program()" << std::endl;
        delete(program);
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
