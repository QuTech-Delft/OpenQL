/**
 * @file   gate.h
 * @date   11/2016
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  gates implementation
 */

#ifndef GATE_H
#define GATE_H

#include <fstream>
#include <iomanip>
#include <complex>

#include <string>
#include <sstream>
#include <map>

#include <compile_options.h>
#include <matrix.h>
#include <json.h>
#include <exception.h>
#include <utils.h>

using json = nlohmann::json;

typedef std::string instruction_t;

namespace ql
{
typedef std::string string_t;

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
    __classical_gate__
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
class gate
{
public:
    std::string name = "";
    std::vector<size_t> operands;
    std::vector<size_t> creg_operands;
    int int_operand;
    size_t duration;
    double angle;                            // for arbitrary rotations
    size_t  cycle = MAX_CYCLE;               // cycle after scheduling; MAX_CYCLE indicates undefined
    virtual instruction_t qasm()       = 0;
    virtual gate_type_t   type()       = 0;
    virtual cmat_t        mat()        = 0;  // to do : change cmat_t type to avoid stack smashing on 2 qubits gate operations
};


/****************************************************************************\
| Standard gates
\****************************************************************************/

/**
 * identity
 */
class identity : public gate
{
public:
    cmat_t m;
    identity(size_t q) : m(identity_c)
    {
        name = "i";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("i q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __identity_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * hadamard
 */
class hadamard : public gate
{
public:
    cmat_t m;
    hadamard(size_t q) : m(hadamard_c)
    {
        name = "h";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("h q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __hadamard_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * phase
 */
class phase : public gate
{
public:
    cmat_t m;

    phase(size_t q) : m(phase_c)
    {
        name = "s";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("s q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __phase_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * phase dag
 */
class phasedag : public gate
{
public:
    cmat_t m;

    phasedag(size_t q) : m(phasedag_c)
    {
        name = "sdag";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("sdag q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __phasedag_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * rx
 */
class rx : public gate
{
public:
    cmat_t m;

    rx(size_t q, double theta)
    {
        name = "rx";
        duration = 40;
        angle = theta;
        operands.push_back(q);
        m(0,0) = cos(angle/2);
        m(0,1) = complex_t(0,-sin(angle/2));
        m(1,0) = complex_t(0,-sin(angle/2));
        m(1,1) = cos(angle/2);
    }

    instruction_t qasm()
    {
        return instruction_t("rx q[" + std::to_string(operands[0]) + "], " + std::to_string(angle) );
    }

    gate_type_t type()
    {
        return __rx_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * ry
 */
class ry : public gate
{
public:
    cmat_t m;

    ry(size_t q, double theta)
    {
        name = "ry";
        duration = 40;
        angle = theta;
        operands.push_back(q);
        m(0,0) = cos(angle/2);
        m(0,1) = -sin(angle/2);
        m(1,0) = sin(angle/2);
        m(1,1) = cos(angle/2);
    }

    instruction_t qasm()
    {
        return instruction_t("ry q[" + std::to_string(operands[0]) + "], " + std::to_string(angle) );
    }

    gate_type_t type()
    {
        return __ry_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * rz
 */
class rz : public gate
{
public:
    cmat_t m;

    rz(size_t q, double theta)
    {
        name = "rz";
        duration = 40;
        angle = theta;
        operands.push_back(q);
        m(0,0) = complex_t(cos(-angle/2), sin(-angle/2));
        m(0,1) = 0;
        m(1,0) = 0;
        m(1,1) =  complex_t(cos(angle/2), sin(angle/2));
    }

    instruction_t qasm()
    {
        return instruction_t("rz q[" + std::to_string(operands[0]) + "], " + std::to_string(angle) );
    }

    gate_type_t type()
    {
        return __rz_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};



/**
 * T
 */
class t : public gate
{
public:
    cmat_t m;

    t(size_t q) : m(t_c)
    {
        name = "t";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("t q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __t_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * T
 */
class tdag : public gate
{
public:
    cmat_t m;

    tdag(size_t q) : m(tdag_c)
    {
        name = "tdag";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("tdag q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __tdag_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * pauli_x
 */
class pauli_x : public gate

{
public:
    cmat_t m;

    pauli_x(size_t q) : m(pauli_x_c)
    {
        name = "x";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("x q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __pauli_x_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * pauli_y
 */
class pauli_y : public gate
{
public:
    cmat_t m;

    pauli_y(size_t q) : m(pauli_y_c)
    {
        name = "y";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("y q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __pauli_y_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * pauli_z
 */
class pauli_z : public gate
{
public:
    cmat_t m;

    pauli_z(size_t q) : m(pauli_z_c)
    {
        name = "z";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("z q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __pauli_z_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * rx90
 */
class rx90 : public gate
{
public:
    cmat_t m;

    rx90(size_t q) : m(rx90_c)
    {
        name = "x90";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("x90 q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __rx90_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * mrx90
 */
class mrx90 : public gate
{
public:
    cmat_t m;

    mrx90(size_t q) : m(mrx90_c)
    {
        name = "mx90";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("mx90 q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __mrx90_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * rx180
 */
class rx180 : public gate
{
public:
    cmat_t m;

    rx180(size_t q) : m(rx180_c)
    {
        name = "x180";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("x180 q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __rx180_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * ry90
 */
class ry90 : public gate
{
public:
    cmat_t m;

    ry90(size_t q) : m(ry90_c)
    {
        name = "y90";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("y90 q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __ry90_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * mry90
 */
class mry90 : public gate
{
public:
    cmat_t m;

    mry90(size_t q) : m(mry90_c)
    {
        name = "my90";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("my90 q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __mry90_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * ry180
 */
class ry180 : public gate
{
public:
    cmat_t m;

    ry180(size_t q) : m(ry180_c)
    {
        name = "y180";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("y180 q[" + std::to_string(operands[0]) + "]");
    }

    gate_type_t type()
    {
        return __ry180_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * measure
 */
class measure : public gate
{
public:
    cmat_t m;

    measure(size_t q) : m(identity_c)
    {
        name = "measure";
        duration = 40;
        operands.push_back(q);
    }

    measure(size_t q, size_t c) : m(identity_c)
    {
        name = "measure";
        duration = 40;
        operands.push_back(q);
        creg_operands.push_back(c);
    }

    instruction_t qasm()
    {
        std::stringstream ss;
        ss << "measure ";
        ss << "q[" << operands[0] << "]";
        if(!creg_operands.empty())
            ss << ", r[" << creg_operands[0] << "]";

        return instruction_t(ss.str());
    }

    gate_type_t type()
    {
        return __measure_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * prep_z
 */
class prepz : public gate
{
public:
    cmat_t m;

    prepz(size_t q) : m(identity_c)
    {
        name = "prep_z";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("prep_z q[" + std::to_string(operands[0]) +"]");
    }

    gate_type_t type()
    {
        return __prepz_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * cnot
 */
class cnot : public gate
{
public:
    cmat_t m;

    cnot(size_t q1, size_t q2) : m(cnot_c)
    {
        name = "cnot";
        duration = 80;
        operands.push_back(q1);
        operands.push_back(q2);
    }

    instruction_t qasm()
    {
        return instruction_t("cnot q[" + std::to_string(operands[0]) + "]"
                             + ",q["  + std::to_string(operands[1]) + "]");
    }

    gate_type_t type()
    {
        return __cnot_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * cphase
 */
class cphase : public gate
{
public:
    cmat_t m;

    cphase(size_t q1, size_t q2) : m(cphase_c)
    {
        name = "cz";
        duration = 80;
        operands.push_back(q1);
        operands.push_back(q2);
    }

    instruction_t qasm()
    {
        return instruction_t("cz q[" + std::to_string(operands[0]) + "]"
                             + ",q["  + std::to_string(operands[1]) + "]" );
    }

    gate_type_t type()
    {
        return __cphase_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

/**
 * toffoli
 */
class toffoli : public gate
{
public:
    cmat_t m;

    toffoli(size_t q1, size_t q2, size_t q3) : m(ctoffoli_c)
    {
        name = "toffoli";
        duration = 160;
        operands.push_back(q1);
        operands.push_back(q2);
        operands.push_back(q3);
    }

    instruction_t qasm()
    {
        return instruction_t("toffoli q[" + std::to_string(operands[0]) + "]"
                             + ",q["  + std::to_string(operands[1]) + "]"
                             + ",q["  + std::to_string(operands[2]) + "]");
    }

    gate_type_t type()
    {
        return __toffoli_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

class nop : public gate
{
public:
    cmat_t m;

    nop() : m(nop_c)
    {
        name = "wait";
        duration = 20;
    }

    instruction_t qasm()
    {
        return instruction_t("nop");
    }

    gate_type_t type()
    {
        return __nop_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


class swap : public gate
{
public:
    cmat_t m;

    swap(size_t q1, size_t q2) : m(swap_c)
    {
        name = "swap";
        duration = 80;
        operands.push_back(q1);
        operands.push_back(q2);
    }

    instruction_t qasm()
    {
        return instruction_t("swap q[" + std::to_string(operands[0]) + "]"
                             + ",q["  + std::to_string(operands[1]) + "]");
    }

    gate_type_t type()
    {
        return __swap_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/****************************************************************************\
| Special gates
\****************************************************************************/

class wait : public gate
{
public:
    cmat_t m;
    size_t duration_in_cycles;

    wait(std::vector<size_t> qubits, size_t d, size_t dc) : m(nop_c)
    {
        name = "wait";
        duration = d;
        duration_in_cycles = dc;
        for(auto & q : qubits)
        {
            operands.push_back(q);
        }
    }

    instruction_t qasm()
    {
        return instruction_t("wait " + std::to_string(duration_in_cycles));
    }

    gate_type_t type()
    {
        return __wait_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

class SOURCE : public gate
{
public:
    cmat_t m;

    SOURCE() : m(nop_c)
    {
        name = "SOURCE";
        duration = 1;
    }

    instruction_t qasm()
    {
        return instruction_t("SOURCE");
    }

    gate_type_t type()
    {
        return __dummy_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

class SINK : public gate
{
public:
    cmat_t m;

    SINK() : m(nop_c)
    {
        name = "SINK";
        duration = 1;
    }

    instruction_t qasm()
    {
        return instruction_t("SINK");
    }

    gate_type_t type()
    {
        return __dummy_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

class display : public gate
{
public:
    cmat_t m;

    display() : m(nop_c)
    {
        name = "display";
        duration = 0;
    }

    instruction_t qasm()
    {
        return instruction_t("display");
    }

    gate_type_t type()
    {
        return __display__;
    }

    cmat_t mat()
    {
        return m;
    }
};


/**
 * custom gate support
 */
// FIXME: move to separate file
class custom_gate : public gate
{
public:
    cmat_t              m;                // matrix representation
    std::string         arch_operation_name;  // name of instruction in the architecture (e.g. cc_light_instr)

public:

    /**
     * ctor
     */
    custom_gate(string_t name)
    {
        DOUT("Custom gate constructor for " << name);
        this->name = name;
    }

    /**
     * copy ctor
     */
    custom_gate(const custom_gate& g)
    {
        DOUT("Custom gate copy constructor for " << g.name);
        name = g.name;
        creg_operands = g.creg_operands;
        duration  = g.duration;
        m.m[0] = g.m.m[0];
        m.m[1] = g.m.m[1];
        m.m[2] = g.m.m[2];
        m.m[3] = g.m.m[3];
    }


#if 0    // FIXME: unused, but see comment in hardware_configuration.h::load_instruction
    /**
     * load instruction from json map
     */
    custom_gate(std::string& name, json& instr)
    {
        DOUT("Custom gate load from json map for " << name);
        this->name = name;
        load(instr);
    }
#endif

    /**
     * match qubit id
     */
    bool is_qubit_id(std::string& str)
    {
        if (str[0] != 'q')
            return false;
        size_t l = str.length();
        if (l>=1)
        {
            for (size_t i=1; i<l; ++i)
                if (!str::is_digit(str[i]))
                    return false;
        }
        return true;
    }

    /**
     * return qubit id
     */
    size_t qubit_id(std::string qubit)
    {
        std::string id = qubit.substr(1);
        return (atoi(id.c_str()));
    }

    /**
     * load instruction from json map
     */
    void load(json& instr)
    {
        DOUT("loading instruction '" << name << "'...");
        std::string l_attr = "(none)";
        try
        {
            l_attr = "qubits";
            DOUT("qubits: " << instr["qubits"]);
            size_t parameters = instr["qubits"].size();
            for (size_t i=0; i<parameters; ++i)
            {
                std::string qid = instr["qubits"][i];
                if (!is_qubit_id(qid))
                {
                    EOUT("invalid qubit id in attribute 'qubits' !");
                    throw ql::exception("[x] error : ql::custom_gate() : error while loading instruction '" + name + "' : attribute 'qubits' : invalid qubit id !", false);
                }
                operands.push_back(qubit_id(qid));
            }
            l_attr = "duration";
            duration = instr["duration"];
            DOUT("duration: " << instr["duration"]);
            l_attr = "matrix";
            // FIXME: make matrix optional, default to NaN
            auto mat = instr["matrix"];
            DOUT("matrix: " << instr["matrix"]);
            m.m[0] = complex_t(mat[0][0], mat[0][1]);
            m.m[1] = complex_t(mat[1][0], mat[1][1]);
            m.m[2] = complex_t(mat[2][0], mat[2][1]);
            m.m[3] = complex_t(mat[3][0], mat[3][1]);
        }
        catch (json::exception &e)
        {
            EOUT("while loading instruction '" << name << "' (attr: " << l_attr << ") : " << e.what());
            throw ql::exception("[x] error : ql::custom_gate() : error while loading instruction '" + name + "' : attribute '" + l_attr + "' : \n\t" + e.what(), false);
        }

        if ( instr.count("cc_light_instr") > 0)
        {
            arch_operation_name = instr["cc_light_instr"].get<std::string>();
            DOUT("cc_light_instr: " << instr["cc_light_instr"]);
        }
    }

    void print_info()
    {
        println("[-] custom gate : ");
        println("    |- name     : " << name);
        utils::print_vector(operands,"[openql]     |- qubits   :"," , ");
        println("    |- duration : " << duration);
        println("    |- matrix   : [" << m.m[0] << ", " << m.m[1] << ", " << m.m[2] << ", " << m.m[3] << "]");
    }

    /**
     * qasm output
     */
    instruction_t qasm()
    {
        std::stringstream ss;
        size_t p = name.find(" ");
        std::string gate_name = name.substr(0,p);
        if (operands.size() == 0)
            ss << gate_name;
        else if (operands.size() == 1)
            ss << gate_name << " q[" << operands[0] << "]";
        else
        {
            ss << gate_name << " q[" << operands[0] << "]";
            for (size_t i=1; i<operands.size(); i++)
                ss << ",q[" << operands[i] << "]";
        }

        // deal with custom gates with argument, such as angle
        if(gate_name == "rx" || gate_name == "ry" || gate_name == "rz")
        {
            ss << ", " << angle;
        }

        if(creg_operands.size() == 0)
        {

        }
        else if(creg_operands.size() == 1)
        {
            ss << ",r" << creg_operands[0];
        }
        else
        {
            ss << ",r" << creg_operands[0];
            for (size_t i=1; i<creg_operands.size(); i++)
                ss << ",r" << creg_operands[i];
        }

        return instruction_t(ss.str());
    }

    /**
     * type
     */
    gate_type_t type()
    {
        return __custom_gate__;
    }

    /**
     * matrix
     */
    cmat_t mat()
    {
        return m;
    }

};

/**
 * composite gate
 */
class composite_gate : public custom_gate
{
public:
    cmat_t m;
    std::vector<gate *> gs;

    composite_gate(std::string name) : custom_gate(name)
    {
        duration = 0;
    }

    composite_gate(std::string name, std::vector<gate *> seq) : custom_gate(name)
    {
        duration = 0;
        for (gate * g : seq)
        {
            gs.push_back(g);
            duration += g->duration;
            operands.insert(operands.end(), g->operands.begin(), g->operands.end());
        }
    }

    instruction_t qasm()
    {
        std::stringstream instr;
        for (gate * g : gs)
            instr << g->qasm() << "\n";
        return instruction_t(instr.str());
    }

    gate_type_t type()
    {
        return __composite_gate__;
    }

    cmat_t mat()
    {
        return m;
    }
};

} // end ql namespace

#endif // GATE_H
