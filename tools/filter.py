import os

curdir = os.getcwd()
indir = 'test_files'

os.chdir(os.path.join(curdir, indir))
files = os.listdir()


for file in files:
	if not '.py' in file:
		continue
	with open(file, 'r') as fopen:
		text = fopen.readlines()
	for line in text:
		if 'num_qubits = ' in line:
			num_qubits = line.split('num_qubits = ')[1]
			if int(num_qubits) > 17:
				os.remove(file)
				print('Deleted: ', file)
				break
