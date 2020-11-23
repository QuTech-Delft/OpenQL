/**
 * @file   gate.h
 * @date   11/2016
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  gates implementation
 */

#pragma once

#include <string>
#include <vector>
#include "json.h"
#include "matrix.h"
#include "utils.h"

namespace ql {

typedef std::string instruction_t;

// gate types
typedef enum __gate_type_t
{
    __identity_gate__,
    __hadamard_gate__,
    __pauli_x_gate__,
    __pauli_y_gate__,
    __pauli_z_gate__,
    __phase_gate__,
    __phasedag_gate__,
    __t_gate__,
    __tdag_gate__,
    __rx90_gate__,
    __mrx90_gate__,
    __rx180_gate__,
    __ry90_gate__,
    __mry90_gate__,
    __ry180_gate__,
    __rx_gate__,
    __ry_gate__,
    __rz_gate__,
    __prepz_gate__,
    __cnot_gate__,
    __cphase_gate__,
    __toffoli_gate__,
    __custom_gate__,
    __composite_gate__,
    __measure_gate__,
    __display__,
    __display_binary__,
    __nop_gate__,
    __dummy_gate__,
    __swap_gate__,
    __wait_gate__,
    __classical_gate__,
    __remap_gate__
} gate_type_t;

#define sqrt_2  (1.4142135623730950488016887242096980785696718753769480731766797379f)
#define rsqrt_2 (0.7071067811865475244008443621048490392848359376884740365883398690f)

#define __c(r,i) complex_t(r,i)

const complex_t identity_c [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0), __c(0.0, 0.0),
                                                                     __c(0.0, 0.0), __c(1.0, 0.0)
                                                                   };     /* I */

const complex_t pauli_x_c  [] /* __attribute__((aligned(64))) */ = { __c(0.0, 0.0), __c(1.0, 0.0),
                                                                     __c(1.0, 0.0), __c(0.0, 0.0)
                                                                   };      /* X */

const complex_t pauli_y_c  [] /* __attribute__((aligned(64))) */ = { __c(0.0, 0.0), __c(0.0,-1.0),
                                                                     __c(0.0, 1.0), __c(0.0, 0.0)
                                                                   };      /* Y */

const complex_t pauli_z_c  [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0), __c(0.0, 0.0),
                                                                     __c(0.0, 0.0), __c(-1.0,0.0)
                                                                   };      /* Z */

const complex_t hadamard_c [] /* __attribute__((aligned(64))) */  = { rsqrt_2,  rsqrt_2,
                                                                      rsqrt_2, -rsqrt_2
                                                                    };            /* H */

const complex_t phase_c    [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0), __c(0.0, 0.0),
                                                                     __c(0.0, 0.0), __c(0.0, 1.0)
                                                                   };        /* S */

const complex_t phasedag_c [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0), __c(0.0, 0.0),
                                                                     __c(0.0, 0.0), __c(0.0, -1.0)
                                                                   };        /* S */

const complex_t t_c    [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0), __c(0.0, 0.0),
                                                                 __c(0.0, 0.0), __c(0.707106781, 0.707106781)
                                                               };        /* T */

const complex_t tdag_c    [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0), __c(0.0, 0.0),
                                                                    __c(0.0, 0.0), __c(0.707106781, -0.707106781)
                                                                  };        /* Tdag */

const complex_t rx90_c  [] /* __attribute__((aligned(64))) */ = { __c(rsqrt_2, 0.0), __c(0.0, -rsqrt_2),
                                                                  __c(0.0, -rsqrt_2), __c(rsqrt_2,  0.0)
                                                                };   /* rx90  */

const complex_t ry90_c  [] /* __attribute__((aligned(64))) */ = { __c(rsqrt_2, 0.0), __c(-rsqrt_2, 0.0),
                                                                  __c(rsqrt_2, 0.0 ), __c( rsqrt_2, 0.0)
                                                                };   /* ry90  */

const complex_t mrx90_c [] /* __attribute__((aligned(64))) */ = { __c(rsqrt_2, 0.0), __c(0.0,  rsqrt_2),
                                                                  __c(0.0, rsqrt_2), __c(rsqrt_2,  0.0)
                                                                };   /* mrx90 */

const complex_t mry90_c [] /* __attribute__((aligned(64))) */ = { __c(rsqrt_2, 0.0), __c(rsqrt_2, 0.0),
                                                                  __c(-rsqrt_2, 0.0), __c(rsqrt_2, 0.0)
                                                                };   /* ry90  */

const complex_t rx180_c [] /* __attribute__((aligned(64))) */ = { __c(0.0, 0.0), __c(0.0,-1.0),
                                                                  __c(0.0,-1.0), __c(0.0, 0.0)
                                                                };   /* rx180 */

const complex_t ry180_c [] /* __attribute__((aligned(64))) */ = { __c(0.0, 0.0), __c(-1.0, 0.0),
                                                                  __c(1.0, 0.0), __c( 0.0, 0.0)
                                                                };   /* ry180 */

/**
 * to do : multi-qubit gates should not be represented by their matrix (the matrix depends on the ctrl/target qubit locations, the simulation using such matrix is inefficient as well...)
 */

const complex_t cnot_c [] /* __attribute__((aligned(64))) */ =
{
    __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0)
};  /* cnot  */

// TODO correct it, for now copied from cnot
const complex_t cphase_c [] /* __attribute__((aligned(64))) */ =
{
    __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(-1.0, 0.0)
}; /* cz */

const complex_t swap_c [] /* __attribute__((aligned(64))) */ =
{
    __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0)
};  /* swap  */

// TODO correct it, for now copied from toffoli
const complex_t ctoffoli_c[] /* __attribute__((aligned(64))) */ =
{
    __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0),
    __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0)
};

const complex_t nop_c      [] /*__attribute__((aligned(64)))*/ =
{
    __c(1.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0), __c(1.0, 0.0)
};

#undef __c




/**
 * gate interface
 */
class gate {
public:
    std::string name;
    std::vector<size_t> operands;
    std::vector<size_t> creg_operands;
    int int_operand = 0;
    size_t duration = 0;
    double angle = 0.0;                      // for arbitrary rotations
    size_t  cycle = MAX_CYCLE;               // cycle after scheduling; MAX_CYCLE indicates undefined
    virtual ~gate() = default;
    virtual instruction_t qasm() const = 0;
    virtual gate_type_t   type() const = 0;
    virtual cmat_t        mat()  const = 0;  // to do : change cmat_t type to avoid stack smashing on 2 qubits gate operations
};


/****************************************************************************\
| Standard gates
\****************************************************************************/

class identity : public gate {
public:
    cmat_t m;
    explicit identity(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class hadamard : public gate {
public:
    cmat_t m;
    explicit hadamard(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class phase : public gate {
public:
    cmat_t m;
    explicit phase(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class phasedag : public gate {
public:
    cmat_t m;
    explicit phasedag(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class rx : public gate {
public:
    cmat_t m;
    rx(size_t q, double theta);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class ry : public gate {
public:
    cmat_t m;
    ry(size_t q, double theta);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class rz : public gate {
public:
    cmat_t m;
    rz(size_t q, double theta);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class t : public gate {
public:
    cmat_t m;
    explicit t(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class tdag : public gate {
public:
    cmat_t m;
    explicit tdag(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class pauli_x : public gate {
public:
    cmat_t m;
    explicit pauli_x(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class pauli_y : public gate {
public:
    cmat_t m;
    explicit pauli_y(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class pauli_z : public gate {
public:
    cmat_t m;
    explicit pauli_z(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class rx90 : public gate {
public:
    cmat_t m;
    explicit rx90(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class mrx90 : public gate {
public:
    cmat_t m;
    explicit mrx90(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class rx180 : public gate {
public:
    cmat_t m;
    explicit rx180(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class ry90 : public gate {
public:
    cmat_t m;
    explicit ry90(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class mry90 : public gate {
public:
    cmat_t m;
    explicit mry90(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class ry180 : public gate {
public:
    cmat_t m;
    explicit ry180(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class measure : public gate {
public:
    cmat_t m;
    explicit measure(size_t q);
    measure(size_t q, size_t c);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class prepz : public gate {
public:
    cmat_t m;
    explicit prepz(size_t q);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class cnot : public gate {
public:
    cmat_t m;
    cnot(size_t q1, size_t q2);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class cphase : public gate {
public:
    cmat_t m;
    cphase(size_t q1, size_t q2);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class toffoli : public gate {
public:
    cmat_t m;
    toffoli(size_t q1, size_t q2, size_t q3);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class nop : public gate {
public:
    cmat_t m;
    nop();
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class swap : public gate {
public:
    cmat_t m;
    swap(size_t q1, size_t q2);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

/****************************************************************************\
| Special gates
\****************************************************************************/

class wait : public gate {
public:
    cmat_t m;
    size_t duration_in_cycles;

    wait(std::vector<size_t> qubits, size_t d, size_t dc);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class SOURCE : public gate {
public:
    cmat_t m;
    SOURCE();
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class SINK : public gate {
public:
    cmat_t m;
    SINK();
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class display : public gate {
public:
    cmat_t m;
    display();
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class custom_gate : public gate {
public:
    cmat_t              m;                // matrix representation
    std::string         arch_operation_name;  // name of instruction in the architecture (e.g. cc_light_instr)
    explicit custom_gate(const std::string &name);
    custom_gate(const custom_gate &g);
    static bool is_qubit_id(const std::string &str);
    static size_t qubit_id(const std::string &qubit);
    void load(nlohmann::json &instr);
    void print_info() const;
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class composite_gate : public custom_gate {
public:
    cmat_t m;
    std::vector<gate *> gs;
    explicit composite_gate(const std::string &name);
    composite_gate(const std::string &name, const std::vector<gate*> &seq);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

class remap : public gate {
    cmat_t m;
    size_t virtual_qubit_index;
    remap(const size_t r_index, const size_t v_index);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

} // namespace ql
