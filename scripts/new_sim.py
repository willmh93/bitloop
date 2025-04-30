import os
import subprocess
import platform

# Paths
SIMULATIONS_FOLDER = "../src/simulations"
TEMPLATES_FOLDER = "../templates"

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
    #else:
    #    # Linux/macOS: Regenerate using default CMake generator (e.g., Unix Makefiles)
    #    result = subprocess.run(
    #        ["cmake", "-S", ".", "-B", BUILD_DIR],
    #        capture_output=True,
    #        text=True
    #    )
    #    if result.returncode != 0:
    #        print("CMake regeneration failed!")
    #        print(result.stderr)
    #    else:
    #        print("CMake regeneration complete!")
    #        print(result.stdout)

# Function to read a template file
def read_template(template_name):
    template_path = os.path.join(TEMPLATES_FOLDER, template_name)
    try:
        with open(template_path, "r") as template_file:
            return template_file.read()
    except FileNotFoundError:
        print(f"Error: Template file {template_name} not found.")
        exit(1)

# Function to generate a simulation
def generate_simulation():
    # Prompt for simulation details
    class_name = input("Enter simulation class name (e.g. ParticleSim): ").strip()
    simulation_name = input("Enter simulation full name (e.g. Particle Simulation): ").strip()
    
    # Derive filenames and header guard
    header_file = f"{class_name}.h"
    cpp_file = f"{class_name}.cpp"

    # Read templates
    header_template = read_template("../templates/sim_header.h")
    cpp_template = read_template("../templates/sim_source.cpp")

    print(f"Generated: {header_template}")
    

    # Fill templates with values
    header_content = header_template.format(CLASS_NAME=class_name)
    cpp_content = cpp_template.format(
        HEADER_FILE=header_file,
        CLASS_NAME=class_name,
        SIM_NAME=simulation_name
    )

    # Ensure the simulations folder exists
    if not os.path.exists(SIMULATIONS_FOLDER):
        os.makedirs(SIMULATIONS_FOLDER)

    # Write header file
    header_path = os.path.join(SIMULATIONS_FOLDER, header_file)
    with open(header_path, "w") as header:
        header.write(header_content)
    print(f"Generated: {header_path}")

    # Write CPP file
    cpp_path = os.path.join(SIMULATIONS_FOLDER, cpp_file)
    with open(cpp_path, "w") as cpp:
        cpp.write(cpp_content)
    print(f"Generated: {cpp_path}")

    print("Simulation files created successfully!")




if __name__ == "__main__":
    generate_simulation()

    os.chdir(os.pardir)
    regenerate_project()