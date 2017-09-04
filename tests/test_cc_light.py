import os
import unittest
from openql import openql as ql

curdir    = os.path.dirname(__file__)

output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

class Test_basic(unittest.TestCase):

    # single qubit mask generation test
    # @unittest.expectedFailure
    # @unittest.skip
    def test_smis(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('my_program', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('first_kernel', platform)
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
        p.compile(optimize=False, verbose=True)


    # single qubit mask generation test with custom gates
    # @unittest.skip
    def test_smis_with_custom_gates(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 2
        num_qubits = 7
        p = ql.Program('my_program', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate the second kernel using both custom and default gates
        k = ql.Kernel('first_kernel', platform)
        k.gate('prepz', 0) # this line is equivalent to the previous
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


    # single qubit mask generation multi-kernel test (custom with non-custom gates)
    @unittest.skip
    def test_smis_multi_kernel(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 2
        num_qubits = 7
        p = ql.Program('my_program', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate the first kernel using default gates
        k = ql.Kernel('first_kernel', platform)
        k.prepz(0)
        k.x(2)
        k.y(3)
        k.z(5)
        k.rx90(0)
        # k.cphase(0,1)  # when uncommented this should raise 'operation not supported' exception. 
        # k.cphase(0,3)
        k.cnot(0,3)
        k.measure(0)

        # add the kernel to the program
        p.add_kernel(k)

        # populate the second kernel using both custom and default gates
        k = ql.Kernel('first_kernel', platform)
        k.gate('prepz', 0) # this line is equivalent to the previous
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


    # two qubit mask generation test
    # @unittest.skip
    def test_smit(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 7
        p = ql.Program('my_program', num_qubits, platform)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel using default gates
        k = ql.Kernel('first_kernel', platform)
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.prepz(3)
        k.hadamard(0)
        k.hadamard(1)
        k.x(2)
        k.x(3)
        k.cnot(2,0)
        k.cnot(2,0)
        k.cnot(1,4)
        k.measure(0)
        k.measure(1)
        k.measure(2)
        k.measure(3)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, verbose=False)



if __name__ == '__main__':
    unittest.main()
