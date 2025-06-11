Development Guide
=================


Installation
############

In order to start developing, clone the MQT Debugger repository using

    .. code-block:: console

        $ git clone https://github.com/munich-quantum-toolkit/debugger

A C++ compiler supporting C++17 and a minimum CMake version of 3.19 are required to build the project.

Working on the core C++ library
###############################

Our projects use CMake as the main build configuration tool.
Building a project using CMake is a two-stage process.
First, CMake needs to be configured by calling

    .. code-block:: console

        $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

This tells CMake to search the current directory :code:`.` (passed via :code:`-S` for source) for a :code:`CMakeLists.txt` file and process it into a directory :code:`build` (passed via :code:`-B`).
The flag :code:`-DCMAKE_BUILD_TYPE=Release` tells CMake to configure a *Release* build (as opposed to, e.g., a *Debug* build). We also recommend setting the flags
:code:`-DEIGEN_BUILD_TESTING=OFF` and :code:`-DEIGEN_BUILD_DOC=OFF` to prevent the documentation and tests of the ``Eigen3`` library included in the project from being built.

After configuring with CMake, the project can be built by calling

    .. code-block:: console

        $ cmake --build build --config Release

This tries to build the project in the :code:`build` directory (passed via :code:`--build`).
Some operating systems and developer environments explicitly require a configuration to be set, which is why the :code:`--config` flag is also passed to the build command. The flag :code:`--parallel <NUMBER_OF_THREADS>` may be added to trigger a parallel build.

Building the project this way generates

- the main library :code:`libmqt_debugger.a` (Unix) / :code:`mqt_debugger.lib` (Windows) in the :code:`build/src` directory
- a test executable :code:`mqt_debugger_test` containing unit tests in the :code:`build/test` directory (this requires passing :code:`-DBUILD_MQT_DEBUGGER_TESTS=ON` to CMake during configuration)
- the Python bindings library :code:`pydebugger.<...>` in the :code:`build/python` directory (this requires passing :code:`-DBUILD_MQT_DEBUGGER_BINDINGS=ON` to CMake during configuration)
- the CLI App :code:`mqt_debugger_app` in the :code:`build/app` directory (this requires passing :code:`-DBUILD_MQT_DEBUGGER_APP=ON` to CMake during configuration)

Working on the Python module
############################

The :code:`mqt.debugger` Python module can be conveniently built locally by calling

    .. code-block:: console

        (venv) $ pip install --editable .

The :code:`--editable` flag ensures that changes in the Python code are instantly available without re-running the command.

`Pybind11 <https://pybind11.readthedocs.io/>`_ is used for providing bindings of the C++ core library to Python (see `bindings.cpp <https://github.com/munich-quantum-toolkit/debugger/tree/main/bindings/bindings.cpp>`_).
If parts of the C++ code have been changed, the above command has to be run again to make the changes visible in Python.
