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

# All of the modules must be completely imported before we can start importing
# specific names, due to circular dependencies between the various modules.
if PY3:
    from .openql import api
else:
    import api

__all__ = [ api ]
