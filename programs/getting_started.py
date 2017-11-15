import openql.openql as ql
import os

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

def hello_openql():
    # if you copy this example somewhere else, make sure to provide
    # correct path of configuration file
    config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    # config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform = ql.Platform("myPlatform", config_fn)
    nqubits = 2
    p = ql.Program("aProgram", nqubits, platform)

    # create a kernel
    k = ql.Kernel("aKernel", platform)

    # populate kernel using default and custom gates
    for i in range(nqubits):
        k.prepz(i)

    k.x(0)
    k.hadamard(1)
    k.gate('cz', [0, 1])
    k.measure(0)
    k.gate("measure", 1)

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(optimize=False, scheduler='ASAP' , verbose=True)

    print('Output files are generated in {0}'.format(output_dir))


if __name__ == '__main__':
    hello_openql()
