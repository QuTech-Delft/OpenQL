import os
import filecmp
import unittest
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_config_default.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)


class Test_dependence(unittest.TestCase):

    # @unittest.expectedFailure
    # @unittest.skip
    def test_independent(self):
        # populate kernel
        k = ql.Kernel("aKernel", platf)

        for i in range(4):
            k.prepz(i)

        # no dependence
        k.cnot(0, 1)
        k.cnot(2, 3)

        k.measure(0)
        k.measure(1)

        sweep_points = [2]
        num_circuits = 1
        nqubits = 4

        p = ql.Program("independent", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, True)
        p.schedule("ASAP", True)

        gold = rootDir + '/golden/test_independence.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'ASAP.qasm')
        isSame = filecmp.cmp(qasm_fn, gold)
        self.assertTrue(isSame)

    # @unittest.skip
    def test_WAW(self):

        # populate kernel
        k = ql.Kernel("aKernel", platf)

        for i in range(4):
            k.prepz(i)

        # q1 dependence
        k.cnot(0, 1)
        k.cnot(0, 1)

        k.measure(0)
        k.measure(1)

        sweep_points = [2]
        num_circuits = 1
        nqubits = 4

        p = ql.Program("WAW", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_WAW_ASAP.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'ASAP.qasm')
        isSame = filecmp.cmp(qasm_fn, gold)
        self.assertTrue(isSame)

    # @unittest.skip
    def test_RAR_Control(self):

        # populate kernel
        k = ql.Kernel("aKernel", platf)

        for i in range(4):
            k.prepz(i)

        # q0 dependence
        k.cnot(0, 1)
        k.cnot(0, 2)

        k.measure(0)
        k.measure(1)

        sweep_points = [2]
        num_circuits = 1
        nqubits = 4

        p = ql.Program("RAR", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_RAR_Control_ASAP.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'ASAP.qasm')
        isSame = filecmp.cmp(qasm_fn, gold)
        self.assertTrue(isSame)

    # @unittest.skip
    def test_RAW(self):

        # populate kernel
        k = ql.Kernel("aKernel", platf)

        for i in range(4):
            k.prepz(i)

        # q1 dependence
        k.cnot(0, 1)
        k.cnot(1, 2)

        k.measure(0)
        k.measure(1)

        sweep_points = [2]
        num_circuits = 1
        nqubits = 4

        p = ql.Program("RAW", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_RAW_ASAP.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'ASAP.qasm')
        isSame = filecmp.cmp(qasm_fn, gold)
        self.assertTrue(isSame)

    # @unittest.skip
    def test_WAR(self):

        # populate kernel
        k = ql.Kernel("aKernel", platf)

        for i in range(4):
            k.prepz(i)

        # q0 dependence
        k.cnot(0, 1)
        k.cnot(2, 0)

        k.measure(0)
        k.measure(1)

        sweep_points = [2]
        num_circuits = 1
        nqubits = 4

        p = ql.Program("WAR", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_WAR_ASAP.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'ASAP.qasm')
        isSame = filecmp.cmp(qasm_fn, gold)
        self.assertTrue(isSame)

if __name__ == '__main__':
    unittest.main()
