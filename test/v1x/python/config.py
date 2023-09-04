import os

cur_dir = os.path.dirname(os.path.realpath(__file__))
root_dir = os.path.join(cur_dir, '..', '..', '..')
res_dir = os.path.join(root_dir, 'res', 'v1x')

cq_dir = os.path.join(res_dir, 'cq')
cq_golden_dir = os.path.join(cq_dir, 'golden')

json_dir = os.path.join(res_dir, 'json')

qasm_dir = os.path.join(res_dir, 'qasm')
qasm_golden_dir = os.path.join(qasm_dir, 'golden')

output_dir = "test_output"
