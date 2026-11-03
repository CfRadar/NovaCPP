#!/usr/bin/env python3
"""
NovaCPP Multi-Agent Adapter Build Engine
Compiles canonical skills in agent/skills/ into native adapters for:
- Google Antigravity (.agent/, .agents/, .agent/rules/nova.md)
- Claude Code (.claude/, CLAUDE.md)
- Cursor (.cursor/rules/*.mdc)
- OpenAI Codex (AGENTS.md)
- GitHub Copilot (.github/copilot-instructions.md, .github/instructions/)
- Windsurf (.windsurf/rules/)
- Gemini CLI (GEMINI.md)
- Universal LLMs (nova-agent-context.md)
"""

import os
import sys
import shutil
import re
import yaml

def clean_dir(dir_path):
    if os.path.exists(dir_path):
        shutil.rmtree(dir_path)
    os.makedirs(dir_path, exist_ok=True)
    # Touch .gitkeep to maintain repository directory tracking
    with open(os.path.join(dir_path, ".gitkeep"), 'w', encoding='utf-8') as f:
        pass

def copy_tree(src, dst):
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst)

def parse_skill(skill_md_path):
    with open(skill_md_path, 'r', encoding='utf-8') as f:
        content = f.read()

    if not content.startswith('---'):
        raise ValueError(f"Missing frontmatter in {skill_md_path}")

    end_fm = content.find('\n---', 3)
    if end_fm == -1:
        raise ValueError(f"Malformed frontmatter in {skill_md_path}")

    fm_raw = content[3:end_fm]
    body = content[end_fm + 4:].strip()

    meta = yaml.safe_load(fm_raw)
    return meta, body, content

def build_skill_index(skills):
    # Tier ordering
    tier_order = [
        "nova-getting-started",
        "nova-project-structure",
        "nova-build-and-debug",
        "nova-routing-and-handlers",
        "nova-ui-rendering",
        "nova-browser-client",
        "nova-http-client",
        "nova-database",
        "nova-sessions-state",
        "nova-auth",
        "nova-background-work",
        "nova-server-config",
        "nova-state-graph",
        "nova-checkpoints",
        "nova-streaming",
        "nova-telemetry",
        "nova-mcp-and-tools",
        "nova-vector-memory",
        "nova-recipe-crud-page",
        "nova-recipe-auth-flow",
        "nova-recipe-ai-chat",
        "nova-recipe-deploy"
    ]

    sorted_skills = sorted(skills, key=lambda s: tier_order.index(s['name']) if s['name'] in tier_order else 999)

    lines = [
        "| Skill | Trigger / Use When | Canonical Path |",
        "|---|---|---|"
    ]

    for s in sorted_skills:
        name = s['name']
        desc = s['meta'].get('description', '')
        # Extract concise "Use when" or first sentence
        if "Use when" in desc:
            use_when = desc[desc.find("Use when"):].strip()
        else:
            use_when = desc.split('.')[0].strip()
        
        # Clean up whitespace
        use_when = " ".join(use_when.split())
        path = f"agent/skills/{name}/SKILL.md"
        lines.append(f"| [`{name}`]({path}) | {use_when} | `{path}` |")

    return "\n".join(lines)

