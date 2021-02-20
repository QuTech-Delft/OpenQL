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
output_dir = os.path.join(curdir, 'test_output')

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
        
        ql.set_option('write_qasm_files', 'no')
        ql.set_option('write_report_files', 'no')


    def test_mapper_maxcut(self):
        # rigetti test copied from Venturelli's paper
        v = 'maxcut'
        config = os.path.join(curdir, "test_mapper_rig.json")
        num_qubits = 8

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        k.gate("x", [0])
        k.gate("x", [1])
        k.gate("x", [2])
        k.gate("x", [3])
        k.gate("x", [4])
        k.gate("x", [5])
        k.gate("x", [6])
        k.gate("x", [7])

        k.gate("cz", [1,4])
        k.gate("cz", [1,3])
        k.gate("cz", [3,4])
        k.gate("cz", [3,7])
        k.gate("cz", [4,7])
        k.gate("cz", [6,7])
        k.gate("cz", [5,6])
        k.gate("cz", [1,5])

        k.gate("x", [0])
        k.gate("x", [1])
        k.gate("x", [2])
        k.gate("x", [3])
        k.gate("x", [4])
        k.gate("x", [5])
        k.gate("x", [6])
        k.gate("x", [7])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


    def test_mapper_oneNN(self):
        # just check whether mapper works for trivial case
        # parameters
        v = 'oneNN'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        k.gate("x", [0])
        k.gate("y", [1])
        k.gate("cnot", [2,5])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


    def test_mapper_allNN(self):
        # a list of all cnots that are NN in trivial/natural mapping (as in test_mapper_s7.json)
        #   (s7 is 3 rows: 2 data qubits (0 and 1), 3 ancillas (2 to 4), and 2 data qubits (5 and 6))
        # so no swaps are inserted and map is not changed
        # also tests commutation of cnots in mapper and postscheduler
        # parameters
        v = 'allNN'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [0,2])
        k.gate("cnot", [0,3])
        k.gate("cnot", [1,3])
        k.gate("cnot", [1,4])
        k.gate("cnot", [2,0])
        k.gate("cnot", [2,5])
        k.gate("cnot", [3,0])
        k.gate("cnot", [3,1])
        k.gate("cnot", [3,5])
        k.gate("cnot", [3,6])
        k.gate("cnot", [4,1])
        k.gate("cnot", [4,6])
        k.gate("cnot", [5,2])
        k.gate("cnot", [5,3])
        k.gate("cnot", [6,3])
        k.gate("cnot", [6,4])
        for j in range(7):
            k.gate("x", [j])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_mapper_oneD2(self):
        # one cnot with operands that are at distance 2 in s7
        # initial placement should find this
        # otherwise ...
        # there are 2 alternative paths
        # in each path there are 2 alternative places to put the cnot
        # this introduces 1 swap/move and so uses an ancilla
        # parameters
        v = 'oneD2'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        k.gate("x", [2])
        k.gate("y", [3])
        k.gate("cnot", [2,3])
        k.gate("x", [2])
        k.gate("y", [3])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


    def test_mapper_oneD4(self):
        # one cnot with operands that are at distance 4 in s7
        # initial placement should find this
        # otherwise ...
        # there are 4 alternative paths
        # in each path there are 4 alternative places to put the cnot
        # this introduces 3 swaps/moves
        # parameters
        v = 'oneD4'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        k.gate("x", [2])
        k.gate("y", [4])
        k.gate("cnot", [2,4])
        k.gate("x", [2])
        k.gate("y", [4])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


    def test_mapper_allD(self):
        # all possible cnots in s7, in lexicographic order
        # there is no initial mapping that maps this right so initial placement cannot find it
        # so the heuristics must act and insert swaps/moves
        # parameters
        v = 'allD'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for j in range(7):
            k.gate("x", [j])

        for i in range(7):
            for j in range(7):
                if (i != j):
                    k.gate("cnot", [i,j])

        for j in range(7):
            k.gate("x", [j])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


    def test_mapper_allDopt(self):
        # all possible cnots in s7, avoiding collisions:
        # - the pair of possible CNOTs in both directions hopefully in parallel
        # - these pairs ordered from low distance to high distance to avoid disturbance by swaps
        # - and then as much as possible in opposite sides of the circuit to improve ILP
        # idea is to get shortest latency circuit with all possible cnots
        # there is no initial mapping that maps this right so initial placement cannot find it
        # so the heuristics must act and insert swaps/moves
        # parameters
        v = 'allDopt'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [0,3]);
        k.gate("cnot", [3,0]);
        k.gate("cnot", [6,4]);
        k.gate("cnot", [4,6]);
        k.gate("cnot", [3,1]);
        k.gate("cnot", [1,3]);
        k.gate("cnot", [5,2]);
        k.gate("cnot", [2,5]);
        k.gate("cnot", [1,4]);
        k.gate("cnot", [4,1]);
        k.gate("cnot", [3,5]);
        k.gate("cnot", [5,3]);
        k.gate("cnot", [6,3]);
        k.gate("cnot", [3,6]);
        k.gate("cnot", [2,0]);
        k.gate("cnot", [0,2]);
        k.gate("cnot", [0,1]);
        k.gate("cnot", [1,0]);
        k.gate("cnot", [3,4]);
        k.gate("cnot", [4,3]);
        k.gate("cnot", [1,6]);
        k.gate("cnot", [6,1]);
        k.gate("cnot", [6,5]);
        k.gate("cnot", [5,6]);
        k.gate("cnot", [3,2]);
        k.gate("cnot", [2,3]);
        k.gate("cnot", [5,0]);
        k.gate("cnot", [0,5]);
        k.gate("cnot", [0,6]);
        k.gate("cnot", [6,0]);
        k.gate("cnot", [1,5]);
        k.gate("cnot", [5,1]);
        k.gate("cnot", [0,4]);
        k.gate("cnot", [4,0]);
        k.gate("cnot", [6,2]);
        k.gate("cnot", [2,6]);
        k.gate("cnot", [2,1]);
        k.gate("cnot", [1,2]);
        k.gate("cnot", [5,4]);
        k.gate("cnot", [4,5]);
        k.gate("cnot", [2,4]);
        k.gate("cnot", [4,2]);
        for j in range(7):
            k.gate("x", [j])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


    def test_mapper_allIP(self):
        # longest string of cnots with operands that could be at distance 1 in s7
        # matches intel NISQ application
        # initial placement should find this
        # and then no swaps are inserted
        # otherwise ...
        # the heuristics must act and insert swaps/moves
        # parameters
        v = 'allIP'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [0,1]);
        k.gate("cnot", [1,2]);
        k.gate("cnot", [2,3]);
        k.gate("cnot", [3,4]);
        k.gate("cnot", [4,5]);
        k.gate("cnot", [5,6]);
        for j in range(7):
            k.gate("x", [j])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))



    def test_mapper_lingling5(self):
        # parameters
        # 'realistic' circuit
        v = 'lingling5'
        config = os.path.join(curdir, "test_mapper_s17.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        k.gate("prepz", [5]);
        k.gate("prepz", [6]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("x", [6]);
        k.gate("ym90", [6]);
        k.gate("ym90", [0]);
        k.gate("cz", [5,0]);
        k.gate("ry90", [0]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [6,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [1,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [2,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [6,5]);
        k.gate("ry90", [5]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("ym90", [3]);
        k.gate("cz", [5,3]);
        k.gate("ry90", [3]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("measure", [5]);
        k.gate("measure", [6]);

        k.gate("prepz", [5]);
        k.gate("prepz", [6]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("x", [6]);
        k.gate("ym90", [6]);
        k.gate("ym90", [1]);
        k.gate("cz", [5,1]);
        k.gate("ry90", [1]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [6,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [2,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [3,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [6,5]);
        k.gate("ry90", [5]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("ym90", [4]);
        k.gate("cz", [5,4]);
        k.gate("ry90", [4]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("measure", [5]);
        k.gate("measure", [6]);

        k.gate("prepz", [5]);
        k.gate("prepz", [6]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("x", [6]);
        k.gate("ym90", [6]);
        k.gate("ym90", [2]);
        k.gate("cz", [5,2]);
        k.gate("ry90", [2]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [6,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [3,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [4,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [6,5]);
        k.gate("ry90", [5]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("ym90", [0]);
        k.gate("cz", [5,0]);
        k.gate("ry90", [0]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("measure", [5]);
        k.gate("measure", [6]);

        k.gate("prepz", [5]);
        k.gate("prepz", [6]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("x", [6]);
        k.gate("ym90", [6]);
        k.gate("ym90", [3]);
        k.gate("cz", [5,3]);
        k.gate("ry90", [3]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [6,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [4,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [0,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [5]);
        k.gate("cz", [6,5]);
        k.gate("ry90", [5]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("ym90", [1]);
        k.gate("cz", [5,1]);
        k.gate("ry90", [1]);
        k.gate("x", [5]);
        k.gate("ym90", [5]);
        k.gate("measure", [5]);
        k.gate("measure", [6]);

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))



    def test_mapper_lingling7(self):
        # parameters
        v = 'lingling7'
        config = os.path.join(curdir, "test_mapper_s17.json")
        num_qubits = 9

        # create and set platform
        prog_name = "test_mapper_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        k.gate("prepz", [7]);
        k.gate("prepz", [8]);
        k.gate("x", [7]);
        k.gate("ym90", [7]);
        k.gate("ym90", [4]);
        k.gate("cz", [7,4]);
        k.gate("ry90", [4]);
        k.gate("ym90", [8]);
        k.gate("cz", [0,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [8]);
        k.gate("cz", [7,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [6]);
        k.gate("cz", [7,6]);
        k.gate("ry90", [6]);
        k.gate("ym90", [8]);
        k.gate("cz", [2,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [3]);
        k.gate("cz", [7,3]);
        k.gate("ry90", [3]);
        k.gate("ym90", [8]);
        k.gate("cz", [4,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [8]);
        k.gate("cz", [7,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [5]);
        k.gate("cz", [7,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [8]);
        k.gate("cz", [6,8]);
        k.gate("ry90", [8]);
        k.gate("x", [7]);
        k.gate("ym90", [7]);
        k.gate("measure", [7]);
        k.gate("measure", [8]);
        k.gate("prepz", [7]);
        k.gate("prepz", [8]);
        k.gate("x", [7]);
        k.gate("ym90", [7]);
        k.gate("ym90", [5]);
        k.gate("cz", [7,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [8]);
        k.gate("cz", [1,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [8]);
        k.gate("cz", [7,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [6]);
        k.gate("cz", [7,6]);
        k.gate("ry90", [6]);
        k.gate("ym90", [8]);
        k.gate("cz", [2,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [3]);
        k.gate("cz", [7,3]);
        k.gate("ry90", [3]);
        k.gate("ym90", [8]);
        k.gate("cz", [5,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [8]);
        k.gate("cz", [7,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [4]);
        k.gate("cz", [7,4]);
        k.gate("ry90", [4]);
        k.gate("ym90", [8]);
        k.gate("cz", [6,8]);
        k.gate("ry90", [8]);
        k.gate("x", [7]);
        k.gate("ym90", [7]);
        k.gate("measure", [7]);
        k.gate("measure", [8]);

        k.gate("prepz", [7]);
        k.gate("prepz", [8]);
        k.gate("x", [7]);
        k.gate("ym90", [7]);
        k.gate("ym90", [1]);
        k.gate("cz", [7,1]);
        k.gate("ry90", [1]);
        k.gate("ym90", [8]);
        k.gate("cz", [2,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [8]);
        k.gate("cz", [7,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [5]);
        k.gate("cz", [7,5]);
        k.gate("ry90", [5]);
        k.gate("ym90", [8]);
        k.gate("cz", [6,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [2]);
        k.gate("cz", [7,2]);
        k.gate("ry90", [2]);
        k.gate("ym90", [8]);
        k.gate("cz", [0,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [8]);
        k.gate("cz", [7,8]);
        k.gate("ry90", [8]);
        k.gate("ym90", [6]);
        k.gate("cz", [7,6]);
        k.gate("ry90", [6]);
        k.gate("ym90", [8]);
        k.gate("cz", [4,8]);
        k.gate("ry90", [8]);
        k.gate("x", [7]);
        k.gate("ym90", [7]);
        k.gate("measure", [7]);
        k.gate("measure", [8]);

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))



if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
