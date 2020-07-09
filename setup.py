#!/usr/bin/env python3

# TODO: clean includes once done
import os, platform, shutil, sys, subprocess
from distutils.command.bdist import bdist as _bdist
from distutils.command.sdist import sdist as _sdist
from distutils.command.build import build as _build
from distutils.command.clean import clean as _clean
from setuptools.command.egg_info import egg_info as _egg_info
import distutils.cmd
import distutils.log
from setuptools import setup, Extension, find_packages
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
import re

cbuild_dir = os.getcwd() + os.sep + 'cbuild'    # cmake build directory
pybuild_dir = os.getcwd() + os.sep + 'pybuild'  # python-specific build directory (removed by setup.py clean)
prefix_dir = pybuild_dir + os.sep + 'prefix'    # cmake install prefix for the normal targets
module_dir = pybuild_dir + os.sep + 'openql'    # cmake install directory for the python module
build_dir = pybuild_dir + os.sep + 'build'      # directory for setuptools to dump various files into
dist_dir = pybuild_dir + os.sep + 'dist'        # wheel output directory

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

    except Exception as e:
        print(e)
        version = 'none'

    if verbose:
        print('get_version: %s' % version)

    return version

def read(fname):
    with open(os.path.join(os.path.dirname(__file__), fname)) as f:
        return f.read()

class clean(_clean):
    def run(self):
        _clean.run(self)
        shutil.rmtree(cbuild_dir)
        shutil.rmtree(pybuild_dir)

class build(_build):
    def initialize_options(self):
        _build.initialize_options(self)
        self.build_base = os.path.relpath(build_dir)

    def run(self):
        from plumbum import local, FG
        with local.cwd(cbuild_dir):
            local['cmake']['..']['-DOPENQL_PYTHON_DIR=' + module_dir]['-DCMAKE_INSTALL_PREFIX=' + prefix_dir] & FG
            local['cmake']['--build']['.'] & FG
            local['cmake']['--install']['.'] & FG
        _build.run(self)

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
    author='Nader Khammassi and Imran Ashraf',
    author_email='nader.khammassi@gmail.com, iimran.aashraf@gmail.com',
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
    include_package_data = True,

    cmdclass = {
        'bdist': bdist,
        'bdist_wheel': bdist_wheel,
        'build': build,
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
