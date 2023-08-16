import openql as ql

from config import output_dir


ql.initialize()

ql.set_option('output_dir', output_dir)
ql.set_option('log_level', 'LOG_INFO')

platform = ql.Platform('my_platform', 'none')

num_qubits = 3
program = ql.Program('my_program', platform, num_qubits)
kernel = ql.Kernel('my_kernel', platform, num_qubits)

for i in range(num_qubits):
    kernel.prepz(i)

kernel.x(0)
kernel.hadamard(1)
kernel.cz(2, 0)
kernel.measure(0)
kernel.measure(1)

program.add_kernel(kernel)

program.compile()
