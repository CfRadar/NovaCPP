# NovaCPP Agent Skills Tooling

This directory contains the Python automation tools powering the NovaCPP Agent Skills subsystem:

- `build.py`: Multi-agent adapter generator emitting artifacts into `agent/dist/`.
- `validate.py`: Static validation suite checking frontmatter, link integrity, and real C++ symbol presence.
- `compile_snippets.py`: CI compilation test ensuring every `assets/*.cpp` compiles with the C++17 compiler.
