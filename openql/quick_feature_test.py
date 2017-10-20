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


def test_buffers():
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
    # k.gate("x", 0)
    # k.gate("y", 0)

    # should mw - mw be there in this case (different qubit but same qwg is involved)?
    # k.gate("x", 0)
    # k.gate("y", 1)

    # should mw - mw be there in this case (different qubit and different qwg is involved)?
    # k.gate("x", 0)
    # k.gate("y", 2)

    # mw - flux
    # k.gate("x", 2)
    # k.gate("cnot", [0,2])

    # mw - readout
    # k.gate("x", 0)
    # k.gate("measure", 0)

    # flux - flux
    # k.gate("cnot", [0,2])
    # k.gate("cnot", [0,2])

    # flux - mw
    # k.gate("cnot", [0,2])
    # k.gate("x", 2)

    # flux - readout
    # k.gate("cnot", [0,2])
    # k.gate("measure", 2)

    # readout - readout
    # k.gate("measure", 0)
    # k.gate("measure", 0)

    # readout - mw
    # k.gate("measure", 0)
    # k.gate("x", 0)

    # readout - flux
    # k.gate("measure", 0)
    # k.gate("cnot", [0,2])

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(optimize=False, verbose=True)

def test_latencies():
    config_fn = os.path.join(curdir, '../tests/test_cfg_cc_light_buffers_latencies.json')
    # config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_circuits = 1
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, num_circuits)

    k = ql.Kernel('aKernel', platform)

    # 'y q3' has a latency of 0 ns
    # k.gate("x", 0)
    # k.gate("y", 3)

    # 'y q4' has a latency of +20 ns
    # k.gate("x", 0)
    # k.gate("y", 4)

    # 'y q5' has a latency of -20 ns
    # k.gate("x", 0)
    # k.gate("y", 5)

    # qubit dependence and 'y q3' has a latency of 0 ns
    # k.gate("x", 3)
    # k.gate("y", 3)

    # qubit dependence and 'y q4' has a latency of +20 ns
    # k.gate("x", 4)
    # k.gate("y", 4)

    # qubit dependence and 'y q5' has a latency of -20 ns
    k.gate("x", 5)
    k.gate("y", 5)

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(optimize=False, verbose=True)

if __name__ == '__main__':
    # test2()
    # test_buffers()
    test_latencies()
