# NovaCPP Canonical Agent Skills Directory

> **Source of Truth Notice**: This directory (`NovaCPP/agent/skills/`) is the single canonical source of truth for all NovaCPP agent skills. Humans edit skills exclusively here. All target-specific adapters in `agent/dist/` (Claude Code, Google Antigravity, Cursor, OpenAI Codex, GitHub Copilot, Windsurf) are automatically compiled by `agent/tools/build.py`.

---

## 1. Directory Layout per Skill

Each skill is a self-contained directory whose name matches the skill name:

```text
nova-<topic>/
├── SKILL.md              # Mandatory playbook (< 5,000 tokens, < 500 lines)
├── references/           # Optional: deep API docs, specs, loaded on explicit demand
│   └── api.md
├── assets/               # Optional: compilable C++17 templates (verified in CI)
│   └── example.cpp
└── scripts/              # Optional: non-destructive helper scripts
    └── check.py
```

---

## 2. YAML Frontmatter Specification

Every `SKILL.md` must start with valid YAML frontmatter:

```yaml
---
name: nova-<topic>
description: >-
  Concise summary of what the skill does in one clause. Use when the user asks
  to <trigger verbs and nouns in developer's words> in a NovaCPP project.
  Not for <sibling concern> (see nova-<sibling>).
license: MIT
compatibility: Requires a NovaCPP project and a C++17 compiler (MSVC 2022, GCC 11+, Clang 13+).
metadata:
  nova-version: ">=2.1.0"
  author: novacpp-core-team
  version: "1.0.0"
---
```

### Invariants:
1. `name`: 1-64 characters, lowercase alphanumeric and single hyphens (`^[a-z0-9]+(-[a-z0-9]+)*$`). Must match folder name exactly.
2. `description`: 1-1,024 characters. Must include "Nova" and a "Use when" clause.
3. `metadata`: Store framework version and author information under `metadata:` to prevent parser errors in third-party agents.

---

## 3. Mandatory Structure of `SKILL.md`

1. `# <Skill Title>`
2. `## When to use`: 1-2 sentence trigger summary.
3. `## Prerequisites`: What must already exist before following this skill.
4. `## Quick start`: Minimal self-contained C++17 snippet (under 30 lines).
5. `## Steps`: Numbered, concrete procedural steps.
6. `## Rules`: Imperative `MUST` and `NEVER` constraints.
7. `## Common mistakes`: Diagnostic table with Symptom -> Cause -> Fix.
8. `## Verify`: Exact terminal commands with expected results.
9. `## Go deeper`: Links to `references/*.md` files.

---

## 4. Symbol Verification Convention

To prevent hallucinated APIs, skills should annotate required symbols:
```html
<!-- symbols: NovaBuilder, JsonResponse, State, fetch -->
```
The automated validator (`agent/tools/validate.py`) reads this comment and verifies that all symbols exist in `novacpp/*.hpp`.

