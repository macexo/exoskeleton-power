
# exoskeleton-power

Code and documentation for the Power Architecture subteam of the McMaster Exoskeleton team.

For the 2026–2027 year, the Power subteam is responsible for developing, testing, and integrating the exoskeleton's electrical power system. This repository contains the team's firmware, testing programs, and supporting documentation.

Previous development work is preserved to provide a reference for future designs and to encourage the reuse of tested components such as the INA228 current sensor.

## Repository Layout

Each academic year's work is organized into a separate folder. This allows the team to maintain previous designs and development milestones while working on new hardware and software.

| Folder | Description |
|---|---|
| [`2025-2026/`](2025-2026/) | Previous year's power system development, including STM32 firmware, precharge control, voltage and current monitoring, and CAN communication. |
| `Future FOlder/` | Current year's power system development. To be added as development progresses. |

### 2025–2026 Development

The previous team developed an STM32-based power management system using an STM32 NUCLEO-F446RE development board.

The system included:
- Voltage and current monitoring using INA228 sensors.
- Precharge sequencing and contactor control.
- CAN bus telemetry for communication with the Dashboard.
- UART data logging for testing and debugging.

The integrated firmware and earlier development milestones are available in the following locations:

| Folder | Description |
|---|---|
| [`power_system/`](2025-2026/power_system/) | Integrated firmware containing the precharge state machine, sensor monitoring, CAN telemetry, and UART logging. |
| [`tests/stm32/precharge_system/`](2025-2026/tests/stm32/precharge_system/) | Precharge state-machine and contactor control development. |
| [`tests/stm32/ina228_double_logger/`](2025-2026/tests/stm32/ina228_double_logger/) | Two-sensor INA228 data logging. |
| [`tests/stm32/ina228_logger/`](2025-2026/tests/stm32/ina228_logger/) | Single-sensor INA228 UART data logger. |
| [`tests/stm32/dashboard_integration/`](2025-2026/tests/stm32/dashboard_integration/) | CAN communication testing with the Dashboard. |

See the [2025–2026 power system README](2025-2026/power_system/README.md) for firmware documentation, communication formats, sensor configuration, and testing utilities.

## Toolchain

The following tools were used during the 2025–2026 development year and will be reused.:

- **IDE:** STM32CubeIDE 1.19.0
- **Development board:** STM32 NUCLEO-F446RE
- **Microcontroller family:** STM32F4
- **Host testing:** Python with PySerial for UART communication and `python-can` for CAN communication


## Working on Code

To maintain a reliable and organized codebase:

- Develop and test new functionality on separate branches.
- Submit changes through Pull Requests for review before merging into `main`.
- Do not submit PRs to the 2025-2026 folder. We will delete them.
- Reuse previously tested code where practical to reduce development and testing complexity.
- Document the purpose, configuration, and testing procedure for new projects.
- Include build and execution instructions in project-level README files.
- Avoid modifying previous years' files unless correcting errors or improving documentation.

Refer to the Embedded & Controls Repository's README for shared coding standards, APIs, and CAN communication conventions.
