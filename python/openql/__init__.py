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

# Import the SWIG-generated module into ourselves.
if PY3:
    from .openql import *
else:
    from openql import *
del PY3

# List of all the relevant SWIG-generated stuff, to avoid outputting docs for
# all the other garbage SWIG generates for internal use.
__all__ = [
    'initialize',
    'ensure_initialized',
    'get_version',
    'set_option',
    'get_option',
    'print_options',
    'dump_options',
    'print_architectures',
    'dump_architectures',
    'print_passes',
    'dump_passes',
    'print_resources',
    'dump_resources',
    'print_platform_docs',
    'dump_platform_docs',
    'print_compiler_docs',
    'dump_compiler_docs',
    'Platform',
    'Program',
    'Kernel',
    'CReg',
    'Operation',
    'Unitary',
    'Compiler',
    'Pass',
    'cQasmReader',
]

# Swig's autodoc thing is nice, because it saves typing out all the overload
# signatures of each function. But it doesn't play nicely with Sphinx out of the
# box: Sphinx expects multiple signature lines to have a \ at the end (for as
# far as it's supported at all, you need 3.x at least), and SWIG outputs
# C++-style types rather than even trying to convert the types to Python.
# Python's object model to the rescue: we can just monkey-patch the docstrings
# after the fact.
def _fixup_swig_autodoc_type(typ):
    typ = typ.split()[0].split('::')[-1]
    typ = {
        'string': 'str',
        'size_t': 'int',
        'double': 'float',
        'mapss': 'Dict[str, str]',
        'vectorp': 'List[Pass]',
        'vectorui': 'List[int]',
        'vectord': 'List[float]'
    }.get(typ, typ)
    return typ

def _fixup_swig_autodoc_signature(sig):
    try:

        # Parse the incoming SWIG autodoc signature.
        begin, rest = sig.split('(', maxsplit=1)
        args, return_type = rest.split(')', maxsplit=1)
        name = begin.strip()
        return_type = return_type.strip()
        if return_type:
            assert return_type.startswith('-> ')
            return_type = return_type[3:]
        spacing = ' ' * (len(begin) - len(name))

        # Fix argument type names and use Python syntax for signature.
        args = args.split(',')
        for i, arg in enumerate(args):
            if not arg:
                continue
            toks = arg.split()
            if toks[-1] == 'self':
                args[i] = 'self'
                continue
            arg_name = toks[-1].split('=')
            if len(arg_name) > 1:
                default_val = ' = ' + arg_name[1]
            else:
                default_val = ''
            arg_name = arg_name[0]
            args[i] = arg_name + ': ' + _fixup_swig_autodoc_type(toks[0]) + default_val

        args = ', '.join(args)

        # Fix return type name.
        if return_type:
            return_type = _fixup_swig_autodoc_type(return_type)
        else:
            return_type = 'None'

        sig = spacing + name + '(' + args + ') -> ' + return_type

    except (Exception, AssertionError):
        pass

    return sig

def _fixup_swig_autodoc(ob, keep_sig, keep_docs):
    try:
        lines = ob.__doc__.split('\n')
        new_lines = []
        state = 0
        for line in lines:
            if state == 0:
                if line.strip():
                    state = 1
                    if keep_sig:
                        new_lines.append(_fixup_swig_autodoc_signature(line))
            elif state == 1:
                if not line.strip():
                    state = 2
                elif keep_sig:
                    new_lines[-1] += ' \\'
                    new_lines.append(_fixup_swig_autodoc_signature(line))
            elif keep_docs:
                new_lines.append(line)
        while new_lines and not new_lines[-1]:
            del new_lines[-1]
        ob.__doc__ = '\n'.join(new_lines) + '\n'
    except Exception:
        pass

for ob in __all__:
    ob = globals()[ob]
    if type(ob) == type:
        for mem in dir(ob):
            if mem == '__init__':
                _fixup_swig_autodoc(getattr(ob, mem), True, False)
            elif not mem.startswith('_'):
                if isinstance(getattr(ob, mem), property):
                    _fixup_swig_autodoc(getattr(ob, mem), False, True)
                else:
                    _fixup_swig_autodoc(getattr(ob, mem), True, True)

    else:
        _fixup_swig_autodoc(ob, True, True)


# Monkey-patch some pure-Python functions into Platform.
@staticmethod
def get_platform_json(platform_config='none'):
    """
    get_platform_json(platform_config: str) -> Dict[...] \
    get_platform_json() -> Dict[...]

    Returns the default platform configuration data as the Python object
    representation of the JSON data (as returned by json.loads()).

    Parameters
    ----------
    platform_config : str
        The platform configuration. Same syntax as the platform constructor, so
        this supports architecture names, architecture variant names, or JSON
        filenames. In the latter case, this function just parses the file
        contents and returns it.

    Returns
    -------
    str
        The Python object representation of the JSON data corresponding to the
        given platform configuration string.
    """
    import json
    lines = Platform.get_platform_json_string(platform_config).split('\n')
    for i in range(len(lines)):
        lines[i] = lines[i].split('//')[0]
    return json.loads('\n'.join(lines))
Platform.get_platform_json = get_platform_json
del get_platform_json

@staticmethod
def from_json(name, platform_config_json, compiler_config=''):
    """
    from_json(name: str, platform_config_json: Dict[...], compiler_config: str) -> Platform \\
    from_json(name: str, platform_config_json: Dict[...]) -> Platform

    Alternative constructor. Instead of the platform JSON data being loaded from
    a file, they are loaded from the given Python object representation of the
    JSON platform configuration data.

    This is useful when you only need to change a builtin platform for some
    architecture variant a little bit. In this case, you can get the default
    JSON data using get_platform_json(), introspect and modify it
    programmatically, and then use this to build the platform from the modified
    configuration.

    Parameters
    ----------
    name : str
        The name for the platform.
    platform_config_json : JSON-like object
        The platform JSON configuration data in Python object representation
        (anything accepted by json.dumps()).
    compiler_config : str
        Optional compiler configuration JSON filename. This is *NOT* JSON data.

    Returns
    -------
    Platform
        The constructed platform.
    """
    import json
    platform_config_json = json.dumps(platform_config_json)
    return Platform.from_json_string(name, platform_config_json, compiler_config)
Platform.from_json = from_json
del from_json
