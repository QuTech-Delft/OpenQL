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
    config_fn = os.path.join(curdir, '../tests/test_cfg_cc_light_buffers_latencies.json')
    # config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_circuits = 1
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, num_circuits)

    k = ql.Kernel('aKernel', platform)

    # should mw - mw be there in this case (same qubit and of course same qwg is involved)?
    # k.prepz(0)
    # k.x(0)

    # should mw - mw be there in this case (different qubit but same qwg is involved)?
    k.prepz(0)
    k.x(1)

    # should mw - mw be there in this case (different qubit and different qwg is involved)?
    k.prepz(0)
    k.x(2)

    # mw - flux
    # k.prepz(2)
    # k.cnot(0,2)

    # mw - readout
    # k.prepz(0)
    # k.measure(0)

    # flux - flux
    # k.cnot(0,2)
    # k.cnot(0,2)

    # flux - mw
    # k.cnot(0,2)
    # k.prepz(2)

    # flux - readout
    # k.cnot(0,2)
    # k.measure(0)
    # k.measure(2)

    # readout - readout
    # k.measure(0)
    # k.measure(0)

    # readout - mw
    # k.measure(0)
    # k.x(0)

    # readout - flux
    # k.measure(0)
    # k.cnot(0,2)

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(optimize=False, verbose=True)

if __name__ == '__main__':
    # test2()
    test_bug()
