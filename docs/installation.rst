Installation
============

MQT Debugger is mainly developed as a C++ library with many of its interfaces for important modules being defined in C.
Parts of the implementation build upon `MQT Core <https://github.com/munich-quantum-toolkit/core>`_, which forms the backbone of the `MQT <https://mqt.readthedocs.io>`_.
In order to make the tool as accessible as possible, it comes with an easy-to-use Python interface.

We encourage installing MQT Debugger via pip (preferably in a `virtual environment <https://docs.python.org/3/library/venv.html>`_):

    .. code-block:: console

        (venv) & pip install mqt.debugger

In most practical cases (under 64-bit Linux, MacOS incl. Apple Silicon, and Windows), this requires no compilation and merely downloads and installs a platform-specific pre-built wheel.

.. note::
    In order to set up a virtual environment, you can use the following commands:

    .. code-block:: console

        $ python3 -m venv venv
        $ source venv/bin/activate

    If you are using Windows, you can use the following commands instead:

    .. code-block:: console

        $ python3 -m venv venv
        $ venv\Scripts\activate.bat

    It is recommended to make sure that you are using the latest version of pip, setuptools, and wheel before trying to install the project:

    .. code-block:: console

        (venv) $ pip install --upgrade pip setuptools wheel

A Detailed Walk Through
#######################

First, save the following lines as :code:`ghz.qasm` in a folder where you want to install MQT Debugger:

    .. code-block::

        qreg q[3];

        h q[0];
        cx q[0], q[1];
        cx q[2], q[0];

        assert-ent q;

Then, create the following Python script as :code:`debug_ghz.py` in the same folder:

    .. code-block:: python

        import mqt.debugger as dbg

        state = dbg.create_ddsim_simulation_state()
        with open("ghz.qasm") as f:
            state.load_code(f.read())

        state.run_simulation()

        if state.did_assertion_fail():
            print(f"Assertion failed at instruction {state.get_current_instruction() + 1}")
            problems = state.get_diagnostics().potential_error_causes()
            print("Potential Errors:")
            for problem in problems:
                print(f"{problem.type} at instruction {problem.instruction + 1}")

The following snippet shows the installation process from setting up the virtual environment to running a small example program.

    .. code-block:: console

        $ python3 -m venv venv
        $ . venv/bin/activate
        (venv) $ pip install -U pip setuptools wheel
        (venv) $ pip install mqt.debugger
        (venv) $ python3 debug_ghz.py

Building from Source for Performance
####################################

In order to get the best performance out of MQT Debugger and enable platform-specific compiler optimizations that cannot be enabled on portable wheels, it is recommended to build the package from source via:

    .. code-block:: console

        (venv) $ pip install mqt.debugger --no-binary mqt.debugger

This requires a `C++ compiler <https://en.wikipedia.org/wiki/List_of_compilers#C++_compilers>`_ compiler supporting *C++20* and a minimum `CMake <https://cmake.org/>`_ version of *3.19*.

The library is continuously tested under Linux, MacOS, and Windows using the `latest available system versions for GitHub Actions <https://github.com/actions/virtual-environments>`_.
In order to access the latest build logs, visit the `GitHub Actions page <https://github.com/munich-quantum-toolkit/debugger/actions/workflows/ci.yml>`_.

.. note::
    We noticed some issues when compiling with Microsoft's *MSCV* compiler toolchain. If you want to start development on this project under Windows, consider using the *clang* compiler toolchain. A detailed description of how to set this up can be found `here <https://docs.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-160>`_.
