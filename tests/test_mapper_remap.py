# tests for mapper
#
# tests combination of prescheduler, clifford, mapper, clifford and postscheduler in cc_light context
#   by generating .qisa and comparing the generated one with a golden one after assembly
#
# assumes config files: test_mapper_rig.json, test_mapper_s7.json and test_mapper_s17.json
#
# written to avoid initial placement since that is not portable
# (although turning it on with options and enabling it by uncommenting first line of src/mapper.h would test it)
# for option assumptions, see setUp below
# see for more details, comment lines with each individual test below

from openql import openql as ql
import os
import unittest
from utils import file_compare


curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'mapper_test_output')

class Test_mapper(unittest.TestCase):

    def setUp(self):
        ql.initialize()
        # uses defaults of options in mapper branch except for output_dir and for maptiebreak
        ql.set_option('output_dir', output_dir)     # this uses output_dir set above
        ql.set_option('maptiebreak', 'first')       # this makes behavior deterministic to cmp with golden
                                                    # and deviates from default

        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('optimize', 'no')
        ql.set_option('use_default_gates', 'no')
        ql.set_option('decompose_toffoli', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'no')
        ql.set_option('scheduler_commute', 'yes')
        ql.set_option('prescheduler', 'yes')
        ql.set_option('scheduler_post179', 'yes')
        ql.set_option('cz_mode', 'manual')
        ql.set_option('print_dot_graphs', 'no')
        
        ql.set_option('clifford_premapper', 'yes')
        ql.set_option('clifford_postmapper', 'yes')
        ql.set_option('mapper', 'minextendrc')
        ql.set_option('mapinitone2one', 'yes')
        ql.set_option('initialplace', 'no')
        ql.set_option('initialplace2qhorizon', '0')
        ql.set_option('mapusemoves', 'yes')
        ql.set_option('mapreverseswap', 'yes')
        ql.set_option('mappathselect', 'all')
        ql.set_option('maplookahead', 'noroutingfirst')
        ql.set_option('maprecNN2q', 'no')
        ql.set_option('mapselectmaxlevel', '0')
        ql.set_option('mapselectmaxwidth', 'min')
        
        ql.set_option('write_qasm_files', 'yes')
        ql.set_option('write_report_files', 'yes')

    def test_mapper_oneD4_golden(self):
        # one cnot with operands that are at distance 4 in s7
        # initial placement should find this
        # otherwise ...
        # there are 4 alternative paths
        # in each path there are 4 alternative places to put the cnot
        # this introduces 3 swaps/moves
        # parameters
        v = 'oneD4_golden'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        ql.set_option('mapenableremaps', 'no')

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        k.gate("x", [2])
        k.gate("x", [4])
        k.gate("cnot", [2,4])
        k.gate("x", [2])
        k.gate("x", [4])

        prog.add_kernel(k)
        prog.compile()

    def test_mapper_oneD4_remaps(self):
        # one cnot with operands that are at distance 4 in s7
        # initial placement should find this
        # otherwise ...
        # there are 4 alternative paths
        # in each path there are 4 alternative places to put the cnot
        # this introduces 3 swaps/moves
        # parameters
        v = 'oneD4_remap'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        ql.set_option('mapenableremaps', 'yes')

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        k.gate("x", [2])
        k.gate("x", [4])
        k.gate("cnot", [2,4])
        k.gate("x", [2])
        k.gate("x", [4])

        prog.add_kernel(k)
        prog.compile()

if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
