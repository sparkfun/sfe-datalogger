import setuptools
from codecs import open  # To use a consistent encoding
from os import path
from platform import system, machine
import subprocess
import sys


#https://stackoverflow.com/a/50914550
def resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    base_path = getattr(sys, '_MEIPASS', path.dirname(path.abspath(__file__)))
    return path.join(base_path, _RESOURCE_DIRECTORY, relative_path)

def get_version(rel_path: str) -> str:
    try: 
        with open(resource_path(rel_path), encoding='utf-8') as fp:
            for line in fp.read().splitlines():
                if line.startswith("__version__"):
                    delim = '"' if '"' in line else "'"
                    return line.split(delim)[1]
            raise RuntimeError("Unable to find version string.")
    except:
        raise RuntimeError("Unable to find _version.py.")

_APP_VERSION = get_version("_version.py")

here = path.abspath(path.dirname(__file__))

# Get the long description from the relevant file
with open(path.join(here, 'DESCRIPTION.md'), encoding='utf-8') as f:
    long_description = f.read()
    
install_deps = ['esptool', 'logging', 'configobj', 'validate','pycryptodome', 'tempfile']


setuptools.setup(
    name='sfe_dl_fuseid',

    # Versions should comply with PEP440.  For a discussion on single-sourcing
    # the version across setup.py and the project code, see
    # http://packaging.python.org/en/latest/tutorial.html#version
    version=_APP_VERSION,

    description='Application to burn a code to eFuse memory of a ESP32 for the SparkFun DataLogger Line',
    long_description=long_description,

    # The project's main homepage.
    url='https://github.com/sparkfun/sfe-datalogger/fuse_id',

    # Author details
    author='SparkFun Electronics',
    author_email='sales@sparkfun.com',

    project_urls = {
        "Bug Tracker" : "https://github.com/sparkfun/sfe-datalogger/issues",
        "Repository"   : "https://github.com/sparkfun/sfe-datalogger/fuse_id"
    },
    # Choose your license
    license='Internal',

    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        # How mature is this project? Common values are
        #   3 - Alpha
        #   4 - Beta
        #   5 - Production/Stable
        'Production Stable :: 4',

        # Indicate who your project is intended for
        'Intended Audience :: SparkFun Employees',
        'Topic :: Hardware Development :: Build Tools',

        # Pick your license as you wish (should match "license" above)
        'License :: Internal',

        # Specify the Python versions you support here. In particular, ensure
        # that you indicate whether you support Python 2, Python 3 or both.
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',

    ],

    download_url="https://github.com/sparkfun/sfe-datalogger/tree/main/fuse_id",

    # What does your project relate to?
    keywords='DataLogger Production',

    # You can just specify the packages manually here if your project is
    # simple. Or you can use find_packages().
    packages=["sfe_dl_fuseid"],

    # List run-time dependencies here.  These will be installed by pip when your
    # project is installed. For an analysis of "install_requires" vs pip's
    # requirements files see:
    # https://packaging.python.org/en/latest/technical.html#install-requires-vs-requirements-files
    install_requires=install_deps,

    # If there are data files included in your packages that need to be
    # installed, specify them here.  If using Python 2.6 or less, then these
    # have to be included in MANIFEST.in as well.
    package_data={}

    # To provide executable scripts, use entry points in preference to the
    # "scripts" keyword. Entry points provide cross-platform support and allow
    # pip to create the appropriate form of executable for the target platform.
    entry_points={
        'console_scripts': ['dl_fuseid=sfe_dl_fuseid:dl_fuseid',
        ],
    },
)
