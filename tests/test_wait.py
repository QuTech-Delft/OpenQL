import os
import unittest
from openql import openql as ql
from test_QISA_assembler_present import assemble

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

def file_compare(fn1, fn2):
    isSame = False
    with open(fn1, 'r') as f1:
        with open(fn2, 'r') as f2:
            a = f1.read()
            b = f2.read()
            f1.close()
            f2.close()
            isSame = (a==b)
    return isSame


class Test_wait(unittest.TestCase):

    def test_wait_simple(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform)

        k.gate("x", 0)
        k.gate("wait", [0], 40) # OR k.wait([0], 40)
        k.gate("x", 0)

        p.add_kernel(k)
        p.compile(False, "ALAP", False) # optimize  scheduler  verbose

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_wait_simple.qisa'        
        self.assertTrue( file_compare(QISA_fn, gold_fn) )


    def test_wait_parallel(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform)

        # wait should not be in parallel with another gate
        k.gate("x", 0)
        k.gate("wait", [1], 20) # OR k.wait([1], 20)
        k.gate("x", 1)

        p.add_kernel(k)
        p.compile(False, "ALAP", False) # optimize  scheduler  verbose

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_wait_parallel.qisa'        
        self.assertTrue( file_compare(QISA_fn, gold_fn) )

if __name__ == '__main__':
    unittest.main()
