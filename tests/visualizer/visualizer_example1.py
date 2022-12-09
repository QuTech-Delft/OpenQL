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

platformCustomGates = ql.Platform('starmon', os.path.join(curdir, 'test_s7.json'))
nqubits = 7
p = ql.Program("testProgram1", platformCustomGates, nqubits, 0)
k = ql.Kernel("aKernel1", platformCustomGates, nqubits, 0)

k.gate("x", [2])
k.gate("y", [3])
k.gate("cnot", [2,3])
k.gate("x", [2])
k.gate("y", [3])

p.add_kernel(k)

k = ql.Kernel("aKernel2", platformCustomGates, nqubits, 0)

k.gate("x", [2])
k.gate("y", [3])
k.gate("cnot", [2,3])
k.gate("x", [2])
k.gate("y", [3])

p.add_kernel(k)

p.get_compiler().insert_pass_before(
    'mapper',
    'ana.visualize.Mapping',
    'before_mapping', {
        'config': os.path.join(curdir, "visualizer_config_example1.json"),
        'output_prefix': output_dir + '/%N_before',
        'interactive': 'yes'
    }
)

p.get_compiler().insert_pass_after(
    'mapper',
    'ana.visualize.Mapping',
    'after_mapping', {
        'config': os.path.join(curdir, "visualizer_config_example1.json"),
        'output_prefix': output_dir + '/%N_after',
        'interactive': 'yes'
    }
)

p.compile()
