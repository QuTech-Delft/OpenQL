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

#include <ql/matrix.h>
#include <ql/json.h>

#include <ql/openql.h>
#include <ql/exception.h>

using json = nlohmann::json;

typedef std::string instruction_t;

namespace ql
{

typedef std::string qasm_inst_t;
typedef std::string ucode_inst_t;

typedef std::string string_t;
typedef std::vector<std::string> strings_t;
typedef std::vector<std::string> ucode_sequence_t;

typedef enum
{
    flux_t,
    rf_t
} instruction_type_t;



typedef std::map<qasm_inst_t, ucode_inst_t> dep_instruction_map_t;

extern dep_instruction_map_t dep_instruction_map;

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
    __t_gate__         ,
    __tdag_gate__      ,
    __rx90_gate__      ,
    __mrx90_gate__     ,
    __rx180_gate__     ,
    __ry90_gate__      ,
    __mry90_gate__     ,
    __ry180_gate__     ,
    __rx_gate__        ,
    __ry_gate__        ,
    __rz_gate__        ,
    __prepz_gate__     ,
    __cnot_gate__      ,
    __cphase_gate__    ,
    __toffoli_gate__   ,
    __custom_gate__    ,
    __composite_gate__ ,
    __measure_gate__   ,
    __display__        ,
    __display_binary__ ,
    __nop_gate__
} gate_type_t;

#define sqrt_2  (1.4142135623730950488016887242096980785696718753769480731766797379f)
#define rsqrt_2 (0.7071067811865475244008443621048490392848359376884740365883398690f)

#define __c(r,i) complex_t(r,i)

const complex_t identity_c [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0) , __c(0.0, 0.0),
                                                                     __c(0.0, 0.0) , __c(1.0, 0.0)
                                                                   };     /* I */

const complex_t pauli_x_c  [] /* __attribute__((aligned(64))) */ = { __c(0.0, 0.0) , __c(1.0, 0.0),
                                                                     __c(1.0, 0.0) , __c(0.0, 0.0)
                                                                   };      /* X */

const complex_t pauli_y_c  [] /* __attribute__((aligned(64))) */ = { __c(0.0, 0.0) , __c(0.0,-1.0),
                                                                     __c(0.0, 1.0) , __c(0.0, 0.0)
                                                                   };      /* Y */

const complex_t pauli_z_c  [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0) , __c(0.0, 0.0),
                                                                     __c(0.0, 0.0) , __c(-1.0,0.0)
                                                                   };      /* Z */

const complex_t hadamard_c [] /* __attribute__((aligned(64))) */  = { rsqrt_2      ,  rsqrt_2,
                                                                      rsqrt_2      , -rsqrt_2
                                                                    };            /* H */

const complex_t phase_c    [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0) , __c(0.0, 0.0),
                                                                     __c(0.0, 0.0) , __c(0.0, 1.0)
                                                                   };        /* S */

const complex_t phasedag_c [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0) , __c(0.0, 0.0),
                                                                     __c(0.0, 0.0) , __c(0.0, -1.0)
                                                                   };        /* S */

const complex_t t_c    [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0) , __c(0.0, 0.0),
                                                                 __c(0.0, 0.0) , __c(0.707106781, 0.707106781)
                                                               };        /* T */

const complex_t tdag_c    [] /* __attribute__((aligned(64))) */ = { __c(1.0, 0.0) , __c(0.0, 0.0),
                                                                    __c(0.0, 0.0) , __c(0.707106781, -0.707106781)
                                                                  };        /* Tdag */

const complex_t rx90_c  [] /* __attribute__((aligned(64))) */ = { __c(rsqrt_2, 0.0) , __c(0.0, -rsqrt_2),
                                                                  __c(0.0, -rsqrt_2), __c(rsqrt_2,  0.0)
                                                                };   /* rx90  */

const complex_t ry90_c  [] /* __attribute__((aligned(64))) */ = { __c(rsqrt_2, 0.0) , __c(-rsqrt_2, 0.0),
                                                                  __c(rsqrt_2, 0.0 ), __c( rsqrt_2, 0.0)
                                                                };   /* ry90  */

const complex_t mrx90_c [] /* __attribute__((aligned(64))) */ = { __c(rsqrt_2, 0.0) , __c(0.0,  rsqrt_2),
                                                                  __c(0.0, rsqrt_2) , __c(rsqrt_2,  0.0)
                                                                };   /* mrx90 */

const complex_t mry90_c [] /* __attribute__((aligned(64))) */ = { __c(rsqrt_2, 0.0) , __c(rsqrt_2, 0.0),
                                                                  __c(-rsqrt_2, 0.0), __c(rsqrt_2, 0.0)
                                                                };   /* ry90  */

const complex_t rx180_c [] /* __attribute__((aligned(64))) */ = { __c(0.0, 0.0) , __c(0.0,-1.0),
                                                                  __c(0.0,-1.0) , __c(0.0, 0.0)
                                                                };   /* rx180 */

const complex_t ry180_c [] /* __attribute__((aligned(64))) */ = { __c(0.0, 0.0) , __c(-1.0, 0.0),
                                                                  __c(1.0, 0.0) , __c( 0.0, 0.0)
                                                                };   /* ry180 */

/**
 * to do : multi-qubit gates should not be represented by their matrix (the matrix depends on the ctrl/target qubit locations, the simulation using such matrix is inefficient as well...)
 */

const complex_t cnot_c [] /* __attribute__((aligned(64))) */ =
{
    __c(1.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0)
};  /* cnot  */

// TODO correct it, for now copied from cnot
const complex_t cphase_c [] /* __attribute__((aligned(64))) */ =
{
    __c(1.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(-1.0, 0.0)
}; /* cz */

// TODO correct it, for now copied from toffoli
const complex_t ctoffoli_c[] /* __attribute__((aligned(64))) */ =
{
    __c(1.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0) , __c(1.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(1.0, 0.0),
    __c(0.0, 0.0) , __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0), __c(0.0, 0.0) , __c(0.0, 0.0), __c(1.0, 0.0), __c(0.0, 0.0)
};

const complex_t nop_c      [] /*__attribute__((aligned(64)))*/ =
{
    __c(1.0, 0.0) , __c(0.0, 0.0),
    __c(0.0, 0.0) , __c(1.0, 0.0)
};

#undef __c


/**
 * gate interface
 */
class gate
{
public:
    bool optimization_enabled = true;
    std::string name = "";
    std::vector<size_t> operands;
    size_t duration;                          // to do change attribute name "duration" to "duration" (duration is used to describe hardware duration)
    virtual instruction_t qasm()       = 0;
    virtual instruction_t micro_code() = 0;  // to do : deprecated
    virtual gate_type_t   type()       = 0;
    virtual cmat_t        mat()        = 0;  // to do : change cmat_t type to avoid stack smashing on 2 qubits gate operations
};


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
        return instruction_t("i q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // TODO fix it
        return instruction_t("  pulse 1100 0000 1100\n     wait 10\n     pulse 1001 0000 1001\n     wait 10");
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
        return instruction_t("h q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // y90 + x180
        return instruction_t("  pulse 1100 0000 1100\n     wait 10\n     pulse 1001 0000 1001\n     wait 10");
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
        return instruction_t("s q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("  pulse 1110 0000 1110\n     wait 10");
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
        return instruction_t("sdag q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("  pulse 1110 0000 1110\n     wait 10");
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
    double angle;

    rx(size_t q, double angle) : angle(angle)
    {
        name = "rx";
        duration = 40;
        operands.push_back(q);
        m(0,0) = cos(angle/2);               m(0,1) = complex_t(0,-sin(angle/2));
        m(1,0) = complex_t(0,-sin(angle/2)); m(1,1) = cos(angle/2);
    }

    instruction_t qasm()
    {
        return instruction_t("rx q" + std::to_string(operands[0]) + std::to_string(angle) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("  pulse 1110 0000 1110\n     wait 10");
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
    double angle;

    ry(size_t q, double angle) : angle(angle)
    {
        name = "ry";
        duration = 40;
        operands.push_back(q);
   m(0,0) = cos(angle/2); m(0,1) = -sin(angle/2);
   m(1,0) = sin(angle/2); m(1,1) = cos(angle/2);
    }

    instruction_t qasm()
    {
        return instruction_t("ry q" + std::to_string(operands[0]) + std::to_string(angle) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("  pulse 1110 0000 1110\n     wait 10");
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
    double angle;

    rz(size_t q, double angle) : angle(angle)
    {
        name = "rz";
        duration = 40;
        operands.push_back(q);
   m(0,0) = complex_t(cos(-angle/2), sin(-angle/2));   m(0,1) = 0;
   m(1,0) = 0;  m(1,1) =  complex_t(cos(angle/2), sin(angle/2));
    }

    instruction_t qasm()
    {
        return instruction_t("rz q" + std::to_string(operands[0]) + std::to_string(angle) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("  pulse 1110 0000 1110\n     wait 10");
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
        return instruction_t("t q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("  pulse 1110 0000 1110\n     wait 10");
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
        return instruction_t("tdag q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("  pulse 1110 0000 1110\n     wait 10");
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
        return instruction_t("x q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // x180
        return instruction_t("  pulse 1001 0000 1001\n     wait 10");
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
        return instruction_t("y q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // y180
        return instruction_t("  pulse 1010 0000 1010\n     wait 10");
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
        return instruction_t("z q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // x180 + y180
        return instruction_t("  pulse 1001 0000 1001\n     wait 10\n     pulse 1010 0000 1010\n     wait 10");
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
        name = "rx90";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("rx90 q" + std::to_string(operands[0]));
    }

    instruction_t micro_code()
    {
        // return instruction_t("  pulse 1011 0000 1011\n     wait 10");
        return ql::dep_instruction_map["rx90"];
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
        return instruction_t("mrx90 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("  pulse 1101 0000 1101\n     wait 10");
        return ql::dep_instruction_map["mrx90"];
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
        return instruction_t("rx180 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("  pulse 1001 0000 1001\n     wait 10");
        return ql::dep_instruction_map["rx180"];
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
        name = "ry90";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("ry90 q" + std::to_string(operands[0]) );
    }

    gate_type_t type()
    {
        return __ry90_gate__;
    }

    instruction_t micro_code()
    {
        // return instruction_t("  pulse 1100 0000 1100\n     wait 10");
        return ql::dep_instruction_map["ry90"];
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
        return instruction_t("mry90 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("  pulse 1110 0000 1110\n     wait 10");
        return ql::dep_instruction_map["mry90"];
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
        name = "ry180";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("ry180 q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        // return instruction_t("  pulse 1010 0000 1010\n     wait 10");
        return ql::dep_instruction_map["ry180"];
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

    instruction_t qasm()
    {
        return instruction_t("measure q" + std::to_string(operands[0]) );
        // + "\n   display_binary\n");
    }

    instruction_t micro_code()
    {
        return instruction_t("  wait 60\n     pulse 0000 1111 1111\n     wait 50\n     measure\n");
        // return ql::dep_instruction_map["measure"];
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
        name = "prepz";
        duration = 40;
        operands.push_back(q);
    }

    instruction_t qasm()
    {
        return instruction_t("prepz q" + std::to_string(operands[0]) );
    }

    instruction_t micro_code()
    {
        return instruction_t("  waitreg r0\n     waitreg r0\n");
        // return ql::dep_instruction_map["prepz"];
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
        return instruction_t("cnot q" + std::to_string(operands[0])
                             + ",q"  + std::to_string(operands[1]) );
    }
    instruction_t micro_code()
    {
        return ql::dep_instruction_map["cnot"];
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
        return instruction_t("cz q" + std::to_string(operands[0])
                             + ",q"  + std::to_string(operands[1]) );
    }
    instruction_t micro_code()
    {
        return ql::dep_instruction_map["cz"];
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
        return instruction_t("toffoli q" + std::to_string(operands[0])
                             + ",q"  + std::to_string(operands[1])
                             + ",q"  + std::to_string(operands[2]) );
    }
    instruction_t micro_code()
    {
        return ql::dep_instruction_map["toffoli"];
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
        name = "nop";
        duration = 20;
    }
    instruction_t qasm()
    {
        return instruction_t("nop");
    }
    instruction_t micro_code()
    {
        return ql::dep_instruction_map["nop"];
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


/**
 * custom gate support
 */
class custom_gate : public gate
{
public:

    // string_t           name;             // qasm gate name
    cmat_t             m;                // matrix representation

    size_t             parameters;       // number of parameters : single qubit, two qubits ... etc

    ucode_sequence_t   qumis;            // microcode sequence
    instruction_type_t operation_type;   // operation type : rf/flux
    strings_t          used_hardware;    // used hardware

public:

    /**
     * ctor
     */
    custom_gate(string_t name)
    {
        this->name = name;
    }

    /**
     * copy ctor
     */
    custom_gate(const custom_gate& g)
    {
        name = g.name;
        parameters = g.parameters;
        qumis.assign(g.qumis.begin(), g.qumis.end());
        operation_type = g.operation_type;
        duration  = g.duration;
        used_hardware.assign(g.used_hardware.begin(), g.used_hardware.end());
        m.m[0] = g.m.m[0];
        m.m[1] = g.m.m[1];
        m.m[2] = g.m.m[2];
        m.m[3] = g.m.m[3];
    }

    /**
     * explicit ctor
     */
    custom_gate(string_t& name, cmat_t& m,
                size_t parameters, size_t duration, size_t latency,
                instruction_type_t& operation_type, ucode_sequence_t& qumis, strings_t hardware) : m(m),
        parameters(parameters),
        qumis(qumis), operation_type(operation_type)
    {
        this->name = name;
        this->duration = duration;
        for (size_t i=0; i<hardware.size(); i++)
            used_hardware.push_back(hardware[i]);
    }

    /**
     * load from json
     */
    custom_gate(string_t name, string_t& file_name)
    {
        this->name = name;
        std::ifstream f(file_name);
        if (f.is_open())
        {
            json instr;
            f >> instr;
            load(instr);
            f.close();
        }
        else std::cout << "[x] error : json file not found !" << std::endl;
    }

    /**
     * load instruction from json map
     */
    custom_gate(std::string& name, json& instr)
    {
        this->name = name;
        load(instr);
    }

    /**
     * match qubit id
     */
    bool is_qubit_id(std::string& str)
    {
        if (str[0] != 'q')
            return false;
        uint32_t l = str.length();
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
    void load(json& instr) throw (ql::exception)
    {
        // println("loading instruction '" << name << "'...");
        std::string l_attr = "qubits";
        try
        {
            l_attr = "qubits";
            // println("qubits: " << instr["qubits"]);
            parameters = instr["qubits"].size();
            for (size_t i=0; i<parameters; ++i)
            {
                std::string qid = instr["qubits"][i];
                if (!is_qubit_id(qid))
                {
                    println("[x] error : invalid qubit id in attribute 'qubits' !");
                    throw ql::exception("[x] error : ql::custom_gate() : error while loading instruction '" + name + "' : attribute 'qubits' : invalid qubit id !", false);
                }
                operands.push_back(qubit_id(qid));
            }
            // ucode_sequence_t ucs = instr["qumis"];
            // qumis.assign(ucs.begin(), ucs.end());
            // operation_type = instr["type"];
            l_attr = "duration";
            duration = instr["duration"];
            // strings_t hdw = instr["hardware"];
            // used_hardware.assign(hdw.begin(), hdw.end());
            l_attr = "matrix";
            auto mat = instr["matrix"];
            m.m[0] = complex_t(mat[0][0], mat[0][1]);
            m.m[1] = complex_t(mat[1][0], mat[1][1]);
            m.m[2] = complex_t(mat[2][0], mat[2][1]);
            m.m[3] = complex_t(mat[3][0], mat[3][1]);
        }
        catch (json::exception e)
        {
            println("[e] error while loading instruction '" << name << "' (attr: " << l_attr << ") : " << e.what());
            throw ql::exception("[x] error : ql::custom_gate() : error while loading instruction '" + name + "' : attribute '" + l_attr + "' : \n\t" + e.what(), false);
        }
    }

    void print_info()
    {
        println("[-] custom gate : ");
        println("    |- name     : " << name);
        println("    |- n_params : " << parameters);
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
        // ss << "   " << gate_name;
        else if (operands.size() == 1)
            ss << gate_name << " q" << operands[0];
        // ss << "   " << gate_name << " q" << operands[0];
        else
        {
            // ss << "   " << gate_name << " q" << operands[0];
            ss << gate_name << " q" << operands[0];
            for (size_t i=1; i<operands.size(); i++)
                ss << ",q" << operands[i];
        }
        return instruction_t(ss.str());
    }

    /**
     * microcode
     */
    instruction_t micro_code()
    {
        std::stringstream ss;
        for (size_t i=0; i<qumis.size(); i++)
            ss << "     " << qumis[i] << "\n";
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
    double angle;
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

    instruction_t micro_code()
    {
        // dummy !
        return instruction_t("");
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
