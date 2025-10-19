#!/usr/bin/env python3
import subprocess
from pathlib import Path

# this script lives in bitloop/scripts/
examples_dir = Path(__file__).resolve().parent.parent / "examples"

for project in sorted(examples_dir.iterdir()):
    if project.is_dir():
        manifest = project / "vcpkg.json"
        if manifest.exists():
            print(f"\n== {project.name} ==")
            subprocess.run(["vcpkg", "x-update-baseline"], cwd=project)
