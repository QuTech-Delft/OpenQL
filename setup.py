import os
import re
from setuptools import setup
from shutil import copyfile
import subprocess
from sys import platform

rootDir = os.path.dirname(os.path.realpath(__file__))
srcDir = os.path.join(rootDir, "src")
buildDir = os.path.join(rootDir, "cbuild")
clibDir = os.path.join(buildDir, "swig")

nprocs = 1
env_var_nprocs = os.environ.get('NPROCS')
if (env_var_nprocs != None):
  nprocs = int(env_var_nprocs)

print('Using {} processes for compilation'.format(nprocs))
if nprocs == 1:
    print('For faster compilation by N processes, set environment variable NPROCS=N')
    print('For example: NPROCS=4 python3 setup.py install --user')

if not os.path.exists(buildDir):
    os.makedirs(buildDir)
os.chdir(buildDir)
#  
if platform == "linux" or platform == "linux2":
    print('Detected Linux OS, installing openql ... ')
    cmd = 'cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/python3 ..'
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    cmd = 'make -j{}'.format(nprocs)
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    clibname = "_openql.so"

elif platform == "darwin":
    print('Detected OSX, installing openql ... ')
    os.environ["MACOSX_DEPLOYMENT_TARGET"] = "10.10"
    cmd = 'cmake ..'
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    cmd = 'make -j{}'.format(nprocs)
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    clibname = "_openql.so"

elif platform == "win32":
    print('Detected Windows OS, installing openql ... ')
    print(os.path)
    cmd = 'cd ../deps/eigen/ && git apply ../../patches/eigen.patch'
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    os.chdir(buildDir)
    cmd = 'cmake -G "NMake Makefiles" ..'
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    # cmd = 'nmake /M/P{}'.format(nprocs)
    cmd = 'nmake'
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    cmd = 'cd ../deps/eigen/ && git checkout Eigen/src/misc/lapacke.h'
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    os.chdir(buildDir)
    clibname = "_openql.pyd"

else:
    print('Unknown/Unsupported OS !!!')

clib = os.path.join(clibDir, clibname)
swigDir = os.path.join(rootDir, "swig", "openql")
clibSwig = os.path.join(swigDir, clibname)

copyfile(clib, clibSwig)
copyfile(os.path.join(clibDir, "openql.py"),
         os.path.join(swigDir, "openql.py"))
os.chdir(rootDir)


def get_version(verbose=0):
    """ Extract version information from source code """

    matcher = re.compile('[\t ]*#define[\t ]+OPENQL_VERSION_STRING[\t ]+"(.*)"')
    version = None
    try:
        with open(os.path.join(srcDir, 'version.h'), 'r') as f:
            for ln in f:
                m = matcher.match(ln)
                if m:
                    version = m.group(1)
                    break

    except Exception as E:
        print(E)
        version = 'none'

    if verbose:
        print('get_version: %s' % version)

    return version


def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()


try:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
    class bdist_wheel(_bdist_wheel):
        def finalize_options(self):
            _bdist_wheel.finalize_options(self)
            self.root_is_pure = False
except ImportError:
    bdist_wheel = None

setup(name='qutechopenql',
      version=get_version(),
      description='OpenQL Python Package',
      long_description=read('README.md'),
      author='Nader Khammassi and Imran Ashraf',
      author_email='nader.khammassi@gmail.com, iimran.aashraf@gmail.com',
      url='https://github.com/QE-Lab/OpenQL',
      license=read('LICENSE'),
      packages=['openql'],
      cmdclass={'bdist_wheel': bdist_wheel},
      package_dir={'': 'swig'},
      include_package_data=True,
      package_data={'openql': [clibSwig]},
      extras_require={'develop': ['pytest', 'numpy']},
      zip_safe=False)
