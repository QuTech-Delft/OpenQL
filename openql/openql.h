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
    std::string            name_;
    std::string            config_file_;
    ql::quantum_platform * platform_;

    Platform() {}
    Platform(std::string name, std::string config_file) : name_(name), config_file_(config_file)
    {
        platform_ = new ql::quantum_platform(name_, config_file_);
    }
    size_t get_qubit_number()
    {
        return platform_->get_qubit_number();
    }
};

class CReg
{
public:
    ql::creg* creg_;
    CReg()
    {
        creg_ = new ql::creg();
    }
    ~CReg()
    {
        delete(creg_);
    }
};

class Operation
{
public:
    ql::operation * operation_;
    Operation(CReg& lop, std::string op, CReg& rop)
    {
        operation_ = new ql::operation(*(lop.creg_), op, *(rop.creg_));
    }
    Operation(std::string op, CReg& rop)
    {
        operation_ = new ql::operation(op, *(rop.creg_));
    }
    Operation(CReg& lop)
    {
        operation_ = new ql::operation(*(lop.creg_));
    }
    Operation(int val)
    {
        operation_ = new ql::operation(val);
    }
    ~Operation()
    {
        delete(operation_);
    }
};


/**
 * quantum kernel interface
 */
class Kernel
{
public:
    std::string name_;
    Platform platform_;    
    size_t qubit_count_;
    size_t creg_count_;
    ql::quantum_kernel * kernel_;

    Kernel(std::string kernel_name, Platform platform, size_t qubit_count, size_t creg_count=0):
        name_(kernel_name), platform_(platform), qubit_count_(qubit_count), creg_count_(creg_count)
    {
        kernel_ = new ql::quantum_kernel(name_, *(platform.platform_), qubit_count_, creg_count_);
    }
    void identity(size_t q0)
    {
        kernel_->identity(q0);
    }
    void hadamard(size_t q0)
    {
        kernel_->hadamard(q0);
    }
    void s(size_t q0)
    {
        kernel_->s(q0);
    }
    void sdag(size_t q0)
    {
        kernel_->sdag(q0);
    }
    void t(size_t q0)
    {
        kernel_->t(q0);
    }
    void tdag(size_t q0)
    {
        kernel_->tdag(q0);
    }
    void x(size_t q0)
    {
        kernel_->x(q0);
    }
    void y(size_t q0)
    {
        kernel_->y(q0);
    }
    void z(size_t q0)
    {
        kernel_->z(q0);
    }
    void rx90(size_t q0)
    {
        kernel_->rx90(q0);
    }
    void mrx90(size_t q0)
    {
        kernel_->mrx90(q0);
    }
    void rx180(size_t q0)
    {
        kernel_->rx180(q0);
    }
    void ry90(size_t q0)
    {
        kernel_->ry90(q0);
    }
    void mry90(size_t q0)
    {
        kernel_->mry90(q0);
    }
    void ry180(size_t q0)
    {
        kernel_->ry180(q0);
    }
    void rx(size_t q0, double angle)
    {
        kernel_->rx(q0, angle);
    }
    void ry(size_t q0, double angle)
    {
        kernel_->ry(q0, angle);
    }
    void rz(size_t q0, double angle)
    {
        kernel_->rz(q0, angle);
    }
    void measure(size_t q0)
    {
        kernel_->measure(q0);
    }
    void prepz(size_t q0)
    {
        kernel_->prepz(q0);
    }
    void cnot(size_t q0, size_t q1)
    {
        kernel_->cnot(q0,q1);
    }
    void cphase(size_t q0, size_t q1)
    {
        kernel_->cphase(q0,q1);
    }
    void cz(size_t q0, size_t q1)
    {
        kernel_->cz(q0,q1);
    }
    void toffoli(size_t q0, size_t q1, size_t q2)
    {
        kernel_->toffoli(q0,q1,q2);
    }
    void clifford(size_t id, size_t q0)
    {
        kernel_->clifford(id, q0);
    }
    void wait(std::vector<size_t> qubits, size_t duration)
    {
        kernel_->wait(qubits, duration);
    }
    void barrier(std::vector<size_t> qubits)
    {
        kernel_->wait(qubits, 0);
    }
    std::string get_custom_instructions()
    {
        return kernel_->get_gates_definition();
    }
    void display()
    {
        kernel_->display();
    }

