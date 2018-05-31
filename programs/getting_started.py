from openql import openql as ql
import os

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')
ql.set_option('log_level', 'LOG_WARNING')

def hello_openql():
    # if you copy this example somewhere else, make sure to provide
    # correct path of configuration file copy the configuration file
    # to the same directory and update the path
    # config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform = ql.Platform("myPlatform", config_fn)

    sweep_points = [1]
    nqubits = 3

    # create a program
    p = ql.Program("aProgram", nqubits, platform)

    # set sweep points
    p.set_sweep_points(sweep_points, len(sweep_points))

    # create a kernel
    k = ql.Kernel("aKernel", platform)

    # populate kernel using default and custom gates
    for i in range(nqubits):
        k.gate('prepz', [i])

    k.gate('x', [0])
    k.gate('h', [1])
    k.gate('cz', [2, 0])
    k.gate('measure', [0])
    k.gate('measure', [1])

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile()

    # print a friendly message to indicate location of output files
    print('Output files are generated in {0}'.format(output_dir))


if __name__ == '__main__':
    hello_openql()
