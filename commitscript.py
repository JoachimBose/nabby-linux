import subprocess

# Print a message
subprocess.run(["echo", "This python script commits nabby-linux to github"])
subprocess.run(["echo", "---"])
# subprocess.run(["echo", "---> add new files with git add filename"])

subprocess.run(["git","config", "--global","user.email","raj.bose@gmail.com"])
subprocess.run(["git","config", "--global","user.name", "Raj Bose"])

subprocess.run(["echo", "---> git status"])
subprocess.run(["git", "status"])

#input("press Enter to continue...")

subprocess.run(["echo", "---> commit all"])
subprocess.run(["git", "add", "."])


description = input("---> Please enter description: ")
subprocess.run(["echo", "---> git commit -m description"])
subprocess.run(["git","commit","-m", description])
subprocess.run(["echo", "---> git push"])
subprocess.run(["git","push"])











