# Welcome to MQT Debugger's documentation!

MQT Debugger is a comprehensive framework for debugging and analyzing the behaviour of quantum programs.
It is part of the _{doc}`Munich Quantum Toolkit (MQT) <mqt:index>`_.

The framework provides tools for running quantum programs in a simulated environment, stepping through the code instruction-by-instruction, and displaying the quantum state at each step.
It also allows developers to include assertions in their code, testing the correctness of provided programs, and, when an assertion fails, suggesting possible causes of the failure.

MQT Debugger is accessible as a stand-alone application, as a C++ and Python library to include in custom programs, and as a DAP server that can be accessed by popular IDEs such as Visual Studio Code and CLion.

We recommend you to start with the {doc}`installation instructions <Installation>`.
Then proceed to the {doc}`quickstart guide <Quickstart>` and read the {doc}`reference documentation <library/Library>`.
If you are interested in the theory behind MQT Debugger, have a look at the publications in the {doc}`publication list <references>`.

We appreciate any feedback and contributions to the project. If you want to contribute, you can find more information in the {doc}`contribution guide <Contributing>`. If you are having trouble with the installation or the usage of MQT Debugger, please let us know at our {doc}`support page <Support>`.

```{toctree}
:hidden:

self
```

```{toctree}
:caption: User Guide
:hidden:
:maxdepth: 1

Installation
Quickstart
Debugging
Assertions
Diagnosis
AssertionRefinement
RuntimeVerification
references
```

```{toctree}
:caption: Developers
:glob:
:hidden:
:maxdepth: 1

Contributing
DevelopmentGuide
Support
```

```{toctree}
:caption: API Reference
:glob:
:hidden:
:maxdepth: 6

library/Library
```

## Contributors and Supporters

The _[Munich Quantum Toolkit (MQT)](https://mqt.readthedocs.io)_ is developed by the [Chair for Design Automation](https://www.cda.cit.tum.de/) at the [Technical University of Munich](https://www.tum.de/) and supported by the [Munich Quantum Software Company (MQSC)](https://munichquantum.software).
Among others, it is part of the [Munich Quantum Software Stack (MQSS)](https://www.munich-quantum-valley.de/research/research-areas/mqss) ecosystem, which is being developed as part of the [Munich Quantum Valley (MQV)](https://www.munich-quantum-valley.de) initiative.

<div style="margin-top: 0.5em">
<div class="only-light" align="center">
  <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-logo-banner-light.svg" width="90%" alt="MQT Banner">
</div>
<div class="only-dark" align="center">
  <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-logo-banner-dark.svg" width="90%" alt="MQT Banner">
</div>
</div>

Thank you to all the contributors who have helped make MQT Debugger a reality!

<p align="center">
<a href="https://github.com/munich-quantum-toolkit/debugger/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=munich-quantum-toolkit/debugger" />
</a>
</p>

The MQT will remain free, open-source, and permissively licensed—now and in the future.
We are firmly committed to keeping it open and actively maintained for the quantum computing community.

To support this endeavor, please consider:

- Starring and sharing our repositories: [https://github.com/munich-quantum-toolkit](https://github.com/munich-quantum-toolkit)
- Contributing code, documentation, tests, or examples via issues and pull requests
- Citing the MQT in your publications (see {doc}`References <references>`)
- Using the MQT in research and teaching, and sharing feedback and use cases
- Sponsoring us on GitHub: [https://github.com/sponsors/munich-quantum-toolkit](https://github.com/sponsors/munich-quantum-toolkit)

<p align="center">
<iframe src="https://github.com/sponsors/munich-quantum-toolkit/button" title="Sponsor munich-quantum-toolkit" height="32" width="114" style="border: 0; border-radius: 6px;"></iframe>
</p>
