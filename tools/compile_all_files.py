import glob
import os
import importlib
import argparse
from openql import openql as ql
import sys
from io import StringIO
from joblib import Parallel, delayed, cpu_count

#very confusing function!
def generate_new_dest_dir(input, mapper):
	folders = [name for name in os.listdir(input) if os.path.isdir(os.path.join(input,name)) and ("mapper="+mapper in name)]
	if folders:
		new_number = max([int(folder.split("mapper="+mapper+"_")[1]) for folder in folders])+1
	else:
		new_number = 1
	return "mapper=" + mapper + "_" + str(new_number)


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
	log_level = 'LOG_NOTHING'
	scheduler = 'ALAP'
	mapper = 'minextendrc'
	optimize = 'no'
	scheduler_uniform = 'no'
	initialplace = 'no'
	scheduler_post179 = 'yes'
	scheduler_commute = 'yes'
	mapusemoves = 'no'
	maptiebreak = 'first'

	# output_dir_name = 'test_output/mapper=' + mapper

	#add other options here (overring the options above will not work! change the value in the options above instead!)
	ql.set_option("maxfidelity_1qbgatefid", "0.999")
	ql.set_option("maxfidelity_2qbgatefid", "0.99")
	ql.set_option("maxfidelity_idlefid", "0.9867")
	ql.set_option("maxfidelity_outputmode", "worst")

	ql.set_option('decompose_toffoli', "no") # = "no";
	ql.set_option("prescheduler", "yes") # = "yes";
	ql.set_option("cz_mode", "manual") # = "manual";
	ql.set_option("clifford_premapper", "yes") # = "yes";
	ql.set_option("clifford_postmapper", "yes") # = "yes";
	ql.set_option("mapinitone2one", "yes") # = "yes";
	ql.set_option("mapassumezeroinitstate", "no") # = "no";
	ql.set_option("initialplace2qhorizon", "0") # = "0";
	ql.set_option("maplookahead", "noroutingfirst") # = "noroutingfirst";
	ql.set_option("mappathselect", "all") # = "all";
	ql.set_option("maprecNN2q", "no") # = "no";
	ql.set_option("mapselectmaxlevel", "0") # = "0";
	ql.set_option("mapselectmaxwidth", "min") # = "min";
	ql.set_option("mapselectswaps", "all") # = "all";
	ql.set_option("mapreverseswap", "yes") # = "yes";


	#==== Create output folder
	all_results_dir = "test_output"
	result_dir = generate_new_dest_dir(os.path.join(indir, all_results_dir), mapper)
	output_dir_name = os.path.join(all_results_dir, result_dir)
	if not os.path.exists(os.path.join(indir, output_dir_name)):
		os.makedirs(os.path.join(indir, output_dir_name))
	print(result_dir)
	print(os.path.join(indir, output_dir_name))

	print("mapper: " +           ql.get_option("mapper") + '\n')

	#=== Serial Compilation
	for file in files:
		imported = importlib.import_module(os.path.join(file.replace(".py", "")))
		try:
			imported.circuit('test_mapper17.json', scheduler = scheduler, mapper = mapper, uniform_sched = scheduler_uniform, new_scheduler = scheduler_post179,  moves = mapusemoves, maptiebreak = maptiebreak, measurement = measurement, optimize = optimize, initial_placement = initialplace, output_dir_name = output_dir_name, log_level = log_level)
		except Exception as e:
			print(str(e))

	# === Parallel Compilation (does not work! Somehow, the ql.set_options don't take effect within each of the worker processes)
	# def parallel_import_compile(file, scheduler, mapper, scheduler_uniform, scheduler_post179, mapusemoves, maptiebreak, measurement, optimize, output_dir_name, log_level):
	# 	imported = importlib.import_module(os.path.join(file.replace(".py", "")))
	# 	try:
	# 		imported.circuit('test_mapper17.json', scheduler = scheduler, mapper = mapper, uniform_sched = scheduler_uniform, new_scheduler = scheduler_post179,  moves = mapusemoves, maptiebreak = maptiebreak, measurement = measurement, optimize = optimize, initial_placement = initialplace, output_dir_name = output_dir_name, log_level = log_level)
	# 	except Exception as e:
	# 		print(str(e))

	# def parallel_import_compile_v2(files):
	# 	imported_list = [importlib.import_module(os.path.join(file.replace(".py", ""))) for file in files]
	# 	def _parallel_import_compile_v2(imported):
	# 		try:
	# 			imported.circuit('test_mapper17.json', scheduler = scheduler, mapper = mapper, uniform_sched = scheduler_uniform, new_scheduler = scheduler_post179,  moves = mapusemoves, maptiebreak = maptiebreak, measurement = measurement, optimize = optimize, initial_placement = initialplace, output_dir_name = output_dir_name, log_level = log_level)
	# 		except Exception as e:
	# 			print(str(e))
	# 	Parallel(n_jobs=4, verbose=8)(delayed(_parallel_import_compile_v2)(imported) for imported in imported_list)


	
	# Parallel(n_jobs=4, verbose=8)(delayed(parallel_import_compile)(file, scheduler, mapper, scheduler_uniform, scheduler_post179, mapusemoves, maptiebreak, measurement, optimize, output_dir_name, log_level) for file in files)
	# parallel_import_compile_v2(files)	

	#==== DO NOT TOUCH HERE (meant to provide some redundancy, so that the parameters variable won't stop working when parallel compilation is fixed):
	#This is because these parameters are only set within each file from qbench
	ql.set_option("log_level", log_level)
	ql.set_option("scheduler", scheduler)
	ql.set_option("mapper", mapper)
	ql.set_option("optimize", optimize)
	ql.set_option("scheduler_uniform", scheduler_uniform)
	ql.set_option("initialplace", initialplace)
	ql.set_option("scheduler_post179", scheduler_post179)
	ql.set_option("scheduler_commute", scheduler_commute)
	ql.set_option("mapusemoves", mapusemoves)
	ql.set_option("maptiebreak", maptiebreak)

	#============== Store the options in a file
	parameters = ["unique_output: " + ql.get_option("unique_output") + '\n'  ,
				  "optimize: " + ql.get_option("optimize") + '\n'  ,
				  "use_default_gates: " + ql.get_option("use_default_gates") + '\n'  ,
				  "decompose_toffoli: " + ql.get_option("decompose_toffoli") + '\n'  ,
				  "quantumsim: " + ql.get_option("quantumsim") + '\n'  ,
				  "prescheduler: " + ql.get_option("prescheduler") + '\n'  ,
				  "scheduler: " + ql.get_option("scheduler") + '\n'  ,
				  "scheduler_uniform: " + ql.get_option("scheduler_uniform") + '\n'  ,
				  "clifford_premapper: " + ql.get_option("clifford_premapper") + '\n'  ,
				  "mapper: " +           ql.get_option("mapper") + '\n'  ,
				  "mapinitone2one: " +   ql.get_option("mapinitone2one") + '\n'  ,
				  "initialplace: " +     ql.get_option("initialplace") + '\n'  ,
				  "initialplace2qhorizon: " +ql.get_option("initialplace2qhorizon") + '\n'  ,
				  "maplookahead: " +     ql.get_option("maplookahead") + '\n'  ,
				  "mappathselect: " +    ql.get_option("mappathselect") + '\n'  ,
				  "maptiebreak: " +      ql.get_option("maptiebreak") + '\n'  ,
				  "mapusemoves: " +      ql.get_option("mapusemoves") + '\n'  ,
				  "mapreverseswap: " +   ql.get_option("mapreverseswap") + '\n'  ,
				  "mapselectswaps: " +   ql.get_option("mapselectswaps") + '\n'  ,
				  "clifford_postmapper: " + ql.get_option("clifford_postmapper") + '\n'  ,
				  "scheduler_post179: " + ql.get_option("scheduler_post179") + '\n'  ,
				  "scheduler_commute: " + ql.get_option("scheduler_commute") + '\n'  ,
				  "cz_mode: " + ql.get_option("cz_mode") + '\n'  ,
				  "mapassumezeroinitstate: " + ql.get_option("mapassumezeroinitstate") + '\n' ,
				  "maprecNN2q: " + ql.get_option("maprecNN2q") + '\n' ,
				  "mapselectmaxlevel: " + ql.get_option("mapselectmaxlevel") + '\n' ,
				  "mapselectmaxwidth: " + ql.get_option("mapselectmaxwidth") + '\n' ,

				  "measurement: " + ('yes' if measurement else 'no') + '\n' ,				  

				  "maxfidelity_1qbgatefid: " + ql.get_option("maxfidelity_1qbgatefid") + '\n'  , 
				  "maxfidelity_2qbgatefid: " + ql.get_option("maxfidelity_2qbgatefid") + '\n'  , 
				  "maxfidelity_idlefid: "	 + ql.get_option("maxfidelity_idlefid") + '\n'   ,
				  "maxfidelity_outputmode: " + ql.get_option("maxfidelity_outputmode") + '\n'
	]
	print("Wrote files to:", output_dir_name)
	print("Writing parameters to:", 'test_files', output_dir_name,'parameters.txt')
	with open(os.path.join('test_files', output_dir_name,'parameters.txt'), 'w') as fopen:
		fopen.writelines(parameters)