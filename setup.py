# coding=utf-8
"""Setup package 'rational'."""

import os
from setuptools import setup, Extension
import sysconfig

DEBUG = int(os.getenv("DEBUG", 0))
extra_compile_args = sysconfig.get_config_var('CFLAGS').split()
extra_compile_args += ["-Wall", "-Wextra"]
if DEBUG:
    extra_compile_args += ["-g3", "-O0", f"-DDEBUG={DEBUG}", "-UNDEBUG"]
else:
    extra_compile_args += ["-DNDEBUG", "-O3"]

ext_modules = [
    Extension(
        'rational.rational',
        ['src/rational/rational.c'],
        include_dirs=['src/rational'],
        extra_compile_args=extra_compile_args,
        # extra_link_args="",
        language='c',
        ),
    ]

with open('README.md') as file:
    long_description = file.read()

setup(
    name="rational",
    author="Michael Amrhein",
    author_email="michael@adrhinum.de",
    url="https://github.com/mamrhein/rational",
    description="Decimal numbers with fixed-point arithmetic",
    long_description=long_description,
    long_description_content_type="text/markdown",
    package_dir={'': 'src'},
    packages=['rational'],
    ext_modules=ext_modules,
    python_requires=">=3.7",
    tests_require=["pytest"],
    license='BSD',
    keywords='rational number datatype',
    platforms='all',
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: BSD License",
        "Operating System :: OS Independent",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: Implementation :: CPython",
        "Programming Language :: Python :: Implementation :: PyPy",
        "Topic :: Software Development",
        "Topic :: Software Development :: Libraries",
        "Topic :: Software Development :: Libraries :: Python Modules"
        ],
    zip_safe=False,
    )
