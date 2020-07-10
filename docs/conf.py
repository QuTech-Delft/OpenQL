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
try:
    docs_dir = os.path.dirname(__file__)
    os.chdir(docs_dir)
    if not os.path.exists('doxygen/doxy'):
        subprocess.check_call(['doxygen'])
        subprocess.check_call(['mv', '-f', 'doxygen/html', 'doxygen/doxy'])
finally:
    os.chdir(original_workdir)

html_extra_path = ['doxygen']

# -- Project information -----------------------------------------------------

project = 'OpenQL'
copyright = '2016, Nader Khammassi & Imran Ashraf, QuTech, TU Delft'
author = 'QuTech, TU Delft'

master_doc = 'index'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'm2r',
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


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
import sphinx_rtd_theme
html_theme = 'sphinx_rtd_theme'
html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']


[extensions]
todo_include_todos=True

# to enable figure numbering
numfig = True
