import sys
import argparse
import os
import shutil
from pathlib import Path

#PLACEHOLDER_IN_FILE  = "{SIM_NAME}"
TEXT_EXTENSIONS      = {".cpp", ".h", ".txt", ".json"}
#RENAME_TARGETS       = {"SIM_NAME.cpp", "SIM_NAME.h"}
#RENAME_FOLDER_TARGET = "SIM_NAME"

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

def rename_file_if_needed(path:Path, project_name:str, TARGET:str, RENAME_TARGETS:set[str]) -> Path:
    if path.name not in RENAME_TARGETS:
        return path
    new_name = path.name.replace(TARGET, project_name)
    new_path = path.with_name(new_name)
    if new_path.exists():
        raise FileExistsError(f"Cannot rename '{path}' to '{new_path}': destination exists.")
    path.rename(new_path)
    return new_path

#def rename_folder_if_needed(path:Path, project_name:str, RENAME_FOLDER_TARGET:str) -> Path:
#    if path.name != RENAME_FOLDER_TARGET:
#        return path
#    new_name = project_name
#    new_path = path.with_name(new_name)
#    if new_path.exists():
#        raise FileExistsError(f"Cannot rename folder '{path}' to '{new_path}': destination exists.")
#    path.rename(new_path)
#    return new_path

def _on_rm_error(func, path, exc_info):
    # Make read-only paths writable, then retry
    try:
        os.chmod(path, stat.S_IWRITE)
    except Exception:
        pass
    func(path)

def _merge_move(src: Path, dst: Path) -> None:
    dst.mkdir(parents=True, exist_ok=True)
    for item in src.iterdir():
        d = dst / item.name
        if item.is_dir():
            # If a file is in the way, remove it
            if d.exists() and d.is_file():
                d.unlink()
            _merge_move(item, d)
        else:
            # If a dir is in the way, remove it
            if d.exists() and d.is_dir():
                shutil.rmtree(d, onerror=_on_rm_error)
            # Move (atomic on same volume)
            item.replace(d)
    # Remove now-empty src (rmdir if empty, else rmtree)
    try:
        src.rmdir()
    except OSError:
        shutil.rmtree(src, onerror=_on_rm_error)

def rename_folder_if_needed(path: Path, project_name: str, target: str) -> Path:
    if path.name != target:
        return path

    new_path = path.with_name(project_name)
    if new_path == path:
        return path  # nothing to do

    # Fast path: try plain rename when destination doesn't exist
    if not new_path.exists():
        try:
            path.rename(new_path)
            return new_path
        except (PermissionError, OSError):
            # Fall through to merge on failure
            pass

    # Destination exists or rename failed: merge contents into new_path
    _merge_move(path, new_path)
    return new_path

def copy_template_files(src: Path, dst: Path, *, merge: bool = False) -> None:
    if not merge:
        shutil.copytree(src, dst)  # requires dst not to exist
        return

    dst.mkdir(parents=True, exist_ok=True)
    for entry in src.iterdir():
        s = entry
        d = dst / entry.name
        if entry.is_dir():
            shutil.copytree(s, d, dirs_exist_ok=True)  # Python 3.8+
        else:
            d.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(s, d)

###############################################

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
            replace_in_file(fpath, "{SIM_NAME}", name)

    # Replace only in files with right extensions
    for root, _, files in os.walk(dst):
        for fname in files:
            fpath = Path(root) / fname
            rename_file_if_needed(fpath, name, "SIM_NAME", {"SIM_NAME.cpp", "SIM_NAME.h"})

    # Rename folders called SIM_NAME (deepest first so paths don't break)
    dir_paths = []
    for root, dirs, _ in os.walk(dst):
        for d in dirs:
            dir_paths.append(Path(root) / d)
    dir_paths.sort(key=lambda p: len(p.parts), reverse=True)
    for dpath in dir_paths:
        if dpath.exists():
            rename_folder_if_needed(dpath, name, "SIM_NAME")

def create_header(bitloop_root:Path):
    name = input("Enter header name: ").strip()
    if not name:
        print("Header name cannot be empty.", file=sys.stderr)
        sys.exit(1)

    src:Path = bitloop_root / "framework" / "templates" / "header"
    dst:Path = Path.cwd() 

    print(f"src = {src} dst = {dst}")
    print(f"Creating new header '{name}' at {dst}")

    if not src.is_dir():
        raise SystemExit(f"Template not found: {src}")

    copy_template_files(src, dst, merge=True)

    # Replace only in files with right extensions
    for root, dirs, files in os.walk(dst):
        for fname in files:
            fpath = Path(root) / fname
            replace_in_file(fpath, "{HEADER_NAME}", name)

    # Rename only HEADER_NAME.h
    for root, _, files in os.walk(dst):
        for fname in files:
            fpath = Path(root) / fname
            rename_file_if_needed(fpath, name, "HEADER_NAME", {"HEADER_NAME.h"})

    # Rename folders called HEADER_NAME (deepest first so paths don't break)
    dir_paths = []
    for root, dirs, _ in os.walk(dst):
        for d in dirs:
            dir_paths.append(Path(root) / d)
    dir_paths.sort(key=lambda p: len(p.parts), reverse=True)
    for dpath in dir_paths:
        if dpath.exists():
            rename_folder_if_needed(dpath, name, "HEADER_NAME")


def create_class(bitloop_root:Path):
    name = input("Enter class name: ").strip()
    if not name:
        print("Class name cannot be empty.", file=sys.stderr)
        sys.exit(1)

    src:Path = bitloop_root / "framework" / "templates" / "class"
    dst:Path = Path.cwd() 

    print(f"src = {src} dst = {dst}")
    print(f"Creating new header '{name}' at {dst}")

    if not src.is_dir():
        raise SystemExit(f"Template not found: {src}")

    copy_template_files(src, dst, merge=True)

    # Replace only in files with right extensions
    for root, dirs, files in os.walk(dst):
        for fname in files:
            fpath = Path(root) / fname
            replace_in_file(fpath, "{CLASS_NAME}", name)

    # Rename only CLASS_NAME.h
    for root, _, files in os.walk(dst):
        for fname in files:
            fpath = Path(root) / fname
            rename_file_if_needed(fpath, name, "CLASS_NAME", {"CLASS_NAME.h", "CLASS_NAME.cpp"})

    # Rename folders called CLASS_NAME (deepest first so paths don't break)
    dir_paths = []
    for root, dirs, _ in os.walk(dst):
        for d in dirs:
            dir_paths.append(Path(root) / d)
    dir_paths.sort(key=lambda p: len(p.parts), reverse=True)
    for dpath in dir_paths:
        if dpath.exists():
            rename_folder_if_needed(dpath, name, "CLASS_NAME")

###############################################

def instantiate_template(bitloop_root:Path, type:str):
    if type == "project":
        create_project(bitloop_root)
    elif type == "header":
        create_header(bitloop_root)
    elif type == "class":
        create_class(bitloop_root)
    else:
        raise SystemExit(f"Unknown template type: {type}")