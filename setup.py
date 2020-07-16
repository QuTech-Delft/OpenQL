#!/usr/bin/env python3

# TODO: clean includes once done
import os, platform, shutil, sys, subprocess
from distutils.command.bdist import bdist as _bdist
from distutils.command.sdist import sdist as _sdist
from distutils.command.build import build as _build
from distutils.command.clean import clean as _clean
from setuptools.command.egg_info import egg_info as _egg_info
from setuptools.command.install import install as _install
from setuptools.command.build_ext import build_ext as _build_ext
import distutils.cmd
import distutils.log
from setuptools import setup, Extension, find_packages
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
import re

root_dir   = os.getcwd()                        # root of the repository
src_dir    = root_dir   + os.sep + 'src'        # C++ source directory
target_dir = root_dir   + os.sep + 'pybuild'    # python-specific build directory
build_dir  = target_dir + os.sep + 'build'      # directory for setuptools to dump various files into
dist_dir   = target_dir + os.sep + 'dist'       # wheel output directory
cbuild_dir = target_dir + os.sep + 'cbuild'     # cmake build directory
prefix_dir = target_dir + os.sep + 'prefix'     # cmake install prefix
module_dir = target_dir + os.sep + 'module'     # openql Python module directory, including generated file(s)

def get_version(verbose=0):
    """ Extract version information from source code """

    matcher = re.compile('[\t ]*#define[\t ]+OPENQL_VERSION_STRING[\t ]+"(.*)"')
    version = None
    with open(os.path.join(src_dir, 'version.h'), 'r') as f:
        for ln in f:
            m = matcher.match(ln)
            if m:
                version = m.group(1)
                break

    if verbose:
        print('get_version: %s' % version)

    return version

def read(fname):
    with open(os.path.join(os.path.dirname(__file__), fname)) as f:
        return f.read()

class clean(_clean):
    def run(self):
        _clean.run(self)
        if os.path.exists(target_dir):
            shutil.rmtree(target_dir)

class build_ext(_build_ext):
    def run(self):
        from plumbum import local, FG, ProcessExecutionError

        # Figure out how many parallel processes to build with.
        if self.parallel:
            nprocs = str(self.parallel)
        else:
            nprocs = os.environ.get('NPROCS', '1')

        # Figure out how setuptools wants to name the extension file and where
        # it wants to place it.
        target = os.path.abspath(self.get_ext_fullpath('openql._openql'))

        # Build the Python module and install it into module_dir.
        if not os.path.exists(cbuild_dir):
            os.makedirs(cbuild_dir)
        with local.cwd(cbuild_dir):
            cmd = (local['cmake'][root_dir]
                ['-DOPENQL_BUILD_PYTHON=YES']
                ['-DCMAKE_INSTALL_PREFIX=' + prefix_dir]
                ['-DOPENQL_PYTHON_DIR=' + os.path.dirname(target)]
                ['-DOPENQL_PYTHON_EXT=' + os.path.basename(target)]

                # Make sure CMake uses the Python installation corresponding
                # with the the Python version we're building with now.
                ['-DPYTHON_EXECUTABLE=' + sys.executable]

                # (ab)use static libs for the intermediate libraries to avoid
                # dealing with R(UN)PATH nonsense on Linux/OSX as much as
                # possible.
                ['-DBUILD_SHARED_LIBS=NO']

                # Build type can be set using an environment variable.
                ['-DCMAKE_BUILD_TYPE=' + os.environ.get('OPENQL_BUILD_TYPE', 'Release')]
            )

            # Unitary decomposition can be disabled using an environment
            # variable.
            if 'OPENQL_DISABLE_UNITARY' in os.environ:
                cmd = cmd['-DWITH_UNITARY_DECOMPOSITION=OFF']

            # Run cmake configuration.
            cmd & FG

            # Do the build with the given number of parallel threads.
            cmd = local['cmake']['--build']['.']
            if nprocs != '1':
                try:
                    parallel_supported = tuple(local['cmake']('--version').split('\n')[0].split()[-1].split('.')) >= (3, 12)
                except:
                    parallel_supported = False
                if parallel_supported:
                    cmd = cmd['--parallel'][nprocs]
                elif not sys.platform.startswith('win'):
                    cmd = cmd['--']['-j'][nprocs]
            cmd & FG

            # Do the install.
            try:
                # install target for makefiles
                local['cmake']['--build']['.']['--target']['install'] & FG
            except ProcessExecutionError:
                # install target for MSVC
                local['cmake']['--build']['.']['--target']['INSTALL'] & FG

class build(_build):
    def initialize_options(self):
        _build.initialize_options(self)
        self.build_base = os.path.relpath(build_dir)

    def run(self):
        # Make sure the extension is built before the Python module is "built",
        # otherwise SWIG's generated module isn't included.
        # See https://stackoverflow.com/questions/12491328
        self.run_command('build_ext')
        _build.run(self)

class install(_install):
    def run(self):
        # See https://stackoverflow.com/questions/12491328
        self.run_command('build_ext')
        _install.run(self)

class bdist(_bdist):
    def finalize_options(self):
        _bdist.finalize_options(self)
        self.dist_dir = os.path.relpath(dist_dir)

class bdist_wheel(_bdist_wheel):
    def run(self):
        if platform.system() == "Darwin":
            os.environ['MACOSX_DEPLOYMENT_TARGET'] = '10.10'
        _bdist_wheel.run(self)
        impl_tag, abi_tag, plat_tag = self.get_tag()
        archive_basename = "{}-{}-{}-{}".format(self.wheel_dist_name, impl_tag, abi_tag, plat_tag)
        wheel_path = os.path.join(self.dist_dir, archive_basename + '.whl')
        if platform.system() == "Darwin":
            from delocate.delocating import delocate_wheel
            delocate_wheel(wheel_path)

class sdist(_sdist):
    def finalize_options(self):
        _sdist.finalize_options(self)
        self.dist_dir = os.path.relpath(dist_dir)

class egg_info(_egg_info):
    def initialize_options(self):
        _egg_info.initialize_options(self)
        self.egg_base = os.path.relpath(target_dir)

setup(
    name='qutechopenql',
    version=get_version(),
    description='OpenQL Python Package',
    long_description=read('README.md'),
    long_description_content_type = 'text/markdown',
    author='QuTech, TU Delft',
    url='https://github.com/QE-Lab/OpenQL',
    license=read('LICENSE'),

    classifiers = [
        'License :: OSI Approved :: Apache Software License',

        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS',
        'Operating System :: Microsoft :: Windows',

        'Programming Language :: Python :: 3 :: Only',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',

        'Topic :: Scientific/Engineering'
    ],

    packages = ['openql'],
    package_dir = {'': 'python'},

    # NOTE: the library build process is completely overridden to let CMake
    # handle it; setuptools' implementation is horribly broken. This is here
    # just to have the rest of setuptools understand that this is a Python
    # module with an extension in it.
    ext_modules = [
        Extension('openql._openql', [])
    ],

    cmdclass = {
        'bdist': bdist,
        'bdist_wheel': bdist_wheel,
        'build_ext': build_ext,
        'build': build,
        'install': install,
        'clean': clean,
        'egg_info': egg_info,
        'sdist': sdist,
    },

    extras_require={'develop': ['pytest', 'numpy']},
    setup_requires = [
        'plumbum',
        'delocate; platform_system == "Darwin"',
    ],
    zip_safe=False
)
