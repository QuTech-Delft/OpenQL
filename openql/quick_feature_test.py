import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

def test_cclight():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)

    k.prepz(0)
    k.prepz(1)
    k.prepz(2)
    k.prepz(3)
    k.x(2)
    k.gate("wait", [1,2], 100)
    k.x(2)
    k.x(3)
    k.cnot(2, 0)
    k.cnot(1, 4)
    k.measure(0)
    k.measure(1)
    k.measure(2)
    k.measure(3)

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(False, "ALAP", True)

def test_none():
    config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')

    platform  = ql.Platform('platform_none', config_fn)
    sweep_points = [1,2]
    num_qubits = 2
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, len(sweep_points))

    k1 = ql.Kernel('aKernel1', platform)
    k2 = ql.Kernel('aKernel2', platform)

    k1.gate("x",0);
    k1.gate("x",0);

    k2.gate("x",0);
    k2.gate("x",0);

    # k.gate("cnot", 0, 1)
    # k.gate("cnot", [2, 3])

    # add the kernel to the program
    p.add_kernel(k1)
    p.add_kernel(k2)

    # compile the program
    # p.compile(optimize=False, scheduler="ASAP", verbose=True)
    p.compile(False, "ASAP", True)


def test_bug():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)

    # bundling of 1-qubit gates
    # for i in range(7):
    #     k.prepz(i)

    # for i in range(7):
    #     k.gate('x', i)

    # bundling of 2-qubit gates
    for i in range(7):
        k.prepz(i)

    k.gate('cz', 2, 0)
    k.gate('cz', 3, 5)
    k.gate('cz', 1, 4)

    k.gate('cz', 4, 6)
    k.gate('cz', 2, 5)
    k.gate('cz', 3, 0)


    # k.gate('cz', 3, 1)

    # k.gate('cnot', 2, 0)

    # k.gate("prepz",[0])
    # k.gate("prepz",[1])
    # k.gate("h",[0])
    # k.gate("cnot", [0, 2])
    # k.gate("measure", [0])
    # k.gate("MeasX", [0])
    # k.gate("measure", [1])

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(False, "ALAP", True)


if __name__ == '__main__':
    test_bug()
    # test_cclight()
    # test_none()
