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
        k.gate("wait", [0], 20) # OR k.wait([1], 20)
        k.gate("x", 1)

        p.add_kernel(k)
        p.compile(False, "ALAP", False) # optimize  scheduler  verbose

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_wait_parallel.qisa'        
        self.assertTrue( file_compare(QISA_fn, gold_fn) )

    def test_wait_sweep(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, len(sweep_points))

        qubit_idx = 0
        waits = [20, 40, 60, 100, 200, 400, 800, 1000, 2000]
        for kno, wait_nanoseconds in enumerate(waits):
            k = ql.Kernel("kernel_"+str(kno), p=platform)

            k.prepz(qubit_idx)

            k.gate('rx90', qubit_idx)
            k.gate("wait", [qubit_idx], wait_nanoseconds)

            k.gate('rx180', qubit_idx)
            k.gate("wait", [qubit_idx], wait_nanoseconds)

            k.gate('rx90', qubit_idx)
            k.gate("wait", [qubit_idx], wait_nanoseconds)

            k.measure(qubit_idx)

            # add the kernel to the program
            p.add_kernel(k)

        # compile the program
        p.compile(False, "ASAP", True)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_wait_sweep.qisa'        
        self.assertTrue( file_compare(QISA_fn, gold_fn) )

    def test_wait_multi(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform)

        for i in range(4):
            k.gate("x", i)

        k.gate("wait", [0, 1, 2, 3], 40)
        k.wait([0, 1, 2, 3], 40)

        for i in range(4):
            k.gate("measure", i)

        p.add_kernel(k)
        p.compile(False, "ALAP", False) # optimize  scheduler  verbose

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_wait_multi.qisa'        
        self.assertTrue( file_compare(QISA_fn, gold_fn) )

    def test_wait_barrier(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform)

        k.gate("x", 0)
        k.gate("x", 1)
        k.gate("measure", 0)
        k.gate("measure", 1)
        k.gate("wait", [0, 1], 0) # this will serve as barrier
        k.gate("y", 0)

        p.add_kernel(k)
        p.compile(False, "ALAP", False) # optimize  scheduler  verbose

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_wait_barrier.qisa'        
        self.assertTrue( file_compare(QISA_fn, gold_fn) )

    def test_barrier(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform)

        k.gate("x", 0)
        k.gate("x", 1)
        k.gate("measure", 0)
        k.gate("measure", 1)

        # k.barrier([0, 1])
        # OR 
        k.gate("barrier", [0, 1])

        k.gate("y", 0)

        p.add_kernel(k)
        p.compile(False, "ALAP", False) # optimize  scheduler  verbose

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_barrier.qisa'        
        self.assertTrue( file_compare(QISA_fn, gold_fn) )

if __name__ == '__main__':
    unittest.main()
