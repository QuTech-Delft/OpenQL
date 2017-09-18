import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

def test_qubit_busy():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)


def test_qwg_available_01():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)

def test_qwg_available_02():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)

def test_qwg_busy():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)

def test_measure_available01():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_circuits = 1
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, num_circuits)

    k = ql.Kernel('aKernel', platform)

    k.measure(0)
    k.measure(5)

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(optimize=False, verbose=True)

def test_measure_available02():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)

def test_measure_busy():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)

def test_edge_available():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)

def test_edge_busy():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)

def test_edge_illegal():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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
    p.compile(optimize=False, verbose=True)

if __name__ == '__main__':
    test_edge_illegal()
