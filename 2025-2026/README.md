# exoskeleton-power

Code for the Power subteam of the McMaster Exoskeleton team.

For the 2025–2026 year, the Power Architecture team runs an STM32 MCU on the Low Voltage (LV) PCB. It monitors voltage and current across the power system using INA228 sensors, controls a precharge circuit (main contactor and motor relays), and reports telemetry to the Dashboard over the CAN bus.

## Repository Layout

Each STM32CubeIDE project represents a tested milestone. New projects build on previously tested code to keep testing complexity down. The current integrated system lives at the top level; earlier milestones are kept under `tests/stm32/`.

| Folder | Description |
| --- | --- |
| [`power_system/`](./power_system) | Current integrated system: precharge FSM, five-sensor monitoring, CAN telemetry, and UART data logger. Start here. |
| [`tests/stm32/precharge_system/`](./tests/stm32/precharge_system) | Precharge state-machine and contactor control development |
| [`tests/stm32/ina228_double_logger/`](./tests/stm32/ina228_double_logger) | Two-sensor INA228 logging |
| [`tests/stm32/ina228_logger/`](./tests/stm32/ina228_logger) | Single-sensor INA228 UART data logger |
| [`tests/stm32/dashboard_integration/`](./tests/stm32/dashboard_integration) | CAN integration with the Dashboard |

See [`power_system/README.md`](./power_system/README.md) for the full firmware and host-tool documentation (CAN frame format, I2C addresses, configuration reference, and Python scripts).

## Toolchain

- **IDE:** STM32CubeIDE 1.19.0
- **Dev board:** NUCLEO-F446RE
- **Host testing:** PySerial for UART communication; `python-can` for CAN

## Working on Code

- Push to `main` in a new folder only once the code has been tested.
- Reuse as much previously tested code as practical to reduce testing complexity.
- Document how to build and run each project in a README or in a comment block at the top of the code.

Refer to the [Embedded Repository's README](https://github.com/McMaster-Exoskeleton/exoskeleton-embedded#readme) for shared coding standards, APIs, and CAN communication conventions.
