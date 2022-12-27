"""Setup script for the MAD module distribution."""

import os
import platform
import subprocess
import sys
from pprint import pprint
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


# read the contents of the README file
from pathlib import Path
this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text()


# Command line flags forwarded to CMake (for debug purpose)
cmake_cmd_args = []
for f in sys.argv:
    if f.startswith('-D'):
        cmake_cmd_args.append(f)

for f in cmake_cmd_args:
    sys.argv.remove(f)


def _get_env_variable(name, default='OFF'):
    if name not in os.environ.keys():
        return default
    return os.environ[name]


class CMakeExtension(Extension):
    def __init__(self, name, cmake_lists_dir='.', **kwargs):
        Extension.__init__(self, name, sources=[], **kwargs)
        self.cmake_lists_dir = os.path.abspath(cmake_lists_dir)

class CMakeBuild(build_ext):

    def build_extensions(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError('Cannot find CMake executable')

        python_version = '{}.{}.{}'.format(
            sys.version_info.major, 
            sys.version_info.minor, 
            sys.version_info.micro
        )

        for ext in self.extensions:

            extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
            cfg = 'Debug' if _get_env_variable('PYMP3_DEBUG') == 'ON' else 'Release'

            python_module_name, python_module_ext = os.path.splitext(os.path.basename(self.get_ext_fullpath(ext.name)))

            cmake_args = [
                '-DCMAKE_BUILD_TYPE=%s' % cfg,

                # Ask CMake to place the resulting library in the directory
                # containing the extension
                '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir),

                # Other intermediate static libraries are placed in a
                # temporary build directory instead
                '-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), self.build_temp),

                # Hint CMake to use the same Python executable that
                # is launching the build, prevents possible mismatching if
                # multiple versions of Python are installed
                '-DPython3_EXECUTABLE={}'.format(sys.executable),

                '-DPYMP3_PYTHON_MODULE_NAME={}'.format(python_module_name),
                '-DPYMP3_PYTHON_MODULE_EXT={}'.format(python_module_ext),
            ]

            if platform.system() == 'Windows':
                plat = ('x64' if platform.architecture()[0] == '64bit' else 'Win32')
                cmake_args += [
                    '-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE',
                    '-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir),
                ]
                if self.compiler.compiler_type == 'msvc':
                    cmake_args += [
                        '-DCMAKE_GENERATOR_PLATFORM=%s' % plat,
                    ]
                else:
                    cmake_args += [
                        '-G', 'MinGW Makefiles',
                    ]

            cmake_args += cmake_cmd_args

            pprint(cmake_args)

            if not os.path.exists(self.build_temp):
                os.makedirs(self.build_temp)

            # Config and build the extension
            subprocess.check_call(['cmake', ext.cmake_lists_dir] + cmake_args,
                                  cwd=self.build_temp)

            subprocess.check_call(['cmake', '--build', '.', '--config', cfg],
                                  cwd=self.build_temp)


setup(
    name='pymp3',
    description='Read and write MP3 files.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    author='Gennadiy Bezkorovayniy',
    author_email='gb@miarec.com',
    url='https://github.com/miarec/pymp3',
    license='GPL',
    ext_modules=[CMakeExtension('mp3')],
    cmdclass={'build_ext': CMakeBuild},
    zip_safe=False,
    python_requires='>=3.6',
    setup_requires=['setuptools-git-versioning<2'],
    setuptools_git_versioning={
        "enabled": True,
    },
    classifiers=[
        'Topic :: Multimedia :: Sound/Audio :: Conversion',
        'Programming Language :: Python :: 3 :: Only',
        'License :: OSI Approved :: GNU General Public License v2 (GPLv2)',
        # 'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX :: Linux'
    ]
)
