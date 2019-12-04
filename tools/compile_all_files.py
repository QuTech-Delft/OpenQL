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
	log_level = 'LOG_WARNING'
	scheduler = 'ALAP'
	mapper = 'minextendrc'
	optimize = 'no'
	scheduler_uniform = 'no'
	initialplace = 'no'
	scheduler_post179 = 'yes'
	scheduler_commute = 'yes'
	mapusemoves = 'no'
	maptiebreak = 'random'

	output_dir_name = 'test_output/mapper=' + mapper

	#add other options here (overring the options above will not work! change the value in the options above instead!)
	ql.set_option('decompose_toffoli', "no")
	ql.set_option("prescheduler", "yes") 
	ql.set_option("cz_mode", "manual") # = "manual";
	ql.set_option("clifford_premapper", "yes") # = "yes";
	ql.set_option("clifford_postmapper", "yes") # = "yes";
	ql.set_option("mapinitone2one", "yes") # = "yes";
	ql.set_option("mapassumezeroinitstate", "no") # = "no";
	ql.set_option("initialplace", "no") # = "no";
	ql.set_option("initialplace2qhorizon", "0") # = "0";
	ql.set_option("maplookahead", "noroutingfirst") # = "noroutingfirst";
	ql.set_option("mappathselect", "all") # = "all";
	ql.set_option("maprecNN2q", "no") # = "no";
	ql.set_option("mapselectmaxlevel", "0") # = "0";
	ql.set_option("mapselectmaxwidth", "min") # = "min";
	ql.set_option("mapselectswaps", "all") # = "all";
	ql.set_option("mapreverseswap", "yes") # = "yes";



	for file in files:
		imported = importlib.import_module(os.path.join(file.replace(".py", "")))
		try:
			imported.circuit('test_mapper17.json', scheduler = scheduler, mapper = mapper, uniform_sched = scheduler_uniform, new_scheduler = scheduler_post179,  moves = mapusemoves, maptiebreak = maptiebreak, measurement = measurement, optimize = optimize, output_dir_name = output_dir_name, log_level = log_level)
		except Exception as e:
			print(str(e))

