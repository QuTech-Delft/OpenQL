import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

def test_AllXY():
    """
    Single qubit AllXY sequence.
    Writes output files to the directory specified in openql.
    Output directory is set as an attribute to the program for convenience.

    Input pars:
        qubit_idx:      int specifying the target qubit (starting at 0)
        platf_cfg:      filename of the platform config file
        double_points:  if true repeats every element twice
    Returns:
        p:              OpenQL Program object containing


    """
    config_fn = os.path.join(curdir, 'test_cfg_CCL.json')
    platf  = ql.Platform('seven_qubits_chip', config_fn)
    p = ql.Program(pname="AllXY", nqubits=platf.get_qubit_number(), p=platf)

    allXY = [ ['i', 'i'], ['rx180', 'rx180'], ['ry180', 'ry180'] ]

    # this should be implicit
    p.set_sweep_points(np.arange(len(allXY), dtype=float), len(allXY))
    qubit_idx=0
    for i, xy in enumerate(allXY):
        k = ql.Kernel("AllXY_"+str(i), p=platf)
        k.prepz(qubit_idx)
        k.gate(xy[0], qubit_idx)
        k.gate(xy[1], qubit_idx)
        k.measure(qubit_idx)
        p.add_kernel(k)

    p.compile()

def test_independent():
    config_fn = os.path.join(curdir, '../tests/test_config_default.json')
    platf = ql.Platform("starmon", config_fn)
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

    p = ql.Program("independent", nqubits, platf)
    p.set_sweep_points(sweep_points, num_circuits)
    p.add_kernel(k)
    p.compile(False, True)
    p.schedule("ASAP", True)

if __name__ == '__main__':
    test_independent()

# # two qubit mask generation test
# def test_smit():
#     # You can specify a config location, here we use a default config
#     config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
#     platform  = ql.Platform('seven_qubits_chip', config_fn)
#     sweep_points = [1,2]
#     num_circuits = 1
#     num_qubits = 7
#     p = ql.Program('my_program', num_qubits, platform)
#     p.set_sweep_points(sweep_points, num_circuits)

#     # populate kernel using default gates
#     k = ql.Kernel('first_kernel', platform)

#     k.prepz(0)
#     k.prepz(1)
#     k.prepz(2)
#     k.prepz(3)
#     k.hadamard(0)
#     k.hadamard(1)
#     k.x(2)
#     k.x(3)
#     k.cnot(2,0)
#     k.cnot(2,0)
#     k.cnot(1,4)
#     k.measure(0)
#     k.measure(1)
#     k.measure(2)
#     k.measure(3)

#     # add the kernel to the program
#     p.add_kernel(k)

#     # compile the program
#     p.compile(optimize=False, verbose=True)