def rewrite_cursor_links(skill_name, content):
    # Strip file:/// links to relative agent path
    content = re.sub(
        r'file:///[a-zA-Z]:/[^)]+/agent/skills/([a-zA-Z0-9_\-]+)/references/([a-zA-Z0-9_\-]+\.md)',
        r'nova-agent/skills/\1/references/\2',
        content
    )
    content = re.sub(
        r'file:///[a-zA-Z]:/[^)]+/agent/skills/([a-zA-Z0-9_\-]+)/assets/([a-zA-Z0-9_\-]+\.cpp)',
        r'nova-agent/skills/\1/assets/\2',
        content
    )
    content = re.sub(
        r'file:///[a-zA-Z]:/[^)]+/agent/skills/([a-zA-Z0-9_\-]+)/SKILL\.md',
        r'nova-agent/skills/\1/SKILL.md',
        content
    )
    # Rewrite local relative links: references/... -> nova-agent/skills/<name>/references/...
    content = re.sub(
        r'\]\(references/([a-zA-Z0-9_\-]+\.md)\)',
        rf'](nova-agent/skills/{skill_name}/references/\1)',
        content
    )
    # Rewrite local relative links: assets/... -> nova-agent/skills/<name>/assets/...
    content = re.sub(
        r'\]\(assets/([a-zA-Z0-9_\-]+\.cpp)\)',
        rf'](nova-agent/skills/{skill_name}/assets/\1)',
        content
    )
    return content

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    agent_dir = os.path.abspath(os.path.join(script_dir, ".."))
    skills_dir = os.path.join(agent_dir, "skills")
    router_file = os.path.join(agent_dir, "router.md")
    dist_dir = os.path.join(agent_dir, "dist")

    print(f"NovaCPP Agent Skills Build Engine")
    print(f"Source Directory: {skills_dir}")
    print(f"Target Dist:      {dist_dir}")

    clean_dir(dist_dir)

    # 1. Collect and parse skills
    skill_folders = [d for d in os.listdir(skills_dir) if os.path.isdir(os.path.join(skills_dir, d))]
    skills = []

    for name in sorted(skill_folders):
        skill_md = os.path.join(skills_dir, name, "SKILL.md")
        if not os.path.exists(skill_md):
            continue
        meta, body, full = parse_skill(skill_md)
        skills.append({
            'name': name,
            'path': os.path.join(skills_dir, name),
            'meta': meta,
            'body': body,
            'full': full
        })

    print(f"Loaded {len(skills)} canonical skills.")

    # 2. Build skill index table
    skill_index_table = build_skill_index(skills)

    # 3. Read and populate router template
    with open(router_file, 'r', encoding='utf-8') as f:
        router_template = f.read()

    agents_md = router_template.replace("{{SKILL_INDEX}}", skill_index_table)
    agents_md_bytes = len(agents_md.encode('utf-8'))
    print(f"Generated AGENTS.md: {agents_md_bytes} bytes (Budget: < 24576 bytes / 24 KB)")

    if agents_md_bytes >= 24 * 1024:
        raise ValueError(f"AGENTS.md exceeds 24 KB budget: {agents_md_bytes} bytes!")

    # 4. Emit target: dist/root (AGENTS.md & GEMINI.md)
    root_dist = os.path.join(dist_dir, "root")
    os.makedirs(root_dist, exist_ok=True)

    with open(os.path.join(root_dist, "AGENTS.md"), 'w', encoding='utf-8') as f:
        f.write(agents_md)

    gemini_md = (
        "# Gemini CLI Agent Context for NovaCPP\n\n"
        "@./AGENTS.md\n\n"
        "Follow the non-negotiable hard rules, lifetime safety invariants, and canonical skill workflows defined above.\n"
    )
    with open(os.path.join(root_dist, "GEMINI.md"), 'w', encoding='utf-8') as f:
        f.write(gemini_md)

    # 5. Emit target: dist/claude (.claude/skills/ & CLAUDE.md)
    claude_dist = os.path.join(dist_dir, "claude")
    claude_skills = os.path.join(claude_dist, ".claude", "skills")
    os.makedirs(claude_skills, exist_ok=True)

    for s in skills:
        dst = os.path.join(claude_skills, s['name'])
        copy_tree(s['path'], dst)

    with open(os.path.join(claude_dist, "CLAUDE.md"), 'w', encoding='utf-8') as f:
        f.write("@AGENTS.md\n\nAlways follow the skills defined in `.claude/skills/` and hard architectural rules.\n")

    # 6. Emit target: dist/antigravity (.agent/skills/, .agents/skills/, .agent/rules/nova.md)
    antigravity_dist = os.path.join(dist_dir, "antigravity")
    ag_skills = os.path.join(antigravity_dist, ".agent", "skills")
    ag_rules = os.path.join(antigravity_dist, ".agent", "rules")
    os.makedirs(ag_skills, exist_ok=True)
    os.makedirs(ag_rules, exist_ok=True)

    for s in skills:
        copy_tree(s['path'], os.path.join(ag_skills, s['name']))

    # Write .agent/rules/nova.md
    with open(os.path.join(ag_rules, "nova.md"), 'w', encoding='utf-8') as f:
        f.write(agents_md)

    # Also dist/agents (.agents/skills/)
    agents_dist = os.path.join(dist_dir, "agents")
    dot_agents_skills = os.path.join(agents_dist, ".agents", "skills")
    os.makedirs(dot_agents_skills, exist_ok=True)
    for s in skills:
        copy_tree(s['path'], os.path.join(dot_agents_skills, s['name']))

    # 7. Emit target: dist/cursor (.cursor/rules/*.mdc)
    cursor_dist = os.path.join(dist_dir, "cursor")
    cursor_rules = os.path.join(cursor_dist, ".cursor", "rules")
    os.makedirs(cursor_rules, exist_ok=True)

    # 00-nova-router.mdc
    with open(os.path.join(cursor_rules, "00-nova-router.mdc"), 'w', encoding='utf-8') as f:
        f.write("---\n")
        f.write("description: \"NovaCPP Core Architecture and Agent Router\"\n")
        f.write("globs: \"**/*\"\n")
        f.write("alwaysApply: true\n")
        f.write("---\n\n")
        f.write(agents_md)

    # Individual rule files for each skill
    for s in skills:
        rewritten_body = rewrite_cursor_links(s['name'], s['body'])
        rule_desc = s['meta'].get('description', '').replace('\n', ' ').strip()
        # Escape quotes in description
        rule_desc_escaped = rule_desc.replace('"', '\\"')

        rule_path = os.path.join(cursor_rules, f"{s['name']}.mdc")
        with open(rule_path, 'w', encoding='utf-8') as f:
            f.write("---\n")
            f.write(f"description: \"{rule_desc_escaped}\"\n")
            f.write("globs: \"**/*\"\n")
            f.write("alwaysApply: false\n")
            f.write("---\n\n")
            f.write(rewritten_body)
            f.write("\n")

    # 8. Emit target: dist/copilot (.github/copilot-instructions.md & .github/instructions/)
    copilot_dist = os.path.join(dist_dir, "copilot")
    copilot_gh = os.path.join(copilot_dist, ".github")
    copilot_instr = os.path.join(copilot_gh, "instructions")
    os.makedirs(copilot_instr, exist_ok=True)

    with open(os.path.join(copilot_gh, "copilot-instructions.md"), 'w', encoding='utf-8') as f:
        f.write(agents_md)

    for s in skills:
        instr_path = os.path.join(copilot_instr, f"{s['name']}.instructions.md")
        with open(instr_path, 'w', encoding='utf-8') as f:
            f.write(f"---\napplyTo: \"**/*.cpp,**/*.hpp\"\n---\n\n")
            f.write(s['body'])
            f.write("\n")

    # 9. Emit target: dist/windsurf (.windsurf/rules/<name>.md)
    windsurf_dist = os.path.join(dist_dir, "windsurf")
    windsurf_rules = os.path.join(windsurf_dist, ".windsurf", "rules")
    os.makedirs(windsurf_rules, exist_ok=True)

    for s in skills:
        w_path = os.path.join(windsurf_rules, f"{s['name']}.md")
        with open(w_path, 'w', encoding='utf-8') as f:
            f.write(f"# NovaCPP Rule: {s['name']}\n\n")
            f.write(s['body'])
            f.write("\n")

    # 10. Emit target: dist/generic (nova-agent-context.md)
    generic_dist = os.path.join(dist_dir, "generic")
    os.makedirs(generic_dist, exist_ok=True)

    context_bundle_lines = [
        agents_md,
        "\n\n---\n\n# Canonical Skills Reference Catalog\n"
    ]

    for s in skills:
        context_bundle_lines.append(f"\n\n## Skill: {s['name']}\n")
        context_bundle_lines.append(f"> {s['meta'].get('description', '')}\n\n")
        # Include Quick Start and Rules from the skill body
        body = s['body']
        context_bundle_lines.append(body)

    with open(os.path.join(generic_dist, "nova-agent-context.md"), 'w', encoding='utf-8') as f:
        f.write("\n".join(context_bundle_lines))

    print(f"Build complete. All adapters successfully generated in {dist_dir}")

if __name__ == '__main__':
    main()
