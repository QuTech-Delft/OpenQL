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

src_dir = os.getcwd() + os.sep + 'src'          # source directory for C++
python_dir = os.getcwd() + os.sep + 'python'    # source directory for Python
pybuild_dir = os.getcwd() + os.sep + 'pybuild'  # python-specific build directory
build_dir = pybuild_dir + os.sep + 'build'      # directory for setuptools to dump various files into
dist_dir = pybuild_dir + os.sep + 'dist'        # wheel output directory
cbuild_dir = os.getcwd() + os.sep + 'cbuild'    # cmake build directory
prefix_dir = pybuild_dir + os.sep + 'prefix'    # cmake install prefix
swig_dir = pybuild_dir + os.sep + 'swig'        # swig output directory
module_dir = pybuild_dir + os.sep + 'module'    # openql Python module directory, including generated file(s)

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
        if os.path.exists(cbuild_dir):
            shutil.rmtree(cbuild_dir)
        if os.path.exists(pybuild_dir):
            shutil.rmtree(pybuild_dir)

class build_ext(_build_ext):
    def run(self):
        from plumbum import local, FG

        # Figure out how many parallel processes to build with.
        if self.parallel:
            nprocs = str(self.parallel)
        else:
            nprocs = os.environ.get('NPROCS', '1')

        # Build the OpenQL C++ library using CMake and install it into
        # prefix_dir.
        if not os.path.exists(cbuild_dir):
            os.makedirs(cbuild_dir)
        with local.cwd(cbuild_dir):
            (local['cmake']['..']
                ['-DCMAKE_INSTALL_PREFIX=' + prefix_dir]
                ['-DBUILD_SHARED_LIBS=YES'] # Transitive dependencies don't work with static libs
                ['-DCMAKE_SKIP_RPATH=YES'] # see below (trigger warning!)
            ) & FG
            # We need to handle finding the shared objects manually at runtime.
            # Normally, CMake will set the RPATH/RUNPATH key of the libraries
            # it installs to the install directory, so it should find them even
            # if they're not installed into a system directory and
            # LD_LIBRARY_PATH isn't appropriately set by the user. However,
            # CMake needs to know the exact absolute path where the files will
            # end up in order to do this correctly. Since this is dynamically
            # handled by Python and the instal; prefix directory certainly
            # isn't correct (it gets deleted by pip after the install), all we
            # can do is to have __init__.py set LD_LIBRARY_PATH to the correct
            # value at runtime. Depending on Linux version, an RPATH key would
            # override even LD_LIBRARY_PATH, so we have to disable the RPATH
            # mechanism entirely during build/install.
            #
            # Note that the above RPATH nonsense only applies to Linux and Mac,
            # and thankfully the same method should work for both. Windows is
            # much more lenient in where it finds DLLs, checking the current
            # directory and such by default, so it shouldn't need any of this
            # stupidity.

            # Do the build with the given number of parallel threads.
            cmd = local['cmake']['--build']['.']
            if nprocs != '1':
                cmd = cmd['--parallel'][nprocs]
            cmd & FG

            # Do the install.
            local['cmake']['--install']['.'] & FG

        # Copy the user-written Python API files into module_dir for the
        # parent build_ext to clobber with SWIG output and build to find the
        # user modules AND swig-generated module in.
        if os.path.exists(module_dir):
            shutil.rmtree(module_dir)
        shutil.copytree(python_dir, module_dir)

        # Also copy the files CMake placed in lib/lib64 into the build
        # directory managed by setuptools, so the shared objects/DLLs are
        # added to the generated Python module.
        dest_dir = self.build_lib + os.sep + 'openql'
        if not os.path.exists(dest_dir):
            os.makedirs(dest_dir)
        src_dir = 'lib64' if os.path.exists(prefix_dir + os.sep + 'lib64') else 'lib'
        src_dir = prefix_dir + os.sep + src_dir
        for fname in os.listdir(src_dir):
            if os.path.isfile(src_dir + os.sep + fname) and not os.path.islink(src_dir + os.sep + fname):
                shutil.copyfile(src_dir + os.sep + fname, dest_dir + os.sep + fname)

        # Let setuptools handle the SWIG build and final link.
        _build_ext.run(self)

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
        elif platform.system() == "Linux":
            # This only works for manylinux
            if 'AUDITWHEEL_PLAT' in os.environ:
                from auditwheel.repair import repair_wheel
                repair_wheel(wheel_path, abi=os.environ['AUDITWHEEL_PLAT'], lib_sdir=".libs", out_dir=self.dist_dir, update_tags=True)

class sdist(_sdist):
    def finalize_options(self):
        _sdist.finalize_options(self)
        self.dist_dir = os.path.relpath(dist_dir)

class egg_info(_egg_info):
    def initialize_options(self):
        _egg_info.initialize_options(self)
        self.egg_base = os.path.relpath(pybuild_dir)

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
    package_dir = {'openql': os.path.relpath(module_dir)},

    ext_modules = [
        Extension(
            'openql._openql',
            [module_dir + os.sep + 'openql.i'],
            libraries = ['ql'],
            library_dirs = [prefix_dir + os.sep + 'lib', prefix_dir + os.sep + 'lib64'],
            #runtime_library_dirs = [prefix_dir + os.sep + 'lib', prefix_dir + os.sep + 'lib64'],
            include_dirs = [prefix_dir + os.sep + 'include'],
            extra_compile_args = ['-std=c++11'],
            swig_opts = ['-v', '-py3', '-castmode', '-modern', '-w511', '-c++', '-I' + prefix_dir + os.sep + 'include']
        )
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
