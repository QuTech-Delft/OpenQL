from openql import openql as ql
import os
from random import random

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'visualizer_example_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
#ql.set_option('scheduler', 'ASAP')
#ql.set_option('log_level', 'LOG_DEBUG')
#ql.set_option('log_level', 'LOG_INFO')
ql.set_option('unique_output', 'yes')
ql.set_option('write_qasm_files', 'no')
ql.set_option('write_report_files', 'no')

c = ql.Compiler("testCompiler")
#c.add_pass("RotationOptimizer")
#c.add_pass("DecomposeToffoli")
#c.add_pass("Scheduler")
c.add_pass("Visualizer")
c.add_pass("BackendCompiler")

c.set_pass_option("Visualizer", "visualizer_type", "INTERACTION_GRAPH")
c.set_pass_option("Visualizer", "visualizer_config_path", os.path.join(curdir, "visualizer_config_example1.json"))
c.set_pass_option("BackendCompiler", "eqasm_compiler_name", "cc_light_compiler")

platformCustomGates = ql.Platform('starmon', os.path.join(curdir, 'hardware_config_cc_light_visualizer2.json'))
num_qubits = 2
p = ql.Program("Deutsch", platformCustomGates, num_qubits, 0)
k = ql.Kernel("kernel", platformCustomGates, num_qubits, 0)
for q in range(num_qubits):
	k.gate('prepz',[q])
k.gate('x',[1])	
for q in range(num_qubits):
	k.gate('h',[q])	
#cr = random()
cr=0.23
if (cr < 0.25):
	print("Balanced : IDENTITY")
	k.gate('cnot',[0,1])
elif (cr < 0.50):
	print("Balanced : NOT")
	k.gate('x',[0])
	k.gate('cnot',[0, 1])
	k.gate('x',[0])
elif (cr < 0.75):
	print("Unbalanced : RESET")
else:
	print("Unbalanced : SET")
	k.gate('x',[1])
k.gate('h',[0])
for q in range(num_qubits):
	k.gate('measure',[q])
p.add_kernel(k)
p.compile()