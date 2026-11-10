import argparse
import csv
import os
import time
from datetime import datetime
from pathlib import Path

import serial


# ============================================================
# 1. Command-line arguments
# ============================================================

parser = argparse.ArgumentParser(
    description="INA228 data logger for STM32 and Raspberry Pi"
)

parser.add_argument(
    "--sensors",
    type=int,
    choices=[1, 2],
    default=1,
    help="Number of INA228 sensors (1 or 2)"
)

parser.add_argument(
    "--rate",
    type=int,
    default=100,
    help="sampling frequency"
)


parser.add_argument(
    "--duration",
    type=int,
    default=60,
    help="Recording duration in seconds"
)

parser.add_argument(
    "--port",
    default="/dev/ttyACM0",
    help="STM32 USB serial port"
)

parser.add_argument(
    "--baud",
    type=int,
    default=115200,
    help="UART baud rate"
)

args = parser.parse_args()

if args.rate <= 0 or args.duration <= 0:
    parser.error("Sampling rate and duration must be positive")


# ============================================================
# 2. Create output directory and CSV file
# ============================================================

output_dir = Path.home() / "ina228_logs"
output_dir.mkdir(parents=True, exist_ok=True)

timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

filename = output_dir / (
    f"ina228_{args.sensors}sensors_{timestamp}.csv"
)

if args.sensors == 1:
    headers = [
        "Timestamp_ms",
        "Voltage1_V",
        "Current1_A",
        "Power1_W"
    ]
else:
    headers = [
        "Timestamp_ms",
        "Voltage1_V",
        "Current1_A",
        "Power1_W",
        "Voltage2_V",
        "Current2_A",
        "Power2_W"
    ]

expected_fields = len(headers)
expected_samples = args.rate * args.duration


# ============================================================
# 3. Initialize serial connection
# ============================================================

print("INA228 Raspberry Pi Data Logger")
print("--------------------------------")
print(f"Sensors:       {args.sensors}")
print(f"Sampling rate: {args.rate} Hz")
print(f"Duration:      {args.duration} s")
print(f"Expected:      {expected_samples} samples")
print(f"Serial port:   {args.port}")
print(f"Output file:   {filename}")
print()

sample_count = 0
invalid_count = 0
completed = False

try:
    with serial.Serial(
        port=args.port,
        baudrate=args.baud,
        timeout=1
    ) as ser:

        # Allow USB serial connection to settle
        time.sleep(2)
        ser.reset_input_buffer()

        # ====================================================
        # 4. Send START command to STM32
        # ====================================================

        command = (
            f"START,{args.rate},{args.duration},"
            f"{args.sensors}\n"
        )

        print(f"Sending command: {command.strip()}")

        ser.write(command.encode("ascii"))
        ser.flush()

        response = ser.readline().decode(
            "ascii", errors="replace"
        ).strip()

        while response.startswith("DEBUG:"):
            print(response)
            response = ser.readline().decode(
                "ascii", errors="replace"
            ).strip()

        if response != "OK":
            raise RuntimeError(
                f"STM32 did not acknowledge command: {response!r}"
            )

        print("STM32 acknowledged START")
        print("Recording...")
        print()

        # ====================================================
        # 5. Receive and save measurements
        # ====================================================

        with open(filename, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(headers)

            last_data_time = time.monotonic()
            last_sync_time = last_data_time

            while True:

                raw = ser.readline()
                line = raw.decode(
                    "ascii", errors="replace"
                ).strip()

                if not line:
                    if time.monotonic() - last_data_time > 10:
                        print("ERROR: No serial data for 10 seconds.")
                        break
                    continue

                last_data_time = time.monotonic()

                if line.startswith("DONE,"):
                    completed = True
                    print("STM32 reported acquisition complete.")
                    break

                if line.startswith("ERR"):
                    print(f"STM32 error: {line}")
                    break

                # Parse CSV row
                values = line.split(",")

                if len(values) != expected_fields:
                    invalid_count += 1
                    print(f"Invalid field count: {line}")
                    continue

                try:
                    timestamp_ms = int(values[0])
                    measurements = [
                        float(value)
                        for value in values[1:]
                    ]

                except ValueError:
                    invalid_count += 1
                    print(f"Invalid measurement: {line}")
                    continue

                writer.writerow(
                    [timestamp_ms] + measurements
                )

                sample_count += 1

                # Commit data to microSD approximately
                # once per second
                now = time.monotonic()

                if now - last_sync_time >= 1.0:
                    f.flush()
                    os.fsync(f.fileno())
                    last_sync_time = now

            # Final synchronization
            f.flush()
            os.fsync(f.fileno())

except serial.SerialException as e:
    print(f"Serial communication error: {e}")

except RuntimeError as e:
    print(f"Logger error: {e}")

except KeyboardInterrupt:
    print("\nRecording interrupted by user.")


# ============================================================
# 6. Summary
# ============================================================

print()
print("========================================")
print("INA228 Data Collection Summary")
print("========================================")
print(f"Sensors:          {args.sensors}")
print(f"Expected samples: {expected_samples}")
print(f"Received samples: {sample_count}")
print(f"Invalid records:  {invalid_count}")
print(f"Completed:        {completed}")

if sample_count > 0:
    print(f"CSV file:         {filename}")

if completed and sample_count == expected_samples:
    print("Status: SUCCESS")
else:
    print("Status: INCOMPLETE / CHECK DATA")

print("========================================")
