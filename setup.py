import os
import re
from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from sys import platform

rootDir = os.path.dirname(os.path.realpath(__file__))
srcDir = os.path.join(rootDir, "ql")
buildDir = os.path.join(rootDir, "cbuild")
clibDir = os.path.join(buildDir, "openql")

if not os.path.exists(buildDir):
    os.makedirs(buildDir)

try:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

    class bdist_wheel(_bdist_wheel):
        def finalize_options(self):
            _bdist_wheel.finalize_options(self)
            # Mark us as not a pure python package
            self.root_is_pure = False

        def get_tag(self):
            python, abi, plat = _bdist_wheel.get_tag(self)
            # We don't contain any python source
            python, abi = 'py2.py3', 'none'
            return python, abi, plat
except ImportError:
    bdist_wheel = None


cmake_args = []
if platform == "linux" or platform == "linux2":
    print('Detected Linux OS, installing openql ... ')
    cmake_command = 'cmake ..'
    make_command = 'make'
    clibname = "_openql.so"
elif platform == "darwin":
    print('Detected OSX, installing openql ... ')
    os.environ["MACOSX_DEPLOYMENT_TARGET"] = "10.10"
    cmake_command = 'cmake ..'
    make_command = 'make'
    clibname = "_openql.so"
elif platform == "win32":
    print('Detected Windows OS, installing openql ... ')
    cmake_args += ["-G", "NMake Makefiles"]
    make_command = 'nmake'
    clibname = "_openql.pyd"
else:
    raise RuntimeError('Unknown/Unsupported OS !!!')

clib = os.path.join(rootDir, "openql", clibname)
genclib = os.path.join(clibDir, clibname)


class BuildOpenQL(build_ext):
    def build_extension(self, ext):
        import shutil
        import os.path

        os.chdir(buildDir)
        self.spawn(['cmake'] + cmake_args + ['..'])
        self.spawn([make_command])
        os.chdir(rootDir)

        try:
            os.makedirs(os.path.dirname(self.get_ext_fullpath(ext.name)))
        except OSError as e:
            # directory already exisits
            if isinstance(e, FileExistsError) or \
                    (hasattr(e, 'winerror') and e.winerror == 183):
                pass
            else:
                raise

        shutil.copyfile(genclib, clib)
        shutil.copyfile(os.path.join(clibDir, "openql.py"),
                        os.path.join(rootDir, "openql", "openql.py"))


def get_version(verbose=0):
    """ Extract version information from source code """

    matcher = re.compile(
        '[\t ]*#define[\t ]+OPENQL_(MAJOR|MINOR|PATCH)_VERSION[\t ]+(.*)')

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
                            break

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
      packages=find_packages(),
      include_package_data=True,
      package_data={'openql': [clib]},
      ext_modules=[
          Extension(name='openql.openql._openql', sources=[])
      ],
      zip_safe=False,
      cmdclass={'build_ext': BuildOpenQL},
      )
