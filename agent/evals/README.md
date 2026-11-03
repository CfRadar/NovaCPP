# NovaCPP Agent Skills Evaluation Framework

> **Evaluation Standard**: Automated regression tests for AI agent skills. Evaluations measure whether an AI agent with access to NovaCPP skills produces working, compilable code compared to baseline performance without skills.

---

## Evaluation Architecture

Each evaluation case is defined in a YAML file (`agent/evals/<skill-name>.yaml`):

```yaml
skill: nova-routing-and-handlers
triggers:
  should:
    - "add a /health endpoint that returns JSON"
    - "create a POST route that accepts a form"
    - "register a dynamic route for /users/:id"
  should_not:
    - "make the navbar blue"
    - "store the user theme in Redis"
    - "connect to postgresql database"

tasks:
  - id: health-endpoint
    prompt: "Add a GET /health route returning JSON {\"status\":\"ok\"}"
    fixture: fixtures/minimal-app
    checks:
      - type: build
        command: "cmake --build build"
      - type: http
        request: "GET /health"
        expect_status: 200
        expect_json: { status: "ok" }
      - type: forbidden_pattern
        pattern: "sprintf\\(|strcat\\("
    budgets:
      max_turns: 8
```

---

## 3 Evaluation Types

1. **Trigger Evaluations**: Verify that the skill's description triggers on relevant developer requests and stays idle for irrelevant ones.
2. **Outcome Evaluations**: Verify that the generated code builds cleanly, starts the server, passes HTTP assertions, and adheres to security constraints.
3. **Delta Evaluations**: Track the delta in turn count, token consumption, and success rate between runs *with* skills versus *without* skills.
