#!/usr/bin/env python3

from distutils.errors import DistutilsArgError
from glob import glob
import os
import subprocess as process
import setuptools
from setuptools import setup, find_packages, Extension
from distutils.command.clean import clean
from distutils.command.build import build
from distutils.command.install_lib import install_lib
from setuptools.command.build_ext import build_ext
from setuptools.command.develop import develop
from shutil import rmtree

version = '0.1'
source_dir = os.path.abspath('.')

# This cannot be ./build because setuptools looks there for the built libraries
# and if cmake builds inside it, setuptools copies all the build files into
# the module as well
build_dir = os.path.abspath('./build_cmake')


def check_cmake() -> str:
    cmake = 'cmake'
    if 'CMAKE' in os.environ:
        cmake = os.environ['CMAKE']
    try:
        process.check_output([cmake, '--version'])
    except process.SubprocessError:
        raise RuntimeError('Could not find cmake')

    return cmake


class CleanCommand(clean):
    description = clean.description
    user_options = clean.user_options

    def initialize_options(self):
        clean.initialize_options(self)

    def finalize_options(self):
        clean.finalize_options(self)

    def run(self):
        build_default = os.path.join(source_dir, 'build')
        if os.path.exists(build_default):
            rmtree(build_default)
        if os.path.exists(build_dir):
            rmtree(build_dir)
        for f in glob(os.path.join(source_dir, '*.egg-info')):
            rmtree(f)
        for pattern in ['llvm-browse.gresource', '*.so', '*.dylib', '*.dll']:
            for f in glob(os.path.join(source_dir, 'llvm_browse', pattern),
                          recursive=False):
                os.remove(f)
        clean.run(self)


class BuildCommand(build):
    description = build.description
    user_options = build.user_options + [
        ('build-debug', None, 'Set the CMake build type to "Debug"'),
        ('llvm-dir=', None, 'Path to LLVM install root'),
        ('llvm-link-libllvm', None, 'Use libLLVM if available'),
        ('llvm-link-shared', None, 'Link against LLVM shared libraries'),
        ('make-verbose', None, 'Use make in verbose mode')
    ]

    def initialize_options(self):
        build.initialize_options(self)
        self.build_debug = False
        self.llvm_link_shared = False
        self.llvm_link_libllvm = False
        self.llvm_dir = ''
        self.make_verbose = False

    def finalize_options(self):
        build.finalize_options(self)

    def run(self):
        cmake = check_cmake()

        os.makedirs(build_dir, exist_ok=True)

        cfg = [
            cmake,
            '-DCMAKE_BUILD_TYPE={}'.format(
                'Debug' if self.build_debug else 'Release'),
            '-DPROJECT_VERSION={}'.format(version),
        ]
        if self.llvm_dir:
            cfg.append('-DLLVM_INSTALLED={}'.format(self.llvm_dir))
        if self.llvm_link_libllvm:
            cfg.append('-DLLVM_LINK_LIBLLVM=On')
        if self.llvm_link_shared:
            cfg.append('-DLLVM_LINK_SHARED=On')
        cfg.extend(['-B', build_dir])
        cfg.extend(['-S', source_dir])

        make = ['make']
        if self.make_verbose:
            make.append('VERBOSE=1')
        if self.parallel:
            make.append('-j{}'.format(self.parallel))

        try:
            process.check_call(' '.join(cfg), cwd=build_dir, shell=True)
            process.check_call(' '.join(make), cwd=build_dir, shell=True)
        except process.SubprocessError:
            raise RuntimeError('Cmake configuration failed')

        build.run(self)


setup(
    name='LLVM Browse',
    version=version,
    author='Tarun Prabhu',
    author_email='tarun.prabhu@gmail.com',
    description=('Code browser for LLVM IR'),
    license='GPL-3.0',
    keywords='LLVM, LLVM-IR, Browser, Code',
    url='https://www.github.com/tarunprabhu/llvm-browse',
    long_description=open('./README.md', 'r').read(),
    long_description_content_type='text/markdown',
    packages=[
        'llvm_browse'
    ],
    package_data={
        # This is ugly, but basically try to include the extensions for the
        # shared library that will get built
        "llvm_browse": [
            'llvm-browse.gresource',
            '*.so',
            '*.dll',
            '*.dylib'
        ],
    },
    include_package_data=True,
    entry_points={
        'gui_scripts': [
            'llvm-browse = llvm_browse.__main__:main'
        ]
    },
    cmdclass={
        'build': BuildCommand,
        'clean': CleanCommand,
    },
    classifiers=[
        'Intended Audience :: Developers',
        'Development Status :: 3 - Alpha',
        'Topic :: Utilities',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Programming Language :: C',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'Operating System :: POSIX :: Linux'
    ],
)
