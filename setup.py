import os 
from setuptools import setup
import subprocess
from sys import platform

rootDir = os.path.dirname(os.path.realpath(__file__))
myclibDir = os.path.join(rootDir, "api", "openql", "csrc", "build" )

if platform == "linux" or platform == "linux2":
    print('Detected Linux OS')
    myclib = os.path.join(myclibDir, "libapi.so")
    print(myclib)
    cmd = 'mkdir -p {0}'.format(myclibDir)
    ret = subprocess.check_output(cmd, shell=True)
    os.chdir(myclibDir)
    cmd = 'cmake ..'
    ret = subprocess.check_output(cmd, shell=True)
    cmd = 'make'
    ret = subprocess.check_output(cmd, shell=True)
    os.chdir(rootDir)


elif platform == "darwin":
	print('Detected OSX but its not yet tested!')

elif platform == "win32":
    print('Detected Windows OS.')
    myclib = os.path.join(myclibDir, "api.dll")
    print(myclib)
    cmd = 'mkdir -p {0}'.format(myclibDir)
    ret = subprocess.check_output(cmd, shell=True)
    os.chdir(myclibDir)
    cmd = 'cmake -G "NMake Makefiles" ..'
    ret = subprocess.check_output(cmd, shell=True)
    cmd = 'nmake'
    ret = subprocess.check_output(cmd, shell=True)
    os.chdir(rootDir)

else:
	print('Unknown/Unsupported OS !!!')

	
setup(name='openql',
      version='0.1',
      description='OpenQL Python Package',
      author='I. Ashraf',
      author_email='iimran.aashraf@gmail.com',
      packages=['openql'],
      package_dir={'': 'api'},
      install_requires=["cffi>=1.0.0"],
      setup_requires=["cffi>=1.0.0"],
      package_data={'openql': [myclib]},
      zip_safe=False)
