Import('env')
import json

import subprocess

def get_git_version():
    try:
        # Equivalent of: git describe --always --dirty --exclude "*"
        result = subprocess.run(
            ["git", "describe", "--always", "--dirty", "--exclude", "*"],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return "nogit"

PACKAGE_FILE = "package.json"

with open(PACKAGE_FILE, "r") as package:
    version = json.load(package)["version"]
    env.Append(BUILD_FLAGS=[f"-DWLED_VERSION={version}-{get_git_version()}"])
