import sys
import argparse
import os
import shutil
from pathlib import Path

PLACEHOLDER_IN_FILE = "{SIM_NAME}"
TEXT_EXTENSIONS = {".cpp", ".h", ".txt", ".json"}
RENAME_TARGETS = {"SIM_NAME.cpp", "SIM_NAME.h"}
RENAME_FOLDER_TARGET = "SIM_NAME"

def replace_in_file(path:Path, placeholder:str, project_name:str) -> None:
    if path.suffix not in TEXT_EXTENSIONS:
        return
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return
    new_text = text.replace(placeholder, project_name)
    if new_text != text:
        path.write_text(new_text, encoding="utf-8")

def rename_file_if_needed(path:Path, project_name:str) -> Path:
    if path.name not in RENAME_TARGETS:
        return path
    new_name = path.name.replace("SIM_NAME", project_name)
    new_path = path.with_name(new_name)
    if new_path.exists():
        raise FileExistsError(f"Cannot rename '{path}' to '{new_path}': destination exists.")
    path.rename(new_path)
    return new_path

def rename_folder_if_needed(path:Path, project_name:str) -> Path:
    if path.name != RENAME_FOLDER_TARGET:
        return path
    new_name = project_name
    new_path = path.with_name(new_name)
    if new_path.exists():
        raise FileExistsError(f"Cannot rename folder '{path}' to '{new_path}': destination exists.")
    path.rename(new_path)
    return new_path

def copy_template_files(src:Path, dst:Path) -> None:
    shutil.copytree(src, dst)

def create_project(bitloop_root:Path):
    name = input("Enter new project name: ").strip()
    if not name:
        print("Project name cannot be empty.", file=sys.stderr)
        sys.exit(1)

    src:Path = bitloop_root / "framework" / "templates" / "default"
    dst:Path = Path.cwd() / name

    print(f"src = {src} dst = {dst}")

    print(f"Creating new project '{name}' at {dst}")

    if not src.is_dir():
        raise SystemExit(f"Template not found: {src}")
    if dst.exists():
        raise SystemExit(f"Destination already exists: {dst}")

    copy_template_files(src, dst)

    # Replace only in .cpp and .h files
    for root, dirs, files in os.walk(dst):
        for fname in files:
            fpath = Path(root) / fname
            replace_in_file(fpath, PLACEHOLDER_IN_FILE, name)

    # Rename only SIM_NAME.cpp / SIM_NAME.h
    for root, _, files in os.walk(dst):
        for fname in files:
            fpath = Path(root) / fname
            rename_file_if_needed(fpath, name)

    # Rename folders called SIM_NAME (deepest first so paths don't break)
    dir_paths = []
    for root, dirs, _ in os.walk(dst):
        for d in dirs:
            dir_paths.append(Path(root) / d)
    dir_paths.sort(key=lambda p: len(p.parts), reverse=True)
    for dpath in dir_paths:
        if dpath.exists():
            rename_folder_if_needed(dpath, name)