# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.

import os
import sys
sys.path.insert(0, os.path.abspath('../swig'))

# -- Doxygen build -----------------------------------------------------------
import subprocess
import os
import sys
original_workdir = os.getcwd()
docs_dir = os.path.dirname(__file__)
try:
    os.chdir(docs_dir)
    if not os.path.exists('doxygen/doxy'):
        subprocess.check_call(['doxygen'])
        subprocess.check_call(['mv', '-f', 'doxygen/html', 'doxygen/doxy'])
finally:
    os.chdir(original_workdir)

html_extra_path = ['doxygen']

# -- Generate RST files from runtime docs ------------------------------------
import openql as ql
import m2r2
import re

def docs_to_rst_magic(text, header_level=1):
    """Conversion magic for converting from OpenQL runtime docs to ReadTheDocs
    RST files."""

    # Perform conversion magic.
    output = []
    rst_block = False
    remove = False
    blank = True
    indent = 0
    for line in text.split('\n'):

        # Handle blank lines.
        if not line.strip():
            rst_block = False
            remove = False
            blank = True
            continue

        # Drop lines if we're in a removed section.
        if remove:
            continue

        # Strip section indentation.
        while not line.startswith('  '*indent):
            indent -= 1
            header_level -= 1
        line = line[indent*2:]

        # Handle the first line of a block of text, i.e. after a blank line.
        if blank:
            blank = False
            output.append('')

            # Handle section header.
            if line.startswith('* ') and line.endswith(' *'):
                output.append('#'*header_level + ' ' + line[2:-2])
                indent += 1
                header_level += 1
                continue

            # Handle "note" block.
            elif line.startswith('NOTE: '):
                output.append('.. note::')
                rst_block = True
                line = line[6:]

            # Handle removed note blocks. This is used for "X is not included
            # in this build" notifications that are naturally the case when
            # ReadTheDocs is building OpenQL.
            elif line.startswith('NOTE*: '):
                remove = True
                continue

            # Handle "warning" block.
            elif line.startswith('WARNING: '):
                output.append('.. warning::')
                rst_block = True
                line = line[9:]

            # A new RST block (note or warning) was opened, which means we need
            # to capitalize the first letter.
            if rst_block:
                output.append('   ' + m2r2.convert(line[:1].upper() + line[1:]).strip())
                continue

        # Indent followup lines of RST blocks.
        if rst_block:
            line = '   ' + m2r2.convert(line).strip()

        # Finished converting stuff.
        output.append(line)

    # Convert back to normal text.
    text = '\n'.join(output) + '\n'

    # Convert markdown syntax to RST.
    text = m2r2.convert(text)

    # m2r2 is a bit overzealous about things that look like HTML tags. After
    # all, markdown permits insertion of raw HTML, and RST doesn't, so it
    # does its best to convert things. That's not what we want; syntax like
    # <stuff> is used all over the place as placeholders within code blocks
    # and such, and there absolutely should never be raw HTML in the
    # docstrings anyway. So we just revert m2r2's hard work in this respect
    # by stripping all :raw-html-m2r:`` blocks.
    text = re.sub(r'(?:\\ )?:raw-html-m2r:`([^`]+)`(?:\\ )?', r'\1', text)

    return text

def get_version(verbose=0):
    """ Extract version information from source code """

    matcher = re.compile('[\t ]*#define[\t ]+OPENQL_VERSION_STRING[\t ]+"(.*)"')
    with open(os.path.join('..', 'include', 'ql', 'version.h'), 'r') as f:
        for ln in f:
            m = matcher.match(ln)
            if m:
                version = m.group(1)
                break
        else:
            raise Exception('failed to parse version string from include/ql/version.h')

    return version

if not os.path.exists('gen'):
    os.makedirs('gen')

# Version in installation instructions.
with open('manual/installation.rst.template', 'r') as f:
    docs = f.read()
docs = docs.replace('{version}', get_version())
with open('gen/manual_installation.rst', 'w') as f:
    f.write(docs)

# Architecture list.
with open('reference/architectures.rst.template', 'r') as f:
    docs = f.read()
docs = docs.replace('{architectures}', docs_to_rst_magic(ql.dump_architectures(), 2))
with open('gen/reference_architectures.rst', 'w') as f:
    f.write(docs)

# Global option list.
with open('reference/options.rst.template', 'r') as f:
    docs = f.read()
docs = docs.replace('{options}', docs_to_rst_magic(ql.dump_options(), 2))
with open('gen/reference_options.rst', 'w') as f:
    f.write(docs)

# Pass list.
with open('reference/passes.rst.template', 'r') as f:
    docs = f.read()
docs = docs.replace('{passes}', docs_to_rst_magic(ql.dump_passes(), 2))
with open('gen/reference_passes.rst', 'w') as f:
    f.write(docs)

# Resource list.
with open('reference/resources.rst.template', 'r') as f:
    docs = f.read()
docs = docs.replace('{resources}', docs_to_rst_magic(ql.dump_resources(), 2))
with open('gen/reference_resources.rst', 'w') as f:
    f.write(docs)

# Configuration file reference.
with open('reference/configuration.rst.template', 'r') as f:
    docs = f.read()
docs = docs.replace('{platform}', docs_to_rst_magic(ql.dump_platform_docs(), 3))
docs = docs.replace('{compiler}', docs_to_rst_magic(ql.dump_compiler_docs(), 3))
with open('gen/reference_configuration.rst', 'w') as f:
    f.write(docs)

# Output of simple.py.
import shutil
original_workdir = os.getcwd()
examples_dir = os.path.join(os.path.dirname(__file__), '..', 'examples')
try:
    os.chdir(examples_dir)
    subprocess.check_call([sys.executable, os.path.join(examples_dir, 'simple.py')])
finally:
    os.chdir(original_workdir)
shutil.copyfile(os.path.join(examples_dir, 'output', 'my_program.qasm'), 'gen/my_program.qasm')
shutil.copyfile(os.path.join(examples_dir, 'output', 'my_program_scheduled.qasm'), 'gen/my_program_scheduled.qasm')


# -- Project information -----------------------------------------------------

project = 'OpenQL'
copyright = '2016-2021, QuTech, TU Delft'
author = 'QuTech, TU Delft'

master_doc = 'index'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'm2r2',
    'sphinx.ext.todo',
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
    'sphinx.ext.autosummary'
]

autodoc_default_flags = ['members']
# autosummary_generate = True

source_suffix = ['.rst', '.md']


# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# Some or just temporary files,
# other ones files 'include::'d by another .rst file.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 
	'platform_*.rst', 'mapping.rst', 'scheduling.rst', 'decomposition.rst',
	'optimization.rst', 'scheduling_ccl.rst', 'scheduling_cc.rst']

def skip(app, what, name, obj, would_skip, options):
    if name == "__init__":
        return False
    return would_skip

def setup(app):
    app.connect("autodoc-skip-member", skip)

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
import sphinx_rtd_theme
html_theme = 'sphinx_rtd_theme'
html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
#html_static_path = ['_static']


[extensions]
todo_include_todos=True

# to enable figure numbering
numfig = True

autodoc_member_order = 'bysource'
