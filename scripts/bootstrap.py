import sys
import platform
import subprocess
import os
from pathlib import Path

script_dir = Path(__file__).resolve().parent

def run_bootstrap():
    if platform.system() != "Linux":
        print("Error: 'bootstrap' is only supported on Linux.", file=sys.stderr)
        sys.exit(1)

    # Determine repo root
    bitloop_root = script_dir.parent
    bashrc = Path.home() / ".bashrc"
    lines = [
        f"export BITLOOP_ROOT=\"{bitloop_root}\"",
        f"export PATH=\"$BITLOOP_ROOT:$PATH\""
    ]

    # Append to ~/.bashrc if not already present
    content = bashrc.read_text(encoding='utf-8') if bashrc.exists() else ""
    with bashrc.open('a', encoding='utf-8') as f:
        for line in lines:
            if line not in content:
                f.write(f"\n{line}")

    # Make the wrapper script executable
    bitloop_sh = bitloop_root / "bitloop.sh"
    if bitloop_sh.exists():
        subprocess.run(["chmod", "+x", str(bitloop_sh)], check=True)
    else:
        print(f"Warning: bitloop.sh not found at {bitloop_sh}", file=sys.stderr)

    # Create a 'bitloop' symlink for easier invocation
    link_path = bitloop_root / "bitloop"
    if not link_path.exists() and bitloop_sh.exists():
        try:
            os.symlink(str(bitloop_sh), str(link_path))
        except OSError as e:
            print(f"Warning: could not create symlink {link_path}: {e}", file=sys.stderr)

    # Reload the shell config (may not affect current shell)
    try:
        subprocess.run(["bash", "-lc", "source ~/.bashrc"], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Warning: failed to source ~/.bashrc: {e}", file=sys.stderr)

    # Install dependencies
    deps = [
        "git", "cmake", "ninja-build", "build-essential", "curl", "zip", "unzip", "tar", "pkg-config",
        "autoconf", "automake", "autoconf-archive", "libltdl-dev",
        "libx11-dev", "libxft-dev", "libxext-dev",
        "libwayland-dev", "libxkbcommon-dev", "libegl1-mesa-dev", "wayland-protocols", "libdecor-0-dev",
        "libibus-1.0-dev", "nasm"
    ]
    try:
        subprocess.run(["sudo", "apt-get", "update"], check=True)
        subprocess.run(["sudo", "apt-get", "install", "-y"] + deps, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error installing dependencies: {e}", file=sys.stderr)
        sys.exit(e.returncode)

    print("Bootstrap completed: BITLOOP_ROOT set, PATH updated, bitloop.sh executable, 'bitloop' link created, and dependencies installed.")
    
    
if __name__ == "__main__":
    run_bootstrap()
