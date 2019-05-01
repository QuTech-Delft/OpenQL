import os
import re
from setuptools import setup
from shutil import copyfile
import subprocess
from sys import platform

rootDir = os.path.dirname(os.path.realpath(__file__))
srcDir = os.path.join(rootDir, "ql")
buildDir = os.path.join(rootDir, "cbuild")
clibDir = os.path.join(buildDir, "openql")

nprocs = 1
env_var_nprocs = os.environ.get('NPROCS')
if(env_var_nprocs != None):
  nprocs = int(env_var_nprocs)

print('Using {} processes for compilation'.format(nprocs))
if nprocs == 1:
    print('For faster compilation by N processes, set environment variable NPROCS=N')
    print('For example: NPROCS=4 python3 setup.py install --user')

if not os.path.exists(buildDir):
    os.makedirs(buildDir)
os.chdir(buildDir)

if platform == "linux" or platform == "linux2":
    print('Detected Linux OS, installing openql ... ')
    cmd = 'cmake ..'
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
    cmd = 'cmake -G "NMake Makefiles" ..'
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    cmd = 'nmake /MP{}'.format(nprocs)
    proc = subprocess.Popen(cmd, shell=True)
    proc.communicate()
    clibname = "_openql.pyd"

else:
    print('Unknown/Unsupported OS !!!')

genclib = os.path.join(clibDir, clibname)
clib = os.path.join(rootDir, "openql", clibname)
copyfile(genclib, clib)
copyfile(os.path.join(clibDir, "openql.py"),
         os.path.join(rootDir, "openql", "openql.py"))
os.chdir(rootDir)


def get_version(verbose=0):
    """ Extract version information from source code """

    matcher = re.compile('[\t ]*#define[\t ]+OPENQL_(MAJOR|MINOR|PATCH)_VERSION[\t ]+(.*)')

    openql_major_version = None
    openql_minor_version = None
    openql_patch_version = None

    version = None
    try:
        with open(os.path.join(srcDir, 'version.h'), 'r') as f:
            for ln in f:
                m = matcher.match(ln)
                if m:
                    version_val = int(m.group(2))
                    if m.group(1) == 'MAJOR':
                        openql_major_version = version_val
                    elif m.group(1) == 'MINOR':
                        openql_minor_version = version_val
                    else:
                        openql_patch_version = version_val

                        if ((openql_major_version is not None) and
                            (openql_minor_version is not None) and
                            (openql_patch_version is not None)):
                            version = '{}.{}.{}'.format(openql_major_version,
                                              openql_minor_version,
                                              openql_patch_version)
                            break;

    except Exception as E:
        print(E)
        version = 'none'

    if verbose:
        print('get_version: %s' % version)

    return version

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(name='openql',
      version=get_version(),
      description='OpenQL Python Package',
      long_description=read('README.md'),
      author='Nader Khammassi and Imran Ashraf',
      author_email='nader.khammassi@gmail.com, iimran.aashraf@gmail.com',
      url='https://github.com/QE-Lab/OpenQL',
      license=read('license'),
      packages=['openql'],
      include_package_data=True,
      package_data={'openql': [clib]},
      zip_safe=False)
