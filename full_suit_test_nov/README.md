# Full Suit Test — INA228 Data Logger

## Overview

This project collects voltage, current, and power from one or two INA228 sensors during exoskeleton testing. The STM32 NUCLEO-F446RE handles I²C measurements and sampling; a Raspberry Pi receives timestamped samples through the Nucleo ST-LINK USB virtual COM port and saves them to CSV on its microSD card. Samples are streamed rather than buffered for the entire test in STM32 RAM.

## Hardware and configuration

- STM32 NUCLEO-F446RE and Raspberry Pi 4.
- One or two INA228 sensors on I²C1: PB6 (SCL), PB7 (SDA), 400 kHz in the bench firmware.
- Shared ground and 3.3 V-compatible I²C logic; ensure appropriate external pull-ups (including any on the breakout boards). The firmware disables internal pull-ups.
- Connect the Pi's USB-A port to the Nucleo ST-LINK USB connector using a data-capable USB cable. Use the Nucleo USB-power configuration when powering it from the Pi; do not simultaneously connect another power source without verifying the board configuration.
- Intended moving-test power arrangement: a suitable independent 5 V portable pack powers the Pi and Nucleo so logging can continue when the main suit supply is stopped. Verify power capacity and that the logger cannot backfeed or energize the stopped system. E-stop operation has not yet been validated.

### Sensor addresses and calibration

The bench-test sensor 1 address is `0x41` (Adafruit breakout A0 jumper soldered); sensor 2 uses `0x40` (A0/A1 grounded). STM32 HAL requires the 7-bit address shifted left by one:

```c
#define INA228_ADDR1 (0x41 << 1)
#define INA228_ADDR2 (0x40 << 1)
```

Check `Core/Inc/ina228_driver_logging.h` against the actual hardware before flashing. The planned suit sensor with both A0 and A1 tied to SDA uses `0x4A`, not `0x41`. Do not assume bench-test addresses match final hardware.

Verify `SHUNT1_OHMS`, `SHUNT2_OHMS` in `Core/Src/main.c` and the current scaling in the driver header against the installed shunts and expected current range. The uploaded development version used 3 mΩ and 5 mΩ respectively; these are not confirmed calibration values for the final hardware. Current and power accuracy under load has not been validated.

## Build and flash

1. Import this directory into STM32CubeIDE 1.19.0 using **File → Import → General → Existing Projects into Workspace**.
2. Build the project. CubeIDE regenerates the ignored `Debug/` build outputs.
3. Connect the Nucleo to the development computer and flash using **Run As → STM32 C/C++ Application**.
4. Connect the Nucleo to the Raspberry Pi instead of the development computer. Confirm the sensor wiring and power before starting.

The firmware uses USART2 at the bench-test baud rate of 115200 and the ST-LINK virtual COM port.

## Raspberry Pi setup

Copy `ina228_pi_logger.py` and `ina228_pi_plot.py` from this directory to the Pi (for example, `~/ina228_logger/`). On the Pi:

```bash
mkdir -p ~/ina228_logger
cd ~/ina228_logger
python3 -m venv .venv
source .venv/bin/activate
pip install pyserial matplotlib
python -m serial.tools.list_ports -v
```

The ST-LINK port is typically `/dev/ttyACM0`. Use `--port` if it differs. If access is denied, check the device permissions and the user's serial-device group membership (typically `dialout`).

## Collect data

From the Pi's project directory with the virtual environment active:

```bash
# One sensor, 5 seconds at 100 Hz
python ina228_pi_logger.py --sensors 1 --rate 100 --duration 5

# Two sensors, 5 seconds at 100 Hz
python ina228_pi_logger.py --sensors 2 --rate 100 --duration 5

# Two sensors, 5 minutes at 100 Hz
python ina228_pi_logger.py --sensors 2 --rate 100 --duration 300
```

Start the script remotely through VNC or SSH. For SSH sessions that may disconnect during testing, run the logger inside a persistent terminal session such as `tmux`. Keep the independent logging supply connected until the file has been saved and the Pi can be shut down normally.

The Pi sends `START,<rate_hz>,<duration_s>,<sensors>`; the firmware returns `OK` and streams CSV rows, followed by `DONE,<samples_sent>,<overruns>`. The logger handles the completion message and checks the sample count. Inspect any error messages and do not treat an incomplete recording as a successful test.

## Data and plotting

CSV files are saved in `~/ina228_logs/` with timestamped filenames. Each row contains the elapsed STM32 timestamp in milliseconds, voltage (V), current (A), and power (W) for each selected sensor. In dual mode the columns are:

```text
Timestamp_ms,Voltage1_V,Current1_A,Power1_W,Voltage2_V,Current2_A,Power2_W
```

Plot any recording using its actual filename:

```bash
python ina228_pi_plot.py ~/ina228_logs/ina228_2sensors_20261110_181737.csv --save
```

The script displays voltage, current, and power versus elapsed time, prints a summary, and saves a PNG beside the CSV when `--save` is supplied. It can also run on a laptop with Python and Matplotlib installed. Use the actual CSV timestamps for timing analysis.

## Validation status

| Test | Result |
|---|---|
| STM32 build and flash | Passed |
| Single-sensor 100 Hz, 5 s | 500 samples, zero invalid records |
| Dual-sensor 100 Hz, 5 s | 500 samples, zero invalid records |
| Independent voltage readings | Approximately 1.637 V (voltage divider) and 3.294 V (3.3 V rail) |
| Dual-sensor 100 Hz, 300 s | Recording completed; final timestamp 299,991 ms, consistent with the requested schedule |
| CSV logging and plotting | Passed |
| Loaded current/power accuracy and final shunt calibration | Outstanding |
| Independent-power/E-stop data retention | Outstanding |
| Integrated moving suit test | Outstanding |

The first and last timestamps of the bench recordings support the requested 100 Hz schedule; this does not certify every sample interval or simultaneous ADC conversion across sensors. Validate the outstanding items before relying on the logger for the full suit test.
