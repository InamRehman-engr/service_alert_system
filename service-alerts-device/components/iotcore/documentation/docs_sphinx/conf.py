# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information
from sphinx.builders.html import StandaloneHTMLBuilder
import subprocess, os
from hawkmoth.util import readthedocs
hawkmoth_root = os.path.abspath('/app/')
readthedocs.clang_setup()
# Doxygen
subprocess.call('doxygen Doxyfile.in', shell=True)
project = 'Iotcore'
copyright = '2024, Cowlar Design Studio'
author = 'Cowlar Design Studio'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.autosectionlabel',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'sphinx.ext.mathjax',
    'sphinx.ext.ifconfig',
    'sphinx.ext.viewcode',
    'hawkmoth',
    'sphinx.ext.inheritance_diagram',
    'breathe',
    "myst_parser",
    'sphinx_copybutton'
]

source_suffix = {
    '.rst': 'restructuredtext',
    '.txt': 'markdown',
    '.md': 'markdown',
}

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output


# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'canonical_url': '',
    'analytics_id': '',
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    
    'logo_only': False,

    # Toc options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}
# html_logo = ''
# github_url = ''
# html_baseurl = ''
html_static_path = ['_static']

# -- Breathe configuration -------------------------------------------------

c_autodoc_roots = ['/app/']

breathe_projects = {
	"Iotcore": "_build/xml/"
}
breathe_default_project = "Iotcore"
breathe_default_members = ('members', 'undoc-members')