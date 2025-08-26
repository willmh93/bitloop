#!/usr/bin/env python3
import sys
import argparse
import platform
import os
import subprocess
from pathlib import Path

script_dir = Path(__file__).resolve().parent
sys.path.insert(0, str(script_dir))

from instantiate_template import instantiate_template
from bootstrap            import run_bootstrap

def main():
    parser = argparse.ArgumentParser(prog="bitloop")

    # "[bitloop] ..." parser
    subparsers = parser.add_subparsers(dest="command", required=True)

    # "bitloop [...]" subparsers
    new_parser			= subparsers.add_parser("new",			help="scaffold something")
    bootstrap_parser	= subparsers.add_parser("bootstrap",	help="install bootstrap dependencies (Linux only)")
    vcpkg_parser        = subparsers.add_parser("vcpkg",        help="invoke bundled vcpkg")

    vcpkg_parser.add_argument("vcpkg_args", nargs=argparse.REMAINDER,
                          help="arguments to pass through to vcpkg")

    # bitloop new [...] parser
    new_sub = new_parser.add_subparsers(dest="entity", required=True)
    new_sub.add_parser("project", help="scaffold a new project")
    new_sub.add_parser("header", help="add a header (.h)")
    new_sub.add_parser("class", help="add a class (.h  + .cpp) ")


    args = parser.parse_args()

    if args.command == "bootstrap":
        run_bootstrap()
        return

    if args.command == "vcpkg":
        bitloop_root = script_dir.parent
        exe_name = "vcpkg.exe" if os.name == "nt" else "vcpkg"
        vcpkg_exe = bitloop_root / "vcpkg" / exe_name

        forward = list(args.vcpkg_args or [])
        if forward and forward[0] == "--":
            forward = forward[1:]

        cmd = [str(vcpkg_exe)] + (forward if forward else ["--help"])

        rc = subprocess.call(cmd, cwd=str(vcpkg_exe.parent))
        sys.exit(rc)
        return

    if args.command == "new":
        new_type = getattr(args, "entity", None)
        if new_type == "project":
            bitloop_root = script_dir.parent
            instantiate_template(bitloop_root=bitloop_root, type="project")
            return
        elif new_type == "header":
            bitloop_root = script_dir.parent
            instantiate_template(bitloop_root=bitloop_root, type="header")
            return
        elif new_type == "class":
            bitloop_root = script_dir.parent
            instantiate_template(bitloop_root=bitloop_root, type="class")
            return


if __name__ == "__main__":
    main()
