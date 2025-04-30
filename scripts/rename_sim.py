import os
import re
import subprocess
import platform

# Paths
SIMULATIONS_FOLDER = "../src/simulations"
FILE_EXTENSIONS = {".cpp", ".h", ".hpp", ".cxx", ".cc", ".hh"}

BUILD_DIR = "../build"  # Adjust to match your actual build directory
GENERATOR = "Visual Studio 17 2022"  # Adjust for your Visual Studio version
CONFIG = "Debug"


def regenerate_project():
    print("Regenerating project files...")
    if platform.system() == "Windows":
        # Windows: Regenerate Visual Studio project files
        result = subprocess.run(
            ["cmake", "-S", ".", "-B", BUILD_DIR, "-G", GENERATOR, f"-DCMAKE_BUILD_TYPE={CONFIG}"],
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            print("CMake regeneration failed!")
            print(result.stderr)
        else:
            print("CMake regeneration complete!")
            print(result.stdout)


def rename_class_in_file(file_path, old_class, new_class):
    """Replaces all occurrences of old_class with new_class in the given file."""
    with open(file_path, "r", encoding="utf-8") as file:
        content = file.read()

    # Replace only whole words (avoid partial replacements)
    modified_content = re.sub(rf"\b{re.escape(old_class)}\b", new_class, content)

    if modified_content != content:
        with open(file_path, "w", encoding="utf-8") as file:
            file.write(modified_content)
        print(f"Updated: {file_path}")

def rename_class_in_project(source_dir, old_class, new_class):
    """Recursively rename the class in all C++ source files."""
    for root, _, files in os.walk(source_dir):
        for file in files:
            if any(file.endswith(ext) for ext in FILE_EXTENSIONS):
                file_path = os.path.join(root, file)
                rename_class_in_file(file_path, old_class, new_class)

def rename_files(source_dir, old_class, new_class):
    """Renames any .h/.cpp files matching the old class name."""
    for root, _, files in os.walk(source_dir):
        for file in files:
            if file.startswith(old_class):  # Find files that start with old class name
                old_file_path = os.path.join(root, file)
                new_file_name = file.replace(old_class, new_class, 1)  # Replace class name in filename
                new_file_path = os.path.join(root, new_file_name)

                os.rename(old_file_path, new_file_path)
                print(f"Renamed: {old_file_path} → {new_file_path}")

# Function to generate a simulation
def rename_simulation():
    old_ns = input("Enter current class name (e.g. ParticleSim): ").strip()
    new_ns = input("Enter new class name (e.g. FluidSim): ").strip()
    #new_sim_name = input("Enter new full simulation name (e.g. Fluid Simulation): ").strip()

    old_scene_name   = old_ns + "_Scene"
    old_project_name = old_ns + "_Project"
    new_scene_name   = new_ns + "_Scene"
    new_project_name = new_ns + "_Project"

    rename_class_in_project(SIMULATIONS_FOLDER, old_scene_name, new_scene_name)
    rename_class_in_project(SIMULATIONS_FOLDER, old_project_name, new_project_name)
    rename_files(SIMULATIONS_FOLDER, old_ns, new_ns)

if __name__ == "__main__":
    rename_simulation()
    #regenerate_project()