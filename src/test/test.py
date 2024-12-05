import os
import subprocess

# Define the device to format
DEVICE = "/dev/vdb"

# Ensure the script is run as root
if os.geteuid() != 0:
    print("This script must be run as root. Use sudo.")
    exit(1)

# Run mkfs command
print(f"Formatting {DEVICE}...")
result = subprocess.run(["./mkfs.uwu", DEVICE])

if result.returncode == 0:
    print("Formatting completed successfully.")
else:
    print("Error: mkfs failed.")
    exit(result.returncode)

result = subprocess.run(["./mount.uwu", DEVICE, "/mnt", "-f", "-s", "-o", "allow_other"])

if result.returncode != 0:
    print("Mounting completed successfully.")
