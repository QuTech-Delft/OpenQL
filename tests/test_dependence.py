import os
import filecmp
import unittest
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)


class Test_dependence(unittest.TestCase):

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

        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_independence.qasm'
        isSame = filecmp.cmp('output/aProgramASAP.qasm', gold)
        self.assertTrue(isSame)


    def test_WAW(self):
        # set global options kernel
        ql.init()

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

        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_WAW.qasm'
        isSame = filecmp.cmp('output/aProgramASAP.qasm', gold)
        self.assertTrue(isSame)


    def test_RAR_Control(self):
        # set global options kernel
        ql.init()

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

        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_RAR_Control.qasm'
        isSame = filecmp.cmp('output/aProgramASAP.qasm', gold)
        self.assertTrue(isSame)


    def test_RAW(self):
        # set global options kernel
        ql.init()

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

        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_RAW.qasm'
        isSame = filecmp.cmp('output/aProgramASAP.qasm', gold)
        self.assertTrue(isSame)

    def test_WAR(self):
        # set global options kernel
        ql.init()

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

        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile(False, False)
        p.schedule("ASAP", False)

        gold = rootDir + '/golden/test_WAR.qasm'
        isSame = filecmp.cmp('output/aProgramASAP.qasm', gold)
        self.assertTrue(isSame)

if __name__ == '__main__':
    unittest.main()
