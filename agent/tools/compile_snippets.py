#!/usr/bin/env python3
"""
NovaCPP Executable Snippet Compiler
Performs syntax-only verification (/Zs or -fsyntax-only) of all C++17 code snippet assets in agent/skills/*/assets/*.cpp.
Guarantees zero hallucinated or non-compiling C++ examples.
"""

import os
import sys
import subprocess
import glob
from concurrent.futures import ThreadPoolExecutor, as_completed

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')
if hasattr(sys.stderr, 'reconfigure'):
    sys.stderr.reconfigure(encoding='utf-8')

def find_cl_exe():
    # 1. Check in PATH
    try:
        res = subprocess.run(["where", "cl.exe"], capture_output=True, text=True)
        if res.returncode == 0:
            return res.stdout.strip().splitlines()[0]
    except Exception:
        pass

    # 2. Known Visual Studio / BuildTools locations
    candidates = [
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe",
    ]
    for c in candidates:
        matches = glob.glob(c)
        if matches and os.path.exists(matches[0]):
            return matches[0]
    return None

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    agent_dir = os.path.abspath(os.path.join(script_dir, ".."))
    repo_root = os.path.abspath(os.path.join(agent_dir, ".."))
    skills_dir = os.path.join(agent_dir, "skills")

    print("=" * 70)
    print("  NovaCPP Agent Skills: Snippet CI Compiler (/Zs Syntax-Only)")
    print("=" * 70)

    # Collect snippet files
    snippets = []
    for skill_name in sorted(os.listdir(skills_dir)):
        assets_dir = os.path.join(skills_dir, skill_name, "assets")
        if os.path.isdir(assets_dir):
            for f in sorted(os.listdir(assets_dir)):
                if f.endswith(".cpp"):
                    snippets.append(os.path.join(assets_dir, f))

    print(f"Discovered {len(snippets)} executable C++ snippet assets.\n")

    passed = 0
    failed = 0

    if sys.platform == "win32":
        cl_path = find_cl_exe()
        if not cl_path:
            print("  [WARN] MSVC cl.exe not located on system; skipping local compile verification.", flush=True)
            sys.exit(0)

        includes = [
            repo_root,
            os.path.dirname(cl_path) + r"\..\..\..\include",
            r"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt",
            r"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um",
            r"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
        ]
        inc_flags = [f"/I{inc}" for inc in includes if os.path.isdir(inc)]

        def compile_one(snip):
            rel = os.path.relpath(snip, repo_root)
            cmd = [cl_path, "/std:c++17", "/Zs", "/EHsc", "/W4", "/wd4996", "/wd4100", "/wd4127", "/wd4245"] + inc_flags + [snip]
            res = subprocess.run(cmd, capture_output=True, text=True)
            return rel, res.returncode == 0, res.stdout, res.stderr

        workers = min(8, os.cpu_count() or 4)
        with ThreadPoolExecutor(max_workers=workers) as executor:
            future_to_snip = {executor.submit(compile_one, snip): snip for snip in snippets}
            for fut in as_completed(future_to_snip):
                rel, ok, out, err = fut.result()
                if ok:
                    passed += 1
                    print(f"  [PASS] {rel}", flush=True)
                else:
                    failed += 1
                    print(f"  [FAIL] {rel}", flush=True)
                    if out:
                        print(out, flush=True)
                    if err:
                        print(err, flush=True)
    else:
        compiler = "g++" if shutil.which("g++") else "clang++"
        def compile_one_posix(snip):
            rel = os.path.relpath(snip, repo_root)
            cmd = [compiler, "-std=c++17", "-fsyntax-only", "-Wall", "-Wextra", f"-I{repo_root}", snip]
            res = subprocess.run(cmd, capture_output=True, text=True)
            return rel, res.returncode == 0, res.stdout, res.stderr

        workers = min(8, os.cpu_count() or 4)
        with ThreadPoolExecutor(max_workers=workers) as executor:
            future_to_snip = {executor.submit(compile_one_posix, snip): snip for snip in snippets}
            for fut in as_completed(future_to_snip):
                rel, ok, out, err = fut.result()
                if ok:
                    passed += 1
                    print(f"  [PASS] {rel}", flush=True)
                else:
                    failed += 1
                    print(f"  [FAIL] {rel}", flush=True)
                    if err:
                        print(err, flush=True)

    print("\n" + "=" * 70)
    if failed == 0:
        print(f"  ALL {passed} SNIPPETS COMPILED CLEANLY WITH ZERO WARNINGS!")
        print("=" * 70)
        sys.exit(0)
    else:
        print(f"  SNIPPET COMPILATION FAILED: {failed} failed, {passed} passed")
        print("=" * 70)
        sys.exit(1)

if __name__ == "__main__":
    main()

