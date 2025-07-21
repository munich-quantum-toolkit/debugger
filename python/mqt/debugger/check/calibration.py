# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Provides device calibration information for the runtime check."""

from __future__ import annotations

from dataclasses import dataclass, field

missing_optionals: list[str] = []

try:
    import numpy as np
except ImportError:
    missing_optionals.append("numpy")
try:
    from qiskit import QuantumCircuit
except ImportError:
    missing_optionals.append("qiskit")


@dataclass
class Calibration:
    """Represents the calibration data for a quantum device."""

    error_rate_1q: float
    error_rate_2q: float
    error_rate_measurement: float
    specific_gate_errors: dict[str, float] = field(default_factory=dict)
    t: float = 0.0

    def get_expected_success_probability(self, code: str) -> float:
        """Get the expected success probability of the device for some program.

        ... (rest of the code remains the same)

        Currently, expected success probability is computed in terms of measurement and gate fidelities.

        Args:
            code (str): The program to check.

        Returns:
            float: The expected success probability.
        """
        if missing_optionals:
            raise ImportError(
                "The following optional dependencies are required to use this feature: " + ", ".join(missing_optionals)
            )
        fidelity_measurement = 1 - self.error_rate_measurement
        fidelity_1q = 1 - self.error_rate_1q
        fidelity_2q = 1 - self.error_rate_2q

        gate_fidelity = 1.0
        qc = QuantumCircuit.from_qasm_str(code)
        for instruction in qc.data:
            gate_type = instruction.name
            if gate_type in self.specific_gate_errors:
                gate_fidelity *= 1 - self.specific_gate_errors[gate_type]
                continue
            if gate_type == "barrier":
                continue
            if len(instruction.qubits) == 1:
                if gate_type == "measure":
                    gate_fidelity *= fidelity_measurement
                else:
                    gate_fidelity *= fidelity_1q
            else:
                gate_fidelity *= fidelity_2q

        qubit_times = dict.fromkeys(qc.qubits, 0)

        for instruction in qc.data:
            max_time = max(qubit_times[qubit] for qubit in instruction.qubits)
            for qubit in instruction.qubits:
                qubit_times[qubit] = max_time + 1
        qubit_fidelity = 1
        for qubit in qubit_times:
            qubit_fidelity *= np.exp(-qubit_times[qubit] * self.t)
        return gate_fidelity * qubit_fidelity

    @classmethod
    def example(cls) -> Calibration:
        """Get an example calibration."""
        return cls(0.01, 0.01, 0.01)
