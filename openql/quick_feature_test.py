import os
import openql as ql


curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

def first_example():
    # You can specify a config location, here we use a default config
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
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


if __name__ == '__main__':
    first_example()



# import os
# import openql as ql

# curdir = os.path.dirname(__file__)
# config_fn = os.path.join(curdir, '../tests/test_cfg_cbox.json')
# platf = ql.Platform("starmon", config_fn)

# # all the outputs are generated in 'output' dir
# output_dir = os.path.join(curdir, 'output')
# ql.set_output_dir(output_dir)

# def feature_test():

#     # create a kernel
#     k = ql.Kernel("aKernel", platf)

#     # populate a kernel
#     k.prepz(0)
#     k.prepz(1)
#     k.hadamard(0)
#     k.cnot(0, 1)
#     # k.gate("x", 0)
#     k.x(0)
#     k.measure(0)

#     num_circuits = 2
#     sweep_points = [1, 1.25, 1.75, 2.25, 2.75 ]
#     nqubits = 2

#     # create a program
#     p = ql.Program("aProgram", nqubits, platf)
#     p.set_sweep_points(sweep_points, num_circuits)

#     # add kernel to program
#     p.add_kernel(k)

#     # compile  opt  verbose
#     p.compile(False, True)

# if __name__ == '__main__':
#     feature_test()
