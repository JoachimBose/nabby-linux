import subprocess

# Print a message
subprocess.run(["echo", "This python script compiles and loads helloworldtest on the linux nabby-board"])
subprocess.run(["echo", "---"])
subprocess.run(["echo", "---> clean helloworldtest"])
subprocess.run(["make", "helloworldtest-dirclean"],cwd="/home/raj/buildroot-2025.11.2")
subprocess.run(["echo", "---> Make helloworldtest"])
subprocess.run(["make", "helloworldtest"],cwd="/home/raj/buildroot-2025.11.2")
subprocess.run(["echo", "---> make kernel"])
subprocess.run(["make"],cwd="/home/raj/buildroot-2025.11.2")

# Wait for user
input("Invoke FEL mode, press Enter to continue...")
# Run sunxi-fel inside the images directory
subprocess.run(
    ["sudo", "sunxi-fel", "-p", "spiflash-write", "0", "flash.bin"],
    cwd="/home/raj/buildroot-2025.11.2/output/images")

# Wait for user
input("Reset target...")
# start minicom
subprocess.run(["sudo", "minicom"])


