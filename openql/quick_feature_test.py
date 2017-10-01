import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

def test1():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_circuits = 1
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, num_circuits)

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
    p.compile(optimize=False, verbose=True)
  

def test_bug():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    # config_fn = os.path.join(curdir, '/home/iashraf/Desktop/test.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_circuits = 1
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, num_circuits)

    k = ql.Kernel('aKernel', platform)


    k.prepz(0)
    k.prepz(1)
    # k.x(0)
    k.measure(0)
    k.measure(1)
    # k.x(0)
    # k.gate('rx180', 0)

    # k.gate('rx180', 0)
    # # k.x(0)
    # k.measure(0)
    # k.gate('rx180', 0)
    # k.gate('rx180', 0)
    # k.prepz(0)
    # k.measure(0)

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(optimize=False, verbose=True)

if __name__ == '__main__':
    # test1()
    test_bug()

# should mw_mw_buffer be there at the top of gate duration or its the minimum between two gates?
#   whenever there are back to back operations, buffer need to be adjusted

# what about latency?
#   yes it should be compensated and it can be +ve/-ve

# size of bundle cannot be > 2?
#   Nope, no limit

# can operations of different types be combined together in one bundle?
#   yes, no problem

# mw - mw
# k.prepz(0)
# k.x(0)

# mw - readout
# k.prepz(0)
# k.measure(1)

# mw - flux
# k.prepz(1)
# k.cnot(2, 0)

# flux - mw
# k.cnot(2, 0)
# k.prepz(1)

# bundle size limit
# k.prepz(0)
# k.x(1)
# k.y(2)
