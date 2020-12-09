import os
from utils import file_compare
import unittest
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_commutation(unittest.TestCase):

    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option("scheduler_uniform", "no")
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('write_qasm_files', 'yes')
        ql.set_option('write_report_files', 'yes')

    def test_cnot_controlcommute(self):
        config_fn = os.path.join(curdir, 'test_mapper_s7.json')
        platf = ql.Platform("starmon", config_fn)
        ql.set_option("scheduler", 'ALAP');
        ql.set_option("scheduler_commute", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on control operand of CNOT
        # for scheduler, last one is most critical so must be first
        k.gate("cnot", [3,0]);
        k.gate("cnot", [3,6]);
        k.gate("t", [6]);
        k.gate("y", [6]);
        k.gate("cnot", [3,1]);
        k.gate("t", [1]);
        k.gate("y", [1]);
        k.gate("t", [1]);
        k.gate("y", [1]);
        k.gate("cnot", [3,5]);
        k.gate("t", [5]);
        k.gate("y", [5]);
        k.gate("t", [5]);
        k.gate("y", [5]);
        k.gate("t", [5]);
        k.gate("y", [5]);

        sweep_points = [2]

        p = ql.Program("test_cnot_controlcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_targetcommute(self):
        config_fn = os.path.join(curdir, 'test_mapper_s7.json')
        platf = ql.Platform("starmon", config_fn)
        ql.set_option("scheduler", 'ALAP');
        ql.set_option("scheduler_commute", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on target operand of CNOT
        # for scheduler, last one is most critical so must be first
        k.gate("cnot", [0,3]);
        k.gate("cnot", [6,3]);
        k.gate("t", [6]);
        k.gate("y", [6]);
        k.gate("cnot", [1,3]);
        k.gate("t", [1]);
        k.gate("y", [1]);
        k.gate("t", [1]);
        k.gate("y", [1]);
        k.gate("cnot", [5,3]);
        k.gate("t", [5]);
        k.gate("y", [5]);
        k.gate("t", [5]);
        k.gate("y", [5]);
        k.gate("t", [5]);
        k.gate("y", [5]);

        sweep_points = [2]

        p = ql.Program("test_cnot_targetcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cz_anycommute(self):
        config_fn = os.path.join(curdir, 'test_mapper_s7.json')
        platf = ql.Platform("starmon", config_fn)
        ql.set_option("scheduler", 'ALAP');
        ql.set_option("scheduler_commute", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on both operands of CZ
        # for scheduler, last one is most critical so must be first
        k.gate("cz", [0,3]);
        k.gate("cz", [3,6]);
        k.gate("t", [6]);
        k.gate("y", [6]);
        k.gate("cz", [1,3]);
        k.gate("t", [1]);
        k.gate("y", [1]);
        k.gate("t", [1]);
        k.gate("y", [1]);
        k.gate("cz", [3,5]);
        k.gate("t", [5]);
        k.gate("y", [5]);
        k.gate("t", [5]);
        k.gate("y", [5]);
        k.gate("t", [5]);
        k.gate("y", [5]);

        sweep_points = [2]

        p = ql.Program("test_cz_anycommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_mixedcommute(self):
        config_fn = os.path.join(curdir, 'test_mapper_s7.json')
        platf = ql.Platform("starmon", config_fn)
        ql.set_option("scheduler", 'ALAP');
        ql.set_option("scheduler_commute", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        for j in range(7):
            k.gate("x", [j])
        # commute on mixture of operands of many CNOTs
        # basically, each CNOT(a,b) must ultimately be before CNOT(b,a)
        # in between, CNOT(a,b) commutes with CNOT(a,c) and CNOT(d,b)
        k.gate("cnot", [0,2]);
        k.gate("cnot", [0,3]);
        k.gate("cnot", [1,3]);
        k.gate("cnot", [1,4]);
        k.gate("cnot", [2,0]);
        k.gate("cnot", [2,5]);
        k.gate("cnot", [3,0]);
        k.gate("cnot", [3,1]);
        k.gate("cnot", [3,5]);
        k.gate("cnot", [3,6]);
        k.gate("cnot", [4,1]);
        k.gate("cnot", [4,6]);
        k.gate("cnot", [5,2]);
        k.gate("cnot", [5,3]);
        k.gate("cnot", [6,3]);
        k.gate("cnot", [6,4]);
        for j in range(7):
            k.gate("x", [j])

        sweep_points = [2]

        p = ql.Program("test_cnot_mixedcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_variations(self):
        config_fn = os.path.join(curdir, 'test_mapper_s7.json')
        platf = ql.Platform("starmon", config_fn)
        ql.set_option("scheduler", 'ALAP');
        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("vary_commutations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        for j in range(7):
            k.gate("x", [j])
        # commute on mixture of operands of many CNOTs
        # basically, each CNOT(a,b) must ultimately be before CNOT(b,a)
        # in between, CNOT(a,b) commutes with CNOT(a,c) and CNOT(d,b)
        # there will be several commutation sets
        k.gate("cnot", [0,2]);
        k.gate("cnot", [0,3]);
        k.gate("cnot", [1,3]);
        k.gate("cnot", [1,4]);
        k.gate("cnot", [2,0]);
        k.gate("cnot", [2,5]);
        k.gate("cnot", [3,0]);
        k.gate("cnot", [3,1]);
        k.gate("cnot", [3,5]);
        k.gate("cnot", [3,6]);
        k.gate("cnot", [4,1]);
        k.gate("cnot", [4,6]);
        # k.gate("cnot", [5,2]);
        # k.gate("cnot", [5,3]);
        # k.gate("cnot", [6,3]);
        # k.gate("cnot", [6,4]);
        for j in range(7):
            k.gate("x", [j])

        sweep_points = [2]

        p = ql.Program("test_cnot_variations", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/test_cnot_variations_last.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

if __name__ == '__main__':
    unittest.main()
