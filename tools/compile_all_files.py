import glob
import os
import importlib
import argparse
from openql import openql as ql
import sys



if __name__ == "__main__":
	# parser = argparse.ArgumentParser(description='Validate OpenQL compiler')
	# # parser.add_argument('indir', type=str, help='Input dir for circuits')
	# parser.add_argument('indir', type=str, default= 'no', help='Input dir for circuits')
	# args = parser.parse_args()

	# indir = args.indir
	indir = './test_files/'
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


#VARIOUS OPTIONS

	#Measurement
	measurement = False

	#Some compiler options
	log_level = 'LOG_INFO'
	scheduler = 'ALAP'
	mapper = 'minextendrc'
	optimize = 'yes'
	scheduler_uniform = 'no'
	initialplace = 'no'
	scheduler_post179 = 'yes'
	scheduler_commute = 'yes'
	mapusemoves = 'no'
	maptiebreak = 'random'

	#add other options here
	ql.set_option('decompose_toffoli', 'no')


	for file in files:
		imported = importlib.import_module(os.path.join(file.replace(".py", "")))
		try:
			imported.circuit('test_mapper17.json', scheduler = scheduler, mapper = mapper, uniform_sched = scheduler_uniform, new_scheduler = scheduler_post179,  moves = mapusemoves, maptiebreak = maptiebreak, measurement = measurement, optimize = optimize, output_dir_name = 'test_output', log_level = log_level)
		except Exception as e:
			print(str(e))

