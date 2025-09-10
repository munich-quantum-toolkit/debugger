[![PyPI](https://img.shields.io/pypi/v/mqt.debugger?logo=pypi&style=flat-square)](https://pypi.org/project/mqt.debugger/)
![OS](https://img.shields.io/badge/os-linux%20%7C%20macos%20%7C%20windows-blue?style=flat-square)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square)](https://opensource.org/licenses/MIT)
[![CI](https://img.shields.io/github/actions/workflow/status/munich-quantum-toolkit/debugger/ci.yml?branch=main&style=flat-square&logo=github&label=ci)](https://github.com/munich-quantum-toolkit/debugger/actions/workflows/ci.yml)
[![CD](https://img.shields.io/github/actions/workflow/status/munich-quantum-toolkit/debugger/cd.yml?style=flat-square&logo=github&label=cd)](https://github.com/munich-quantum-toolkit/debugger/actions/workflows/cd.yml)
[![Documentation](https://img.shields.io/readthedocs/mqt-debugger?logo=readthedocs&style=flat-square)](https://mqt.readthedocs.io/projects/debugger)
[![codecov](https://img.shields.io/codecov/c/github/munich-quantum-toolkit/debugger?style=flat-square&logo=codecov)](https://codecov.io/gh/munich-quantum-toolkit/debugger)

<p align="center">
  <a href="https://mqt.readthedocs.io">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/logo-mqt-dark.svg" width="60%">
      <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/logo-mqt-light.svg" width="60%" alt="MQT Logo">
    </picture>
  </a>
</p>

# MQT Debugger - A Quantum Circuit Debugging Tool

MQT Debugger is a tool for debugging quantum circuits.
It is part of the [_Munich Quantum Toolkit (MQT)_](https://mqt.readthedocs.io).

<p align="center">
  <a href="https://mqt.readthedocs.io/projects/debugger">
  <img width=30% src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=read%20the%20docs" alt="Documentation" />
  </a>
</p>

## Key Features

- Proposes an interface for the simulation of circuits and diagnosis of errors
- Implementation built upon [MQT Core](https://github.com/munich-quantum-toolkit/core), the backbone of the MQT
- Povides a Debugger Adapter Protocol (DAP) server that can be used to integrate the debugger into IDEs

If you have any questions, feel free to create a [discussion](https://github.com/munich-quantum-toolkit/debugger/discussions) or an [issue](https://github.com/munich-quantum-toolkit/debugger/issues) on [GitHub](https://github.com/munich-quantum-toolkit/debugger).

## Contributors and Supporters

The _[Munich Quantum Toolkit (MQT)](https://mqt.readthedocs.io)_ is developed by the [Chair for Design Automation](https://www.cda.cit.tum.de/) at the [Technical University of Munich](https://www.tum.de/) and supported by the [Munich Quantum Software Company (MQSC)](https://munichquantum.software).
Among others, it is part of the [Munich Quantum Software Stack (MQSS)](https://www.munich-quantum-valley.de/research/research-areas/mqss) ecosystem, which is being developed as part of the [Munich Quantum Valley (MQV)](https://www.munich-quantum-valley.de) initiative.

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-logo-banner-dark.svg" width="90%">
    <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-logo-banner-light.svg" width="90%" alt="MQT Partner Logos">
  </picture>
</p>

Thank you to all the contributors who have helped make MQT Debugger a reality!

<p align="center">
  <a href="https://github.com/munich-quantum-toolkit/debugger/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=munich-quantum-toolkit/debugger" alt="Contributors to munich-quantum-toolkit/debugger" />
  </a>
</p>

The MQT will remain free, open-source, and permissively licensed—now and in the future.
We are firmly committed to keeping it open and actively maintained for the quantum computing community.

To support this endeavor, please consider:

- Starring and sharing our repositories: https://github.com/munich-quantum-toolkit
- Contributing code, documentation, tests, or examples via issues and pull requests
- Citing the MQT in your publications (see [Cite This](#cite-this))
- Citing our research in your publications (see [References](https://mqt.readthedocs.io/projects/debugger/en/latest/references.html))
- Using the MQT in research and teaching, and sharing feedback and use cases
- Sponsoring us on GitHub: https://github.com/sponsors/munich-quantum-toolkit

<p align="center">
  <a href="https://github.com/sponsors/munich-quantum-toolkit">
  <img width=20% src="https://img.shields.io/badge/Sponsor-white?style=for-the-badge&logo=githubsponsors&labelColor=black&color=blue" alt="Sponsor the MQT" />
  </a>
</p>

## Getting Started

`mqt.debugger` is available via [PyPI](https://pypi.org/project/mqt.debugger/).

```console
(.venv) $ pip install mqt.debugger
```

The following code gives an example on the usage:

```python3
from mqt import debugger

state = debugger.create_ddsim_simulation_state()
with open("code.qasm", "r") as f:
    state.load_code(f.read())
f.run_simulation()
print(f.get_state_vector_full())
```

**Detailed documentation and examples are available at [ReadTheDocs](https://mqt.readthedocs.io/projects/debugger).**

## System Requirements

Building the project requires a C++ compiler with support for C++20 and CMake 3.26 or newer.
For details on how to build the project, please refer to the [documentation](https://mqt.readthedocs.io/projects/debugger).
Building (and running) is continuously tested under Linux, macOS, and Windows using the [latest available system versions for GitHub Actions](https://github.com/actions/runner-images).
MQT Debugger is compatible with all [officially supported Python versions](https://devguide.python.org/versions/).

## Cite This

Please cite the work that best fits your use case.

### MQT Debugger (the tool)

When citing the software itself or results produced with it, cite the MQT Debugger paper:

```bibtex
@misc{rovara2024debugging,
  title        = {A Framework for Debugging Quantum Programs},
  author       = {Rovara, Damian and Burgholzer, Lukas and Wille, Robert},
  year         = {2024},
  eprint       = {2412.12269},
  eprinttype   = {arxiv}
}
```

### The Munich Quantum Toolkit (the project)

When discussing the overall MQT project or its ecosystem, cite the MQT Handbook:

```bibtex
@inproceedings{mqt,
  title        = {The {{MQT}} Handbook: {{A}} Summary of Design Automation Tools and Software for Quantum Computing},
  shorttitle   = {{The MQT Handbook}},
  author       = {Wille, Robert and Berent, Lucas and Forster, Tobias and Kunasaikaran, Jagatheesan and Mato, Kevin and Peham, Tom and Quetschlich, Nils and Rovara, Damian and Sander, Aaron and Schmid, Ludwig and Schoenberger, Daniel and Stade, Yannick and Burgholzer, Lukas},
  year         = 2024,
  booktitle    = {IEEE International Conference on Quantum Software (QSW)},
  doi          = {10.1109/QSW62656.2024.00013},
  eprint       = {2405.17543},
  eprinttype   = {arxiv},
  addendum     = {A live version of this document is available at \url{https://mqt.readthedocs.io}}
}
```

### Peer-Reviewed Research

When citing the underlying methods and research, please reference the most relevant peer-reviewed publications from the list below:

[[1]](https://arxiv.org/abs/2412.12269)
D. Rovara, L. Burgholzer, and R. Wille.
A Framework for Debugging Quantum Programs.

[[2]](https://arxiv.org/abs/2412.14252)
D. Rovara, L. Burgholzer, and R. Wille.
Automatically Refining Assertions for Efficient Debugging of Quantum Programs.

[[3]](https://arxiv.org/abs/2505.03885)
D. Rovara, L. Burgholzer, and R. Wille.
A Framework for the Efficient Evaluation of Runtime Assertions on Quantum Computers.

---

## Acknowledgements

The Munich Quantum Toolkit has been supported by the European Research Council (ERC) under the European Union's Horizon 2020 research and innovation program (grant agreement No. 101001318), the Bavarian State Ministry for Science and Arts through the Distinguished Professorship Program, as well as the Munich Quantum Valley, which is supported by the Bavarian state government with funds from the Hightech Agenda Bayern Plus.

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-funding-footer-dark.svg" width="90%">
    <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-funding-footer-light.svg" width="90%" alt="MQT Funding Footer">
  </picture>
</p>