    void gate(std::string name, std::vector<size_t> qubits, 
        size_t duration=0, double angle=0.0)
    {
        kernel_->gate(name, qubits, {}, duration, angle);
    }

    void gate(std::string name, std::vector<size_t> qubits, CReg & destination)
    {
        kernel_->gate(name, qubits, {(destination.creg_)->id} );
    }

    void classical(CReg & destination, Operation& operation)
    {
        kernel_->classical(*(destination.creg_), *(operation.operation_));
    }

    void classical(std::string operation)
    {
        kernel_->classical(operation);
    }

    void controlled(Kernel &k,
        std::vector<size_t> control_qubits,
        std::vector<size_t> ancilla_qubits)
    {
        kernel_->controlled(k.kernel_, control_qubits, ancilla_qubits);
    }

    void conjugate(Kernel &k)
    {
        kernel_->conjugate(k.kernel_);
    }

    ~Kernel()
    {
        delete(kernel_);
    }
};


/**
 * quantum program interface
 */
class Program
{
public:
    std::string name_;
    Platform platform_;
    size_t qubit_count_;
    size_t creg_count_;
    ql::quantum_program *program_;

    Program() {}

    Program(std::string prog_name, Platform & platform, size_t qubit_count, size_t creg_count=0):
        name_(prog_name), platform_(platform), qubit_count_(qubit_count), creg_count_(creg_count)
    {
        program_ = new ql::quantum_program(name_, *(platform_.platform_), qubit_count_, creg_count_);
    }

    void set_sweep_points( std::vector<float> sweep_points, size_t num_sweep_points)
    {
        float* sp = &sweep_points[0];
        program_->set_sweep_points(sp, num_sweep_points);
    }

    void add_kernel(Kernel& k)
    {
        program_->add( *(k.kernel_));
    }

    void add_program(Program& p)
    {
        program_->add_program(*(p.program_));
    }

    void add_if(Kernel& k, Operation& operation)
    {
        program_->add_if( *(k.kernel_), *(operation.operation_));
    }

    void add_if(Program& p, Operation& operation)
    {
        program_->add_if( *(p.program_), *(operation.operation_));
    }

    void add_if_else(Kernel& k_if, Kernel& k_else, Operation& operation)
    {
        program_->add_if_else( *(k_if.kernel_), *(k_else.kernel_), *(operation.operation_));
    }

    void add_if_else(Program& p_if, Program& p_else, Operation& operation)
    {
        program_->add_if_else( *(p_if.program_), *(p_else.program_), *(operation.operation_));
    }

    void add_do_while(Kernel& k, Operation& operation)
    {
        program_->add_do_while( *(k.kernel_), *(operation.operation_));
    }

    void add_do_while(Program& p, Operation& operation)
    {
        program_->add_do_while( *(p.program_), *(operation.operation_));
    }

    void add_for(Kernel& k, size_t iterations)
    {
        program_->add_for( *(k.kernel_), iterations);
    }

    void add_for(Program& p, size_t iterations)
    {
        program_->add_for( *(p.program_), iterations);
    }

    void compile()
    {
        program_->compile();
    }

    std::string qasm()
    {
        return program_->qasm();
    }

    std::string microcode()
    {
        return program_->microcode();
    }

    void print_interaction_matrix()
    {
        program_->print_interaction_matrix();
    }

    void write_interaction_matrix()
    {
        program_->write_interaction_matrix();
    }

    ~Program()
    {
        // std::cout << "program::~program()" << std::endl;
        delete(program_);
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
