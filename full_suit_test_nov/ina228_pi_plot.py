
import csv
import argparse
from pathlib import Path

import matplotlib.pyplot as plt


# ============================================================
# 1. Command-line arguments
# ============================================================

parser = argparse.ArgumentParser(
    description="Plot INA228 voltage, current and power data"
)

parser.add_argument(
    "file",
    help="Path to INA228 CSV file"
)

parser.add_argument(
    "--save",
    action="store_true",
    help="Save plots as PNG files"
)

args = parser.parse_args()

csv_path = Path(args.file).expanduser()

if not csv_path.is_file():
    raise FileNotFoundError(f"File not found: {csv_path}")


# ============================================================
# 2. Read CSV data
# ============================================================

timestamps = []

voltage1 = []
current1 = []
power1 = []

voltage2 = []
current2 = []
power2 = []

with open(csv_path, "r", newline="") as f:

    reader = csv.DictReader(f)

    # Automatically detect the number of sensors
    two_sensors = "Voltage2_V" in reader.fieldnames

    for row in reader:

        timestamps.append(float(row["Timestamp_ms"]))

        voltage1.append(float(row["Voltage1_V"]))
        current1.append(float(row["Current1_A"]))
        power1.append(float(row["Power1_W"]))

        if two_sensors:

            voltage2.append(float(row["Voltage2_V"]))
            current2.append(float(row["Current2_A"]))
            power2.append(float(row["Power2_W"]))

if not timestamps:
    raise ValueError("No samples found in CSV file")


# ============================================================
# 3. Convert timestamps from milliseconds to seconds
# ============================================================

start_time = timestamps[0]

time_s = [
    (t - start_time) / 1000
    for t in timestamps
]

num_samples = len(time_s)
duration = time_s[-1]

if duration > 0:
    actual_rate = (num_samples - 1) / duration
else:
    actual_rate = 0


# ============================================================
# 4. Plot voltage, current and power
# ============================================================

fig, axes = plt.subplots(
    3, 1,
    figsize=(12, 9),
    sharex=True
)

# Voltage
axes[0].plot(
    time_s, voltage1,
    label="Sensor 1"
)

if two_sensors:
    axes[0].plot(
        time_s, voltage2,
        label="Sensor 2"
    )

axes[0].set_ylabel("Voltage (V)")
axes[0].set_title("Bus Voltage")
axes[0].grid(True, alpha=0.3)
axes[0].legend()


# Current
axes[1].plot(
    time_s, current1,
    label="Sensor 1"
)

if two_sensors:
    axes[1].plot(
        time_s, current2,
        label="Sensor 2"
    )

axes[1].set_ylabel("Current (A)")
axes[1].set_title("Current")
axes[1].grid(True, alpha=0.3)
axes[1].legend()


# Power
axes[2].plot(
    time_s, power1,
    label="Sensor 1"
)

if two_sensors:
    axes[2].plot(
        time_s, power2,
        label="Sensor 2"
    )

axes[2].set_xlabel("Time (s)")
axes[2].set_ylabel("Power (W)")
axes[2].set_title("Power")
axes[2].grid(True, alpha=0.3)
axes[2].legend()


fig.suptitle(
    f"INA228 Measurements — {csv_path.stem}",
    fontsize=13
)

plt.tight_layout()


# ============================================================
# 5. Print summary
# ============================================================

print("\nINA228 Data Summary")
print("=" * 45)

print(f"File:          {csv_path.name}")
print(f"Sensors:       {2 if two_sensors else 1}")
print(f"Samples:       {num_samples}")
print(f"Duration:      {duration:.3f} s")
print(f"Sampling rate: {actual_rate:.2f} Hz")

print("\nSensor 1")
print(f"Mean voltage:  {sum(voltage1)/num_samples:.4f} V")
print(f"Mean current:  {sum(current1)/num_samples:.4f} A")
print(f"Mean power:    {sum(power1)/num_samples:.4f} W")
print(f"Peak current:  {max(current1):.4f} A")

if two_sensors:

    print("\nSensor 2")
    print(f"Mean voltage:  {sum(voltage2)/num_samples:.4f} V")
    print(f"Mean current:  {sum(current2)/num_samples:.4f} A")
    print(f"Mean power:    {sum(power2)/num_samples:.4f} W")
    print(f"Peak current:  {max(current2):.4f} A")

print("=" * 45)


# ============================================================
# 6. Save and display plots
# ============================================================

if args.save:

    plot_path = csv_path.with_suffix(".png")

    fig.savefig(
        plot_path,
        dpi=200,
        bbox_inches="tight"
    )

    print(f"Plot saved to: {plot_path}")

plt.show()
