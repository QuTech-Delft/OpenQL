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
#include <complex>

#include <version.h>
#include <openql.h>
#include <classical.h>
#include <unitary.h>

#include "compiler.h"


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

typedef std::complex<double> Complex;

/**
 * quantum unitary matrix interface
 */
class Unitary
{
public:
    std::string name;
    ql::unitary * unitary;

    Unitary(std::string name, std::vector<std::complex<double>> matrix) : name(name)
    {
        unitary = new ql::unitary(name, matrix);
    }

    void decompose()
    {
        unitary->decompose();
    }

    ~Unitary()
    {
        // destroy unitary
        delete(unitary);
    }

    static bool is_decompose_support_enabled() {
        return ql::unitary::is_decompose_support_enabled();
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
  
    Kernel(std::string name):
        name(name)
    {
        DOUT(" API::Kernel named: " << name);
        kernel = new ql::quantum_kernel(name);
    }
    
    Kernel(std::string name, Platform platform, size_t qubit_count, size_t creg_count=0):
        name(name), platform(platform), qubit_count(qubit_count), creg_count(creg_count)
    {
        WOUT("Kernel(name,Platform,#qbit,#creg) API will soon be deprecated according to issue #266 - OpenQL v0.9");
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
    void clifford(int id, size_t q0)
    {
        kernel->clifford(id, q0);
    }
    void wait(std::vector<size_t> qubits, size_t duration)
    {
        kernel->wait(qubits, duration);
    }
    void barrier(std::vector<size_t> qubits = std::vector<size_t>())
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

    void gate(Unitary &u, std::vector<size_t> qubits)
    {
        kernel->gate(*(u.unitary), qubits);
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

    Program(std::string name):
        name(name)
    {
        DOUT("SWIG Program(name) constructor for name: " << name);
        program = new ql::quantum_program(name);
    }
        
    Program(std::string name, Platform & platform, size_t qubit_count, size_t creg_count=0):
        name(name), platform(platform), qubit_count(qubit_count), creg_count(creg_count)
    { 
        WOUT("Program(name,Platform,#qbit,#creg) API will soon be deprecated according to issue #266 - OpenQL v0.9");
        program = new ql::quantum_program(name, *(platform.platform), qubit_count, creg_count);
    }
    
    void set_sweep_points(std::vector<float> sweep_points)
    {
        WOUT("This will soon be deprecated according to issue #76");
        program->sweep_points = sweep_points;
    }

    std::vector<float> get_sweep_points()
    {
        WOUT("This will soon be deprecated according to issue #76");
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
        //program->compile();

        program->compile_modular();
    }

    std::string microcode()
    {
#if OPT_MICRO_CODE
        return program->microcode();
#else
        return std::string("microcode disabled");
#endif
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
        // leave deletion to SWIG, otherwise the python unit test framework fails
        //delete(program);
    }
};

/**
 * cqasm reader interface
 */
class cQasmReader
{
public:
    ql::cqasm_reader *cqasm_reader_;
    Platform platform;
    Program program;

    cQasmReader(const Platform& q_platform, Program& q_program) :
        platform(q_platform), program(q_program)
    {
        cqasm_reader_ = new ql::cqasm_reader(*(platform.platform), *(program.program));
    }

    void string2circuit(std::string cqasm_str)
    {
        cqasm_reader_->string2circuit(cqasm_str);
    }

    void file2circuit(std::string cqasm_file_path)
    {
        cqasm_reader_->file2circuit(cqasm_file_path);
    }

    ~cQasmReader()
    {
        // leave deletion to SWIG, otherwise the python unit test framework fails
        // delete cqasm_reader_;
    }
};


/**
 * quantum compiler interface
 */
class Compiler
{
public:
    std::string           name;
    ql::quantum_compiler *compiler;
 
    Compiler(std::string name) : name(name) 
    {
        compiler = new ql::quantum_compiler(name);
    }
    
    void compile(Program program) 
    {
        DOUT(" Compiler " << name << " compiles program  " << program.name); 
        compiler->compile(program.program);
    }
    
    void add_pass_alias(std::string realPassName, std::string symbolicPassName)
    {
        DOUT(" Add pass " << realPassName << " under alias name  " << symbolicPassName); 
        compiler->addPass(realPassName,symbolicPassName);
    }
    
    void add_pass(std::string realPassName)
    {
        DOUT(" Add pass " << realPassName << " with no alias"); 
        compiler->addPass(realPassName);
    }

    void set_pass_option(std::string passName, std::string optionName, std::string optionValue)
    {
        DOUT(" Set option " << optionName << " = " << optionValue << " for pass " << passName); 
        compiler->setPassOption(passName,optionName, optionValue);
    }

};

#endif
