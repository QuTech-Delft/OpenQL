import openql as ql
import os

from config import json_visualizer_dir, output_dir


ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')

ql.set_option('clifford_premapper', 'yes')
ql.set_option('clifford_postmapper', 'yes')
ql.set_option('mapper', 'minextend')
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

platformCustomGates = ql.Platform('starmon',  os.path.join(json_visualizer_dir, 'test_s7.json'))
num_qubits = 7
p = ql.Program("testProgram1", platformCustomGates, num_qubits, 0)
k = ql.Kernel("aKernel1", platformCustomGates, num_qubits, 0)

k.gate("x", [2])
k.gate("y", [4])
k.gate("cnot", [2, 4])
k.gate("x", [2])
k.gate("y", [4])

p.add_kernel(k)

p.get_compiler().insert_pass_after(
    'mapper',
    'ana.visualize.Mapping',
    'after_mapping', {
        'config':  os.path.join(json_visualizer_dir, 'config_example_1.json'),
        'output_prefix': output_dir + '/%N_after',
        'interactive': 'yes'
    }
)

p.compile()
