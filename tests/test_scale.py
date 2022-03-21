# tests for scalability
#
# assumes config files: test_multi_core_8x1092_full.json
#

from openql import openql as ql
import os
import unittest
from utils import file_compare


curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_multi_core(unittest.TestCase):

    def setUp(self):
        ql.initialize()
        # uses defaults of options in mapper branch except for output_dir and for maptiebreak
        ql.set_option('output_dir', output_dir)     # this uses output_dir set above
        ql.set_option('maptiebreak', 'first')       # this makes behavior deterministic to cmp with golden
                                                    # and deviates from default

        ql.set_option('log_level', 'LOG_INFO')
        ql.set_option('optimize', 'no')
        ql.set_option('use_default_gates', 'no')
        ql.set_option('generate_code', 'no')

        ql.set_option('decompose_toffoli', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'no')
        ql.set_option('scheduler_commute', 'yes')
        ql.set_option('scheduler_commute_rotations', 'yes')
        ql.set_option('prescheduler', 'yes')
        ql.set_option('cz_mode', 'manual')
        ql.set_option('print_dot_graphs', 'no')
        
        ql.set_option('clifford_premapper', 'yes')
        ql.set_option('clifford_postmapper', 'no')
        ql.set_option('mapper', 'minextend')
        ql.set_option('mapinitone2one', 'yes')
        ql.set_option('mapassumezeroinitstate', 'yes')
        ql.set_option('initialplace', 'no')
        ql.set_option('initialplace2qhorizon', '0')
        ql.set_option('mapmaxalters', '1000')
        ql.set_option('mapusemoves', 'yes')
        ql.set_option('mapreverseswap', 'yes')
        ql.set_option('mappathselect', 'all')
        ql.set_option('maplookahead', 'noroutingfirst')
        ql.set_option('maprecNN2q', 'no')
        ql.set_option('mapselectmaxlevel', '0')
        ql.set_option('mapselectmaxwidth', 'min')
        
        # ql.set_option('write_qasm_files', 'yes')
        # ql.set_option('write_report_files', 'yes')

    def test_mc_wide(self):
        v = 'wide'
        config = os.path.join(curdir, "test_multi_core_8x1024_full.json")
        num_qubits = 2048

        # create and set platform
        prog_name = "test_mc_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("mc8x1092full", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for i in range(2048):
            k.gate("xm45", [i])
        for i in range(2048):
            k.gate("measure", [i])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog.name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_INFO')
    unittest.main()
