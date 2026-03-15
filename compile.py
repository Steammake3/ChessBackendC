import os
import subprocess

def list_files_in_directory(path='.'):
    files_list = []
    for entry in os.listdir(path):
        full_path = os.path.join(path, entry)
        base = os.path.basename(full_path)  # important
        name, ext = os.path.splitext(base)
        if os.path.isfile(full_path) and ext == ".c" and name != "tests":
            files_list.append(entry)
    return files_list

files = list_files_in_directory()

try:
    subprocess.run(["gcc", "-O1", *files, "-o", "bot.exe"], check=True, capture_output=True, text=True)
    print("Compilation succeeded.")
except subprocess.CalledProcessError as e:
    print(f"Compilation failed with code {e.returncode}: {e.stderr}")