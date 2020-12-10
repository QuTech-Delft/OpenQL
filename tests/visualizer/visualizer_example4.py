from openql import openql as ql
import os

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'visualizer_example_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')
ql.set_option('log_level', 'LOG_INFO')
ql.set_option('unique_output', 'yes')
ql.set_option('write_qasm_files', 'no')
ql.set_option('write_report_files', 'no')

c = ql.Compiler("testCompiler")
c.add_pass("RotationOptimizer")
c.add_pass("DecomposeToffoli")
c.add_pass("Scheduler")
c.add_pass("BackendCompiler")
c.add_pass("Visualizer")

c.set_pass_option("BackendCompiler", "eqasm_compiler_name", "cc_light_compiler")
c.set_pass_option("Visualizer", "visualizer_type", "CIRCUIT")
c.set_pass_option("Visualizer", "visualizer_config_path", os.path.join(curdir, "visualizer_config_example4.json"))

platformCustomGates = ql.Platform('starmon', os.path.join(curdir, 'hardware_config_cc_light_visualizer.json'))
nqubits = 6
p = ql.Program("testProgram1", platformCustomGates, nqubits, 0)
k = ql.Kernel("aKernel1", platformCustomGates, nqubits, 0)
for i in range(nqubits):
    k.gate('prepz', [i])
k.gate('x', [0])
k.gate('cnot', [0, 3])
k.gate('x', [0])
k.gate('h', [2])
k.gate('x', [4])
k.gate('h', [5])
k.gate('measure', [5])
k.gate('cz', [3, 1])
k.gate('cz', [2, 0])
k.gate('measure', [0])
k.gate('measure', [1])
k.gate('measure', [2])
k.gate('measure', [3])
p.add_kernel(k)
c.compile(p)