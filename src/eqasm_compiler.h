/**
 * @file   eqasm_compiler.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  executable qasm compiler interface
 */



#ifndef QL_EQASM_COMPILER_H
#define QL_EQASM_COMPILER_H

#include <fstream>

#include <program.h>
#include <platform.h>

typedef std::vector<std::string> eqasm_t;

namespace ql
{
    class quantum_platform;

    /**
    * eqasm compiler interface
    */
    class eqasm_compiler
    {
    public:
        eqasm_t eqasm_code;

    public:

        /*
	     * compile must be implemented by all compilation backends.
         */
        virtual void compile(ql::quantum_program* programp, const ql::quantum_platform& plat)
        {
        }

        /**
         * write eqasm code to file/stdout
         */
        virtual void write_eqasm(std::string file_name="")
        {
            if (eqasm_code.empty())
                return;
            if (file_name=="")
            {
                println("[c] eqasm code (" << eqasm_code.size() << " lines) :");
                for (std::string l : eqasm_code)
                    std::cout << l << std::endl;
            }
            else
            {
                // write to file
                std::ofstream file(file_name);
                if (file.is_open())
                {
                    IOUT("writing eqasm code (" << eqasm_code.size() << " lines) to '" << file_name << "' ...");
                    for (std::string l : eqasm_code)
                        file << l << std::endl;
                    file.close();
                }
                else
                    EOUT("opening file '" << file_name << "' !");
            }
        }

        /**
         * write traces
         */
        virtual void write_traces(std::string file_name="")
        {
        }
    };
}

#endif // QL_EQASM_COMPILER_H

