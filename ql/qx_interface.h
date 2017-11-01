/**
 * @file    qx_interface.h
 * @author	Nader Khammassi 
 * @date	   20-12-15
 * @brief	provide a dummy qx interface
 *          to ease qx code reuse
 */

#ifndef QX_INTERFACE
#define QX_INTERFACE

#include <ql/openql.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <map>

#include <stdint.h>

#include <ql/str.h>
#include <ql/quantum_state_loader.h>


#ifndef println
#define println(x) std::cout << x << std::endl
#endif

// #include <core/circuit.h>
// #include <core/error_model.h>

// #include <quantum_code/token.h>
// #include <quantum_code/syntax_error_exception.h>

/**
 * qx wrapper
 */
namespace qx
{
   /**
    * error_model_t
    */
   typedef enum __error_model_t
   {
      __depolarizing_channel__,
      __amplitude_phase_damping__,
      __pauli_twirling__,
      __unknown_error_model__
   } error_model_t;

   /**
    * gate implementation 
    */
   #define __define_qx_gate(g) \
               class g : public gate \
               { \
                 public: g(...)\
                 {}\
               };

   /**
    * gate interface
    */
   class gate 
   { 
   };

   /**
    * definitions of supported gates
    */
   __define_qx_gate(prepz);
   __define_qx_gate(pauli_x);
   __define_qx_gate(pauli_y);
   __define_qx_gate(pauli_z);
   __define_qx_gate(rx);
   __define_qx_gate(ry);
   __define_qx_gate(rz);
   __define_qx_gate(hadamard);
   __define_qx_gate(phase);
   __define_qx_gate(phase_shift);
   __define_qx_gate(t_gate);
   __define_qx_gate(t_dag_gate);
   __define_qx_gate(cphase);
   __define_qx_gate(cnot);
   __define_qx_gate(swap);
   __define_qx_gate(ctrl_phase_shift);
   __define_qx_gate(toffoli);
   __define_qx_gate(measure);
   __define_qx_gate(display);
   __define_qx_gate(prepare);
   __define_qx_gate(bin_ctrl);
   __define_qx_gate(classical_not);
   
   // gates (availble only on the qx server version)
   // to do : implement them on the local qx simulator
   __define_qx_gate(qwait);
   __define_qx_gate(rx180);
   __define_qx_gate(ry180);
   __define_qx_gate(rz180);
   __define_qx_gate(rx90);
   __define_qx_gate(ry90);
   __define_qx_gate(rz90);
   __define_qx_gate(mrx180);
   __define_qx_gate(mry180);
   __define_qx_gate(mrz180);
   __define_qx_gate(mrx90);
   __define_qx_gate(mry90);
   __define_qx_gate(mrz90);

   /**
    * debug instruction
    */
   class print_str : public gate 
   { 
      public: 
        print_str(std::string s) 
        {
        } 
   };

   /**
    * parallel gates
    */
   class parallel_gates : public gate
   {
      public:
        
        std::vector<gate *> gates;

        /**
         * parallel gate
         */
        parallel_gates() 
        { 
        }

        /**
         * add gate
         */
        void add(gate * g)
        {
           gates.push_back(g);
        }
   };


   /**
    * circuit container
    */
   class circuit
   {
      public:

        size_t              num_qubits;
        size_t              iterations;
        std::string         name;
        std::vector<gate *> gates;

       /**
        * circuit
        */
        circuit(size_t num_qubits, std::string name="", size_t iterations=1) : num_qubits(num_qubits), name(name), iterations(iterations)
        {
        }

        /**
         * add gate circuit
         */
        void add(gate * g)
        {
           gates.push_back(g);
        }
   };

   // types definition

} // namespace qx


#endif // QX_INTERFACE
