/**
 * @file   gate.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  gates implementation
 */

#ifndef GATE_H
#define GATE_H

#include <string>
#include <sstream>
#include <map>

#include "matrix.h"
#include <ql/openql.h>



typedef std::string instruction_t;

namespace ql
{

typedef std::string qasm_inst_t;
typedef std::string ucode_inst_t;

typedef std::map<qasm_inst_t, ucode_inst_t> instruction_map_t;

extern instruction_map_t instruction_map;

// gate types
typedef enum __gate_type_t
{
    __identity_gate__  ,
    __hadamard_gate__  ,
    __pauli_x_gate__   ,
    __pauli_y_gate__   ,
    __pauli_z_gate__   ,
    __phase_gate__     ,
    __phasedag_gate__  ,
    __rx90_gate__      ,
    __mrx90_gate__     ,
    __rx180_gate__     ,
    __ry90_gate__      ,
    __mry90_gate__     ,
    __ry180_gate__     ,
    __rz_gate__        ,
    __prepz_gate__     ,
    __cnot_gate__      ,
    __cphase_gate__    ,
    __toffoli_gate__    ,
    __measure_gate__   ,
    __display__        ,
    __display_binary__ ,
    __nop_gate__
} gate_type_t;

#define sqrt_2  (1.4142135623730950488016887242096980785696718753769480731766797379f)
#define rsqrt_2 (0.7071067811865475244008443621048490392848359376884740365883398690f)

const complex_t identity_c  [] __attribute__((aligned(64))) = { complex_t(1.0, 0.0) , complex_t(0.0, 0.0),
                                                                complex_t(0.0, 0.0) , complex_t(1.0, 0.0)
                                                              };     /* I */

const complex_t pauli_x_c  [] __attribute__((aligned(64))) = { complex_t(0.0, 0.0) , complex_t(1.0, 0.0),
                                                               complex_t(1.0, 0.0) , complex_t(0.0, 0.0)
                                                             };      /* X */

const complex_t pauli_y_c  [] __attribute__((aligned(64))) = { complex_t(0.0, 0.0) , complex_t(0.0,-1.0),
                                                               complex_t(0.0, 1.0) , complex_t(0.0, 0.0)
                                                             };      /* Y */

const complex_t pauli_z_c  [] __attribute__((aligned(64))) = { complex_t(1.0, 0.0) , complex_t(0.0, 0.0),
                                                               complex_t(0.0, 0.0) , complex_t(-1.0,0.0)
                                                             };      /* Z */

const complex_t hadamard_c [] __attribute__((aligned(64)))  = { rsqrt_2,  rsqrt_2, rsqrt_2, -rsqrt_2 };            /* H */

const complex_t phase_c    [] __attribute__((aligned(64))) = { complex_t(1.0, 0.0) , complex_t(0.0, 0.0),
                                                               complex_t(0.0, 0.0) , complex_t(0.0, 1.0)
                                                             };        /* S */

const complex_t phasedag_c    [] __attribute__((aligned(64))) = { complex_t(1.0, 0.0) , complex_t(0.0, 0.0),
                                                            complex_t(0.0, 0.0) , complex_t(0.0, -1.0)
                                                          };        /* S */

const complex_t rx90_c  [] __attribute__((aligned(64))) = { complex_t(rsqrt_2, 0.0) , complex_t(0.0, -rsqrt_2),
                                                            complex_t(0.0, -rsqrt_2), complex_t(rsqrt_2,  0.0)
                                                          };   /* rx90  */

const complex_t ry90_c  [] __attribute__((aligned(64))) = { complex_t(rsqrt_2, 0.0) , complex_t(-rsqrt_2, 0.0),
                                                            complex_t(rsqrt_2, 0.0 ), complex_t( rsqrt_2, 0.0)
                                                          };   /* ry90  */

const complex_t mrx90_c [] __attribute__((aligned(64))) = { complex_t(rsqrt_2, 0.0) , complex_t(0.0,  rsqrt_2),
                                                            complex_t(0.0, rsqrt_2) , complex_t(rsqrt_2,  0.0)
                                                          };   /* mrx90 */

const complex_t mry90_c [] __attribute__((aligned(64))) = { complex_t(rsqrt_2, 0.0) , complex_t(rsqrt_2, 0.0),
                                                            complex_t(-rsqrt_2, 0.0), complex_t(rsqrt_2, 0.0)
                                                          };   /* ry90  */

const complex_t rx180_c [] __attribute__((aligned(64))) = { complex_t(0.0, 0.0) , complex_t(0.0,-1.0),
                                                            complex_t(0.0,-1.0) , complex_t(0.0, 0.0)
                                                          };            /* rx180 */

const complex_t ry180_c [] __attribute__((aligned(64))) = { complex_t(0.0, 0.0) , complex_t(-1.0, 0.0),
                                                            complex_t(1.0, 0.0) , complex_t( 0.0, 0.0)
                                                          };            /* ry180 */

const complex_t cnot_c [] __attribute__((aligned(64))) =
{
    complex_t(1.0, 0.0) , complex_t(0.0, 0.0), complex_t(0.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(1.0, 0.0), complex_t(0.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(0.0, 0.0), complex_t(1.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(0.0, 0.0), complex_t(0.0, 0.0), complex_t(1.0, 0.0)
};

// TODO correct it, for now copied from cnot
const complex_t cphase_c [] __attribute__((aligned(64))) =
{
    complex_t(1.0, 0.0) , complex_t(0.0, 0.0), complex_t(0.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(1.0, 0.0), complex_t(0.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(0.0, 0.0), complex_t(1.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(0.0, 0.0), complex_t(0.0, 0.0), complex_t(1.0, 0.0)
};

// TODO correct it, for now copied from toffoli
const complex_t ctoffoli_c [] __attribute__((aligned(64))) =
{
    complex_t(1.0, 0.0) , complex_t(0.0, 0.0), complex_t(0.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(1.0, 0.0), complex_t(0.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(0.0, 0.0), complex_t(1.0, 0.0), complex_t(0.0, 0.0),
    complex_t(0.0, 0.0) , complex_t(0.0, 0.0), complex_t(0.0, 0.0), complex_t(1.0, 0.0)
};

const complex_t nop_c  [] __attribute__((aligned(64))) = { complex_t(0.0, 0.0) , complex_t(0.0, 0.0),
                                                             complex_t(0.0, 0.0) , complex_t(0.0, 0.0)
                                                           };
/**
 * gate interface
 */
class gate
{
public:
    std::vector<size_t> operands;
    size_t latency;
    virtual instruction_t qasm()       = 0;
    virtual instruction_t micro_code() = 0;
    virtual gate_type_t   type()       = 0;
    virtual cmat_t        mat()        = 0;
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
        latency = 2;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   h q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // y90 + x180
        return instruction_t("     pulse 1100 0000 1100\n     wait 10\n     pulse 1001 0000 1001\n     wait 10");
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
        latency = 3;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   s q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("     pulse 1110 0000 1110\n     wait 10");
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

class phasedag : public gate
{
public:
    cmat_t m;

    phasedag(size_t q) : m(phasedag_c)
    {
        latency = 3;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   sdag q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("     pulse 1110 0000 1110\n     wait 10");
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
 * pauli_x
 */
class pauli_x : public gate

{
public:
    cmat_t m;

    pauli_x(size_t q) : m(pauli_x_c)
    {
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   x q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // x180
        return instruction_t("     pulse 1001 0000 1001\n     wait 10");
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
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   y q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // y180
        return instruction_t("     pulse 1010 0000 1010\n     wait 10");
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
        latency = 2;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   z q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // x180 + y180
        return instruction_t("     pulse 1001 0000 1001\n     wait 10\n     pulse 1010 0000 1010\n     wait 10");
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
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   rx90 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("     pulse 1011 0000 1011\n     wait 10");
        return ql::instruction_map["rx90"];
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
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   mrx90 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("     pulse 1101 0000 1101\n     wait 10");
        return ql::instruction_map["mrx90"];
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
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   rx180 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("     pulse 1001 0000 1001\n     wait 10");
        return ql::instruction_map["rx180"];
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
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   ry90 q" + std::to_string(operands[0]) );
    }

    gate_type_t type()
    {
        return __ry90_gate__;
    }

    instruction_t micro_code()
    {
        // return instruction_t("     pulse 1100 0000 1100\n     wait 10");
        return ql::instruction_map["ry90"];
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
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   mry90 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("     pulse 1110 0000 1110\n     wait 10");
        return ql::instruction_map["mry90"];
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
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   ry180 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("     pulse 1010 0000 1010\n     wait 10");
        return ql::instruction_map["ry180"];
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
        latency = 4;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   measure q" + std::to_string(operands[0])
                             + "\n   display_binary\n");
    }

    instruction_t micro_code()
    {
        // return instruction_t("     wait 60\n     pulse 0000 1111 1111\n     wait 50\n     measure\n");
        return ql::instruction_map["measure"];
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
 * prepz
 */
class prepz : public gate
{
public:
    cmat_t m;

    prepz(size_t q) : m(identity_c)
    {
        latency = 1;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("   prepz q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("     waitreg r0\n     waitreg r0\n");
        return ql::instruction_map["prepz"];
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
        latency = 4;
        operands.push_back(q1);
        operands.push_back(q2);
    }
    instruction_t qasm()
    {
        return instruction_t("   cnot q" + std::to_string(operands[0])
                             + ", q"  + std::to_string(operands[1]) );
    }
    instruction_t micro_code()
    {
        return ql::instruction_map["cnot"];
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
        latency = 4;
        operands.push_back(q1);
        operands.push_back(q2);
    }
    instruction_t qasm()
    {
        return instruction_t("   cz q" + std::to_string(operands[0])
                             + ", q"  + std::to_string(operands[1]) );
    }
    instruction_t micro_code()
    {
        return ql::instruction_map["cz"];
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
 * cphase
 */
class toffoli : public gate
{
public:
    cmat_t m;

    toffoli(size_t q1, size_t q2, size_t q3) : m(ctoffoli_c)
    {
        latency = 6;
        operands.push_back(q1);
        operands.push_back(q2);
        operands.push_back(q3);
    }
    instruction_t qasm()
    {
        return instruction_t("   toffoli q" + std::to_string(operands[0])
                             + ", q"  + std::to_string(operands[1])
                             + ", q"  + std::to_string(operands[2]) );
    }
    instruction_t micro_code()
    {
        return ql::instruction_map["toffoli"];
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
        latency = 1;
    }
    instruction_t qasm()
    {
        return instruction_t("   NOP");
    }
    instruction_t micro_code()
    {
        return ql::instruction_map["nop"];
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

} // end ql namespace

#endif // GATE_H
