from openql import openql as ql
import os

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'visualizer_example_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')

ql.set_option('clifford_premapper', 'yes')
ql.set_option('clifford_postmapper', 'yes')
ql.set_option('mapper', 'minextendrc')
ql.set_option('mapinitone2one', 'yes')
ql.set_option('initialplace', 'no')
ql.set_option('initialplace2qhorizon', '0')
ql.set_option('mapusemoves', 'yes')
ql.set_option('mapreverseswap', 'yes')
ql.set_option('mappathselect', 'all')
ql.set_option('maplookahead', 'noroutingfirst')
ql.set_option('maprecNN2q', 'no')
ql.set_option('mapselectmaxlevel', '0')
ql.set_option('mapselectmaxwidth', 'min')
ql.set_option('use_default_gates', 'yes')

ql.set_option('log_level', 'LOG_INFO')
ql.set_option('unique_output', 'yes')
ql.set_option('write_qasm_files', 'no')
ql.set_option('write_report_files', 'no')

c = ql.Compiler("testCompiler")
c.add_pass("BackendCompiler")
c.add_pass("Visualizer")

c.set_pass_option("ALL", "skip", "no")
c.set_pass_option("ALL", "write_qasm_files", "no")
c.set_pass_option("ALL", "write_report_files", "no")
c.set_pass_option("Visualizer", "visualizer_type", "MAPPING_GRAPH")
c.set_pass_option("Visualizer", "visualizer_config_path", os.path.join(curdir, "visualizer_config_example1.json"))
c.set_pass_option("Visualizer", "visualizer_waveform_mapping_path", os.path.join(curdir, "waveform_mapping.json"))

platformCustomGates = ql.Platform('starmon', os.path.join(curdir, 'test_s7.json'))
nqubits = 7
p = ql.Program("testProgram1", platformCustomGates, nqubits, 0)
k = ql.Kernel("aKernel1", platformCustomGates, nqubits, 0)

k.gate("x", [2])
k.gate("y", [4])
k.gate("cnot", [2,4])
k.gate("x", [2])
k.gate("y", [4])

p.add_kernel(k)
c.compile(p)