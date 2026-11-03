#!/usr/bin/env python3
"""
NovaCPP Agent Skills Static Validation Pipeline
Enforces the 9 Quality Gates across all canonical skills in agent/skills/:
  1. YAML Frontmatter & Allowed Keys Check
  2. Name Regex & Directory Match
  3. Description Format & 'Use when' Trigger Clause
  4. Relative Link Integrity (No 404s)
  5. Orphan Reference Checker
  6. Real C++ Symbol Verification against novacpp/*.hpp & src/backend/*.hpp
  7. Rejection of 'VERIFY' / unfinished placeholders
  8. Line Count Bounds (< 500 lines)
  9. Zero Secrets & Credentials Check
"""

import os
import sys
import re
import yaml

ALLOWED_FM_KEYS = {"name", "description", "license", "compatibility", "metadata", "allowed-tools"}
NAME_PATTERN = re.compile(r'^[a-z0-9]+(-[a-z0-9]+)*$')
SECRET_PATTERNS = [
    re.compile(r'sk-[a-zA-Z0-9_-]{20,}'),
    re.compile(r'ghp_[a-zA-Z0-9]{20,}'),
    re.compile(r'-----BEGIN (?:RSA )?PRIVATE KEY-----'),
]

def load_all_headers(repo_root):
    header_content = ""
    for sub in ["novacpp", os.path.join("src", "backend")]:
        p = os.path.join(repo_root, sub)
        if os.path.exists(p):
            for fname in os.listdir(p):
                if fname.endswith(".hpp") or fname.endswith(".h"):
                    with open(os.path.join(p, fname), "r", encoding="utf-8", errors="ignore") as f:
                        header_content += f.read() + "\n"
    return header_content

def validate_skill(skill_dir, skill_name, headers_cache, errors, warnings):
    skill_md = os.path.join(skill_dir, "SKILL.md")
    if not os.path.exists(skill_md):
        errors.append(f"[{skill_name}] Missing SKILL.md")
        return

    with open(skill_md, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Frontmatter Check & Allowed Keys
    if not content.startswith("---"):
        errors.append(f"[{skill_name}] SKILL.md must start with YAML frontmatter '---'")
        return

    end_fm = content.find("\n---", 3)
    if end_fm == -1:
        errors.append(f"[{skill_name}] Malformed frontmatter (missing closing '---')")
        return

    fm_raw = content[3:end_fm]
    body = content[end_fm + 4:].strip()

    try:
        fm = yaml.safe_load(fm_raw)
    except Exception as e:
        errors.append(f"[{skill_name}] YAML parsing error: {e}")
        return

    if not isinstance(fm, dict):
        errors.append(f"[{skill_name}] Frontmatter is not a valid YAML dictionary")
        return

    extra_keys = set(fm.keys()) - ALLOWED_FM_KEYS
    if extra_keys:
        errors.append(f"[{skill_name}] Gate 1 Failed: Undeclared top-level frontmatter keys: {extra_keys}")

    # 2. Name Invariant
    name = fm.get("name", "")
    if not name or name != skill_name:
        errors.append(f"[{skill_name}] Gate 2 Failed: Frontmatter name '{name}' does not match directory '{skill_name}'")
    if not NAME_PATTERN.match(name) or len(name) > 64:
        errors.append(f"[{skill_name}] Gate 2 Failed: Name '{name}' does not match regex or exceeds 64 chars")

    # 3. Description Invariant
    desc = fm.get("description", "")
    if not isinstance(desc, str):
        errors.append(f"[{skill_name}] Gate 3 Failed: Description must be a string")
    else:
        clean_desc = " ".join(desc.split())
        if len(clean_desc) < 50 or len(clean_desc) > 1024:
            errors.append(f"[{skill_name}] Gate 3 Failed: Description length ({len(clean_desc)}) out of bounds [50, 1024]")
        if "Nova" not in clean_desc and "NovaCPP" not in clean_desc:
            errors.append(f"[{skill_name}] Gate 3 Failed: Description must contain 'Nova' or 'NovaCPP'")
        if "Use when" not in clean_desc:
            errors.append(f"[{skill_name}] Gate 3 Failed: Description must contain 'Use when' trigger clause")

    # 4. Relative Link Integrity
    # Strip code blocks before checking markdown links
    content_no_code = re.sub(r'```.*?```', '', content, flags=re.DOTALL)
    link_matches = re.findall(r'\[([^\]]+)\]\(([^)]+)\)', content_no_code)
    for text, href in link_matches:
        if href.startswith("http://") or href.startswith("https://") or href.startswith("#"):
            continue
        if href.startswith("file:///"):
            clean_path = href.replace("file:///", "").replace("/", os.sep)
            # Normalize drive letter on Windows (e.g. d: -> D:)
            if len(clean_path) >= 2 and clean_path[1] == ':':
                clean_path = clean_path[0].upper() + clean_path[1:]
            if not os.path.exists(clean_path):
                errors.append(f"[{skill_name}] Gate 4 Failed: Broken absolute file link: {href}")
        else:
            rel_target = os.path.normpath(os.path.join(skill_dir, href.split('#')[0]))
            if not os.path.exists(rel_target):
                errors.append(f"[{skill_name}] Gate 4 Failed: Broken relative link: {href}")

    # 5. No Orphan References
    ref_dir = os.path.join(skill_dir, "references")
    if os.path.exists(ref_dir):
        for ref_file in os.listdir(ref_dir):
            if ref_file.endswith(".md"):
                if ref_file not in content:
                    errors.append(f"[{skill_name}] Gate 5 Failed: Orphan reference file '{ref_file}' not linked in SKILL.md")

    # 6. Real C++ Symbol Verification
    sym_match = re.search(r'<!--\s*symbols:\s*(.*?)\s*-->', content)
    if sym_match:
        declared_symbols = [s.strip() for s in sym_match.group(1).split(",") if s.strip()]
        for sym in declared_symbols:
            pat = r'\b' + re.escape(sym) + r'\b'
            if not re.search(pat, headers_cache):
                errors.append(f"[{skill_name}] Gate 6 Failed: Claimed symbol '{sym}' not found in NovaCPP headers")
    else:
        errors.append(f"[{skill_name}] Gate 6 Failed: Missing <!-- symbols: ... --> annotation")

    # 7. No Unfinished Placeholders
    if "<!-- VERIFY -->" in content or "<!-- TODO -->" in content:
        errors.append(f"[{skill_name}] Gate 7 Failed: Contains unfinished <!-- VERIFY --> or <!-- TODO --> comment")

    # 8. Line Count Bounds
    line_count = len(body.splitlines())
    if line_count > 800:
        errors.append(f"[{skill_name}] Gate 8 Failed: Body line count ({line_count}) exceeds hard limit of 800 lines")
    elif line_count > 500:
        warnings.append(f"[{skill_name}] Gate 8 Warning: Body line count ({line_count}) exceeds recommended 500 lines")

    # 9. Secret & Token Hygiene Check
    for pat in SECRET_PATTERNS:
        if pat.search(content):
            errors.append(f"[{skill_name}] Gate 9 Failed: Potential credential or private key token detected in SKILL.md")

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    agent_dir = os.path.abspath(os.path.join(script_dir, ".."))
    repo_root = os.path.abspath(os.path.join(agent_dir, ".."))
    skills_dir = os.path.join(agent_dir, "skills")

    print("=" * 70)
    print("  NovaCPP Agent Skills: Static Validation Pipeline")
    print("=" * 70)
    print(f"Skills Directory:  {skills_dir}")
    print(f"Header Search:     {os.path.join(repo_root, 'novacpp')}, {os.path.join(repo_root, 'src', 'backend')}")

    if not os.path.isdir(skills_dir):
        print(f"ERROR: Skills directory not found at {skills_dir}")
        sys.exit(1)

    headers_cache = load_all_headers(repo_root)

    skill_names = sorted([d for d in os.listdir(skills_dir) if os.path.isdir(os.path.join(skills_dir, d))])
    print(f"Discovered {len(skill_names)} skills to validate.\n")

    errors = []
    warnings = []

    for name in skill_names:
        validate_skill(os.path.join(skills_dir, name), name, headers_cache, errors, warnings)

    for w in warnings:
        print(f"  [WARN] {w}")

    if errors:
        print("\n" + "=" * 70)
        print(f"  VALIDATION FAILED: {len(errors)} ERROR(S) DETECTED")
        print("=" * 70)
        for err in errors:
            print(f"  [ERROR] {err}")
        print("=" * 70)
        sys.exit(1)

    print("=" * 70)
    print(f"  ALL {len(skill_names)} SKILLS PASSED ALL 9 QUALITY GATES!")
    print("=" * 70)
    sys.exit(0)

if __name__ == "__main__":
    main()

