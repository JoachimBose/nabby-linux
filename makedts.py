import subprocess

# Print a message
subprocess.run(["echo", "This python script makes kernel after DTS(i) update for the linux nabby-board"])
subprocess.run(["echo", "---"])
subprocess.run(["echo", "---> make linux-update-defconfig"])
subprocess.run(["make", "linux-update-defconfig"],cwd="/home/raj/buildroot-2025.11.2")
subprocess.run(["echo", "---> Make linux-rebuild"])
subprocess.run(["make", "linux-rebuild"],cwd="/home/raj/buildroot-2025.11.2")



