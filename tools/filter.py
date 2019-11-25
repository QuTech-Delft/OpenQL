import os
import time

curdir = os.getcwd()
indir = 'test_files/test_vault/less_than_10'

print("\n\nFiltering files in: ", indir, " ?...\n\n")
input()

os.chdir(os.path.join(curdir, indir))
files = os.listdir()

max_qubits = 10
max_size = 100 #in KB


for file in files:
	if not '.py' in file or file == 'filter.py':
		continue
	flag_rewrite = False
	flag_was_deleted = False
	with open(file, 'r') as fopen:
		text = fopen.readlines()
	for line in reversed(text): #reversed so that removing elements doesn't screw up the loop
		if 'num_qubits = ' in line:
			num_qubits = line.split('num_qubits = ')[1]
			if int(num_qubits) > max_qubits or os.path.getsize(file) > max_size*1024:
				os.remove(file)
				print('Deleted: ', file)
				flag_was_deleted = True
				break
		if 'sweep_points' in line:
			flag_rewrite = True
			text.remove(line)
		if 'prepz' in line:
			flag_rewrite = True
			text.remove(line)
	if flag_rewrite and not flag_was_deleted:
		with open(file, 'w') as fopen:
			fopen.writelines(text)
	
