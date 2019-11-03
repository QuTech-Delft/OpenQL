/**
 * @file   qsoverlay.h
 * @date   11/2019
 * @author Diogo Valada
 * @brief  Prepares the quantumsim circuits using the qsoverlay format
 */

#include <vector>
#include <string>
#include <kernel.h>
#include <gate.h>

//Only support for DiCarlo setup atm
void write_qsoverlay_program( std::string prog_name, size_t num_qubits,
        std::vector<ql::quantum_kernel>& kernels, const ql::quantum_platform & platform, std::string suffix)
    {
        IOUT("Writing scheduled QSoverlay program");
        ofstream fout;
        string qfname( ql::options::get("output_dir") + "/" + prog_name + "_quantumsim_" + suffix + ".py");
        DOUT("Writing scheduled QSoverlay program " << qfname);
        IOUT("Writing scheduled QSoverlay program " << qfname);
        fout.open( qfname, ios::binary);
        if ( fout.fail() )
        {
            EOUT("opening file " << qfname << std::endl
                     << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }

        fout << "# Quantumsim (using Qsoverlay) program generated OpenQL\n"
             << "# Please modify at your will to obtain extra information from Quantumsim\n\n";

        fout << "import numpy as np\n"
			 << "from qsoverlay import DiCarlo_setup\n"
			 << "from qsoverlay.circuit_builder import Builder\n";
			
		fout << "import quantumsim.sparsedm as sparsedm\n"
             << "\n"
             << "# print('GPU is used:', sparsedm.using_gpu)\n"
             << "\n"
             << "\n";

		std::string qubit_list = "";
		for (size_t i=0; i < num_qubits; i++)
			qubit_list += (i != num_qubits-1) ? std::to_string(i) + ", " : std::to_string(i);
		fout << "qubit_list = [" + qubit_list + "]\n";

		fout << "setup = DiCarlo_setup.quick_setup(qubit_list)\n";

		//Gate correspondence
		fout << "Now the circuit is created\n"
			 << "b = Builder(setup)\n";

		std::map <std::string, std::string> gate_map = {
			{"x", "X"},
			{"x45", "RX"},
			{"x90", "RX"},
			{"xm45", "RX"},
			{"xm90", "RX"},
			{"y", "Y"},
			{"y45", "RY"},
			{"y90", "RY"},
			{"ym45", "RY"},
			{"ym90", "RY"},
			{"h", "H"},
			{"cz", "CZ"},
			{"measure", "Measure"}
		};

		std::map <std::string, std::string> angles = {
			{"x45", "np.pi/4"},
			{"x90", "np.pi/2"},
			{"xm45", "-np.pi/4"},
			{"xm90", "-np.pi/2"},
			{"y45", "np.pi/4"},
			{"y90", "np.pi/2"},
			{"ym45", "-np.pi/4"},
			{"ym90", "-np.pi/2"},
		};

		//Circuit creation
		fout << "def circuit_generated() :\n";
		for (auto & gate: kernels.front().c)
		{
			std::string qs_name = gate_map[gate->name];
			
			fout << "	b.add_gate('" << qs_name  << "', " << "['" 
				<< std::to_string(gate->operands[0])
				<< ( gate->operands.size() == 1 ) ? "']" : ", " + std::to_string(gate->operands[1]) + "']";
			
			if (qs_name == "RX" or qs_name == "RY")
			{
				fout << ", angle = " << angles[gate->name];
			}
			fout << ")\n";
		}

		fout << "	b.finalize()\n"
			 << "	return b.circuit\n";

        fout.close();
        IOUT("Writing scheduled Quantumsim program [Done]");
    }
