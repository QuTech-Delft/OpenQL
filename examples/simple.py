import openql as ql

ql.initialize()

ql.set_option('output_dir', 'output')
ql.set_option('log_level', 'LOG_INFO')

platform = ql.Platform('my_platform', 'none')

nqubits = 3
program = ql.Program('my_program', platform, nqubits)
kernel = ql.Kernel('my_kernel', platform, nqubits)

for i in range(nqubits):
    kernel.prepz(i)

kernel.x(0)
kernel.hadamard(1)
kernel.cz(2, 0)
kernel.measure(0)
kernel.measure(1)

program.add_kernel(kernel)

program.compile()
