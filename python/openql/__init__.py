# Author Imran Ashraf, Jeroen van Straten

# Before we can import the dynamic modules, we have to set the linker search
# path appropriately.
import os
ld_lib_path = os.environ.get('LD_LIBRARY_PATH', '')
if ld_lib_path:
    ld_lib_path += ':'
os.environ['LD_LIBRARY_PATH'] = ld_lib_path + os.path.dirname(__file__)
del ld_lib_path, os

# The import syntax changes slightly between python 2 and 3, so we
# need to detect which version is being used:
from sys import version_info
if version_info[0] == 3:
    PY3 = True
elif version_info[0] == 2:
    PY3 = False
else:
    raise EnvironmentError("sys.version_info refers to a version of "
        "Python neither 2 nor 3. This is not permitted. "
        "sys.version_info = {}".format(version_info))
del version_info

# Import the SWIG-generated module.
if PY3:
    from .openql import Program, Kernel
else:
    from openql import *
del PY3

# __all__ = [ init, schedule, compile ]
