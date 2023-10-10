#!/usr/bin/env python3

import os
import platform
import shutil
import re
import sys
from setuptools import setup, Extension
from distutils.dir_util import copy_tree

from distutils.command.clean import clean as _clean
from setuptools.command.build_ext import build_ext as _build_ext
from distutils.command.build import build as _build
from setuptools.command.install import install as _install
from distutils.command.bdist import bdist as _bdist
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
from distutils.command.sdist import sdist as _sdist
from setuptools.command.egg_info import egg_info as _egg_info

root_dir = os.getcwd()  # root of the repository
src_dir = root_dir + os.sep + 'src'  # C++ source directory
inc_dir = root_dir + os.sep + 'include'  # C++ include directory
pysrc_dir = root_dir + os.sep + 'python'  # Python source files
target_dir = root_dir + os.sep + 'pybuild'  # python-specific build directory
build_dir = target_dir + os.sep + 'build'  # directory for setuptools to dump various files into
dist_dir = target_dir + os.sep + 'dist'  # wheel output directory
cbuild_dir = target_dir + os.sep + 'cbuild'  # cmake build directory
prefix_dir = target_dir + os.sep + 'prefix'  # cmake install prefix
srcmod_dir = pysrc_dir + os.sep + 'openql'  # openql Python module directory, source files only
module_dir = target_dir + os.sep + 'openql'  # openql Python module directory for editable install

# Copy the handwritten Python sources into the module directory that we're telling setuptools is our source directory,
# because setuptools insists on spamming output files into that directory.
# This is ugly, especially because it has to run before setup() is invoked,
# but seems to be more-or-less unavoidable to get editable installations to work.
if not os.path.exists(target_dir):
    os.makedirs(target_dir)
copy_tree(srcmod_dir, module_dir)


def get_version(verbose=0):
    """ Extract version information from source code """

    matcher = re.compile('[\t ]*#define[\t ]+OPENQL_VERSION_STRING[\t ]+"(.*)"')
    version = None
    with open(os.path.join(inc_dir, 'ql', 'version.h'), 'r') as f:
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

        # If we were previously built in a different directory,
        # nuke the cbuild dir to prevent inane CMake errors.
        # This happens when the user does 'pip install .' after building locally
        if os.path.exists(cbuild_dir + os.sep + 'CMakeCache.txt'):
            with open(cbuild_dir + os.sep + 'CMakeCache.txt', 'r') as f:
                for line in f.read().split('\n'):
                    line = line.split('#')[0].strip()
                    if not line:
                        continue
                    if line.startswith('OpenQL_BINARY_DIR:STATIC'):
                        config_dir = line.split('=', maxsplit=1)[1]
                        if os.path.realpath(config_dir) != os.path.realpath(cbuild_dir):
                            print('removing pybuild/cbuild to avoid CMakeCache error')
                            shutil.rmtree(cbuild_dir)
                        break

        # Figure out how setuptools wants to name the extension file and where it wants to place it
        target = os.path.abspath(self.get_ext_fullpath('openql._openql'))

        # Build the Python extension and install it where setuptools expects it
        if not os.path.exists(cbuild_dir):
            os.makedirs(cbuild_dir)

        # Configure and build using Conan
        with local.cwd(root_dir):
            # Build type can be set using an environment variable.
            build_type = os.environ.get('OPENQL_BUILD_TYPE', 'Release')
            build_tests = 'True' if 'OPENQL_BUILD_TESTS' in os.environ else 'False'
            disable_unitary = 'True' if 'OPENQL_DISABLE_UNITARY' in os.environ else 'False'

            cmd = local['conan']['profile']['detect']['--force']
            cmd & FG

            cmd = (local['conan']['create']['.']
                ['--version']['0.11.2']
                ['-s:h']['compiler.cppstd=23']
                ['-s:h']["openql/*:build_type=" + build_type]

                # C++ tests can be enabled using an environment variable. They'll be run before the install
                ['-o']['openql/*:build_python=True']
                ['-o']['openql/*:build_tests=' + build_tests]
                # Do not include debug symbols in the wheels
                ['-o']["openql/*:debug_symbols=False"]
                # Unitary decomposition can be disabled using an environment variable
                ['-o']['openql/*:disable_unitary=' + disable_unitary]
                ['-o']['openql/*:python_dir=' + os.path.dirname(target).replace("\\", "\\\\")]
                ['-o']['openql/*:python_executable=' + sys.executable.replace("\\", "\\\\")]
                ['-o']['openql/*:python_ext=' + os.path.basename(target).replace("\\", "\\\\")]
                # (Ab)use static libs for the intermediate libraries
                # to avoid dealing with R(UN)PATH nonsense on Linux/OSX as much as possible
                ['-o']["openql/*:shared=False"]

                ['-b']['missing']
                ['-tf']['']
            )
            if platform.system() == "Darwin":
                cmd = cmd['-c']['tools.build:defines=["_LIBCPP_DISABLE_AVAILABILITY"]']
            cmd & FG


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
    url='https://github.com/QuTech-Delft/OpenQL',

    classifiers=[
        'License :: OSI Approved :: Apache Software License',

        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS',
        'Operating System :: Microsoft :: Windows',

        'Programming Language :: Python :: 3 :: Only',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',

        'Topic :: Scientific/Engineering'
    ],

    packages=['openql'],
    package_dir={'': 'pybuild'},

    # NOTE: the library build process is completely overridden to let CMake handle it.
    # setuptools implementation is horribly broken.
    # This is here just to have the rest of setuptools understand that
    # this is a Python module with an extension in it.
    ext_modules=[
        Extension('openql._openql', [])
    ],

    cmdclass={
        'bdist': bdist,
        'bdist_wheel': bdist_wheel,
        'build_ext': build_ext,
        'build': build,
        'install': install,
        'clean': clean,
        'egg_info': egg_info,
        'sdist': sdist,
    },

    setup_requires=[
        'conan',
        'plumbum',
        'delocate; platform_system == "Darwin"',
    ],
    install_requires=[
        'msvc-runtime; platform_system == "Windows"',
    ],
    tests_require=[
        'pytest', 'numpy'
    ],

    zip_safe=False
)
