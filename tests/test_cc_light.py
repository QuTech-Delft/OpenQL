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


class Test_basic(unittest.TestCase):

    # single qubit mask generation test
    # @unittest.expectedFailure
    # @unittest.skip
    def test_smis(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_circuits = 1
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.prepz(3)
        k.hadamard(0)
        k.hadamard(1)
        k.x(2)
        k.x(3)
        k.measure(0)
        k.measure(1)
        k.measure(2)
        k.measure(3)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    # single qubit mask generation test with custom gates
    # @unittest.skip
    def test_smis_with_custom_gates(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_circuits = 2
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate the second kernel using both custom and default gates
        k = ql.Kernel('aKernel', platform)
        k.gate('prepz', 0)
        k.gate('x', 0)
        k.y(1)
        k.z(5)
        k.gate('rx90', 0)
        k.gate('cnot', [0, 3])
        k.gate('measz', 0)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    # single qubit mask generation multi-kernel test (custom with non-custom
    # gates)
    # @unittest.skip
    def test_smis_multi_kernel(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_circuits = 2
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate the first kernel using default gates
        k = ql.Kernel('aKernel01', platform)
        k.prepz(0)
        k.x(2)
        k.y(3)
        k.z(5)
        k.rx90(0)
        # k.cphase(0,1)  # when uncommented this should raise 'operation not supported' exception.
        # k.cphase(0,3)
        k.cnot(0, 3)
        k.measure(0)

        # add the kernel to the program
        p.add_kernel(k)

        # populate the second kernel using both custom and default gates
        k = ql.Kernel('aKernel02', platform)
        k.gate('prepz', 0)  # this line is equivalent to the previous
        k.gate('x', 0)
        k.y(1)
        k.z(5)
        k.gate('rx90', 0)
        # k.gate('cz', [0, 3])
        k.gate('cnot', [0, 3])
        k.gate('measz', 0)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    # two qubit mask generation test
    # @unittest.skip
    def test_smit(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.prepz(3)
        k.hadamard(0)
        k.hadamard(1)
        k.x(2)
        k.x(3)
        k.cnot(2, 0)
        k.cnot(2, 0)
        k.cnot(1, 4)
        k.measure(0)
        k.measure(1)
        k.measure(2)
        k.measure(3)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)


class Test_advance(unittest.TestCase):
	# @unittest.skip
    def test_qubit_busy(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.prepz(0)
        k.prepz(0)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_qwg_available_01(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.hadamard(0)
        k.x(2)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_qwg_available_02(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.x(0)
        k.x(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_qwg_busy(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.hadamard(0)
        k.x(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

	# @unittest.skip
    def test_measure_available01(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        k = ql.Kernel('aKernel', platform)

        k.measure(0)
        k.measure(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_measure_available02(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.measure(0)
        k.x(0)
        k.measure(4)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

	# @unittest.skip
    def test_measure_busy(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.measure(0)
        k.x(0)
        k.measure(5)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_edge_available(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.cphase(0, 2)
        k.cphase(4, 6)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

	# @unittest.skip
    def test_edge_busy(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.cphase(0, 2)
        k.cphase(1, 3)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_edge_illegal(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)

        k.cphase(0, 2)
        k.cphase(0, 1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        try:
            p.compile(optimize=False, verbose=False)
            raise
        except:
            pass

    # fast feedback test
    # @unittest.expectedFailure
    # @unittest.skip
    def test_fast_feedback(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_circuits = 1
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform)
        k.prepz(0)
        k.gate('cprepz', 1)
        k.measure(0)
        k.measure(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)


    def test_ccl_buffers(self):

        tests = [
                # mw - mw buffer (same qubit and of course same awg channel is involved)
                            (0, [ ("x",[0]), ("y",[0]) ]),
                # mw - mw buffer (different qubit but same awg channel is involved)
                            (1, [ ("x",[0]), ("y",[1]) ]),
                # should mw - mw buffer (different qubit and different awg channel)
                            (2, [ ("x",[0]), ("y",[2]) ]),
                # mw - flux
                            (3, [ ("x",[2]), ("cnot", [0,2]) ]),
                # mw - readout
                            (4, [ ("x",[0]), ("measure", [0]) ]),
                # flux - flux
                            (5, [ ("cnot", [0,2]), ("cnot", [0,2]) ]),
                # flux - mw
                            (6, [ ("cnot", [0,2]), ("x", [2]) ]),
                # flux - readout
                            (7, [("cnot", [0,2]), ("measure", [2])]),
                # readout - readout
                            (8, [("measure", [0]), ("measure", [0])]),
                # readout - mw
                            (9, [("measure", [0]), ("x", [0])]),
                # readout - flux
                            (10, [("measure", [0]), ("cnot", [0,2]) ])
                ]

        for testNo, testKernel in tests:
            print('Running test_ccl_buffers No: {}'.format(testNo))
            config_fn = os.path.join(curdir, 'test_cfg_cc_light_buffers_latencies.json')
            platform  = ql.Platform('seven_qubits_chip', config_fn)
            sweep_points = [1,2]
            num_circuits = 1
            num_qubits = 7
            p = ql.Program('aProgram', num_qubits, platform)
            p.set_sweep_points(sweep_points, num_circuits)
            k = ql.Kernel('aKernel', platform)

            for gate, qubits in testKernel:
                k.gate(gate, qubits)

            p.add_kernel(k)
            p.compile(optimize=False, verbose=True)

            QISA_fn = os.path.join(output_dir, p.name+'.qisa')
            gold_fn = rootDir + '/golden/test_ccl_buffers_'+str(testNo)+'.qisa'        

            assemble(QISA_fn)

            self.assertTrue( file_compare(QISA_fn, gold_fn) )


    def test_ccl_latencies(self):

        tests = [
                # 'y q3' has a latency of 0 ns
                            (0, [("x", [0]), ("y", [3])]),
                # 'y q4' has a latency of +20 ns
                            (1, [("x", [0]),("y", [4])]),
                # 'y q5' has a latency of -20 ns
                            (2, [("x", [0]),("y", [5])]),
                # qubit dependence and 'y q3' has a latency of 0 ns
                            (3, [("x", [3]),("y", [3])]),
                # qubit dependence and 'y q4' has a latency of +20 ns
                            (4, [("x", [4]),("y", [4])]),
                # qubit dependence and 'y q5' has a latency of -20 ns
                            (5, [("x", [5]),("y", [5])])
                ]

        for testNo, testKernel in tests:
            print('Running test_ccl_latencies No: {}'.format(testNo))
            config_fn = os.path.join(curdir, 'test_cfg_cc_light_buffers_latencies.json')
            platform  = ql.Platform('seven_qubits_chip', config_fn)
            sweep_points = [1,2]
            num_circuits = 1
            num_qubits = 7
            p = ql.Program('aProgram', num_qubits, platform)
            p.set_sweep_points(sweep_points, num_circuits)
            k = ql.Kernel('aKernel', platform)

            for gate, qubits in testKernel:
                k.gate(gate, qubits)

            p.add_kernel(k)
            p.compile(optimize=False, verbose=True)

            QISA_fn = os.path.join(output_dir, p.name+'.qisa')
            gold_fn = rootDir + '/golden/test_ccl_latencies_'+str(testNo)+'.qisa'        

            assemble(QISA_fn)

            self.assertTrue( file_compare(QISA_fn, gold_fn) )


if __name__ == '__main__':
    unittest.main()
