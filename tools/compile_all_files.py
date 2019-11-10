import glob
import os
import importlib
import argparse
from openql import openql as ql
import sys



if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Validate OpenQL compiler')
	# parser.add_argument('indir', type=str, help='Input dir for circuits')
	parser.add_argument('indir', type=str, default= 'no', help='Input dir for circuits')
	# parser.add_argument('--outdir', type=str, default= 'no', help='Input dir for circuits')
	args = parser.parse_args()

	indir = args.indir
	# if not args.outdir:
	# 	outdir = os.path.join(indir, "/output")

	curdir = os.getcwd()
	# os.chdir(os.path.join(curdir, indir))
	files = glob.glob('./test_files/*.py')
	files = list(map(os.path.basename, files))
	files = list(map(lambda x: x.replace('.py', ''), files))
	# os.chdir(curdir)
	
	sys.path.append(os.path.join(curdir, indir))

	if __file__ in files:
		files.remove(__file__)

	ql.set_option('quantumsim', 'qsoverlay')
	ql.set_option('write_report_files', 'yes')
	ql.set_option('write_qasm_files', 'yes')
	print(files)
	for file in files:
		imported = importlib.import_module(os.path.join(file.replace(".py", "")))
		try:
			imported.circuit('test_mapper17.json', scheduler = 'ALAP', moves='yes', measurement = False, output_dir_name = 'test_output')
		except:
			continue

