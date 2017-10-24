# getting_started.py
from openql import openql as ql
import os

curdir = os.path.dirname(__file__)


def first_example():
    # set global options kernel
    ql.init()
    # You can specify a config location, here we use a default config
    config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
    platf = ql.Platform("starmon", config_fn)
    sweep_points = [1]
    num_circuits = 2
    nqubits = 2
    p = ql.Program("my_program", nqubits, platf)
    p.set_sweep_points(sweep_points, len(sweep_points))

    # populate kernel using default gates
    k = ql.Kernel("first_kernel", platf)
    k.prepz(0)
    k.ry90(0)
    k.rx180(0)
    k.measure(0)

    # add the kernel to the program
    p.add_kernel(k)

    # populate a second kernel using both custom and default gates
    k = ql.Kernel("second_kernel", platf)
    k.gate("prepz", 0) # this line is equivalent to the previous
    k.gate('rx180', 0)
    k.rx180(1)
    k.gate('cz', [0, 1])
    k.gate("measure", 0)

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(optimize=False, verbose=False)


if __name__ == '__main__':
    first_example()
