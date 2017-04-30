import os 
from setuptools import setup
from shutil import copyfile
import subprocess
from sys import platform


rootDir = os.path.dirname(os.path.realpath(__file__))
buildDir = os.path.join(rootDir, "cbuild" )
myclibDir = os.path.join(buildDir, "openql")

if not os.path.exists(buildDir):
    os.makedirs(buildDir)
os.chdir(buildDir)

if platform == "linux" or platform == "linux2":
    print('Detected Linux OS')
    cmd = 'cmake ..'
    ret = subprocess.check_output(cmd, shell=True)
    cmd = 'make'
    ret = subprocess.check_output(cmd, shell=True)
    genclib = os.path.join(myclibDir, "_openql.so")
    myclib = os.path.join(rootDir, "openql", "_openql.so")

elif platform == "darwin":
	print('Detected OSX but its not yet tested!')


elif platform == "win32":
    print('Detected Windows OS.')
    cmd = 'cmake -G "NMake Makefiles" ..'
    ret = subprocess.check_output(cmd, shell=True)
    cmd = 'nmake'
    ret = subprocess.check_output(cmd, shell=True)
    genclib = os.path.join(myclibDir, "_openql.pyd")
    myclib = os.path.join(rootDir, "openql", "_openql.pyd")

else:
	print('Unknown/Unsupported OS !!!')

copyfile(genclib, myclib)
copyfile( os.path.join(myclibDir, "openql.py"), os.path.join(rootDir, "openql", "openql.py") )
os.chdir(rootDir)

	
setup(name='openql',
      version='0.1',
      description='OpenQL Python Package',
      author='I. Ashraf',
      author_email='iimran.aashraf@gmail.com',
      packages=['openql'],
      package_data={'openql': [myclib]},
      zip_safe=False)

os.remove(myclib)
