---
name: review-interactive
description: Review code changes in the KPHP compiler and runtime codebase interactively
---

# Workflow

## 1. Scope Definition

When invoked, ask the user:

1. **What to review:**
   - Branch vs branch (e.g., `feature-branch` against `master`)
   - Commit vs commit/branch (e.g., `abc123` against `HEAD`)
   - Specific files or directories

2. **Review mode:** Is this your own PR (self-review) or someone else's PR?
   - **Self-review:** I will offer to apply fixes as we find them
   - **Someone else's PR:** I will collect all findings and offer to export/push to GitHub at the end

Collect the diff using `git diff` with function context (`--function-context`).

## 2. Context Expansion

During review, if a code chunk lacks sufficient context to evaluate properly, ask the user for permission to read the full file. Examples of when context expansion is needed:
- Understanding class invariants or preconditions
- Checking function signatures not shown in the diff
- Verifying include/dependency relationships
- Understanding the full implementation of modified functions
- Checking for existing patterns that should be followed

Ask the user: "I need to see more context in `file.cpp` to properly evaluate this change. May I read the full file?"

Proceed based on user response:
- If **yes**: Read the file and continue review with expanded context
- If **no**: Note the limitation and continue with available context, or skip the ambiguous section

## 3. Verification with clangd

Use `clangd-18` to verify code correctness:
- Run from project root: `clangd-18 --check=<path/to/file.cpp>`
- This picks up `.clangd` config automatically (which includes the compilation database location)
- Use to verify: includes are correct, standard library features are available, missing dependencies
- When unsure about C++ includes or standard library usage, check with clangd before commenting

## 4. Review Loop

For each logical chunk of changes:

1. **Analyze** the code against the rules below
   - **Important:** Only review code that was actually modified by the author (present in the diff)
   - Do not flag issues in pre-existing, untouched code unless it directly relates to the changes being made
   - If you need to reference unchanged code for context, clearly distinguish it from the actual changes
2. **Present** findings wrapped in `<findings>` tags with:
   - File location and line numbers
   - Severity: `suggestion`, `nit`, or `issue`
   - Clear explanation of the concern
   - Proposed alternative when applicable
3. **Ask** the user for feedback on your thoughts
4. **Adapt** based on user responses (skip similar issues if asked, dive deeper if requested)

For large changes, feel free to use light humor occasionally to keep the review relaxed and enjoyable for the reviewer.

## 5. External PR Review Mode

When reviewing someone else's PR (as determined in Step 1):

1. **Collect all findings** during the review without offering to apply fixes
2. Present findings as structured comments for later export
3. **At completion**, offer to:
   - **Export to file**: Create a markdown file with all findings
   - **Push to GitHub**: Use `gh` CLI to post review comments to the PR:
     ```bash
     # For each finding, create a review comment
     gh pr review <PR-number> --comment --body "comment text" -- <file>

     # Or create a single review with all comments
     gh pr review <PR-number> --request-changes --body-file review-comments.md
     ```

## 6. Completion

Summarize all findings and ask based on review mode:

**For self-review:**
- Apply any automated fixes
- Continue reviewing other changes

**For external PR review:**
- Export findings to a file
- Push review comments to GitHub PR

---

# Review Rules

## General Principles

Act like a senior developer who prioritizes:

| Priority | Principle |
|----------|-----------|
| 1 | **Readability** - Clear naming, early returns, explicit error handling, meaningful comments |
| 2 | **Standard mechanisms** - Prefer C++ standard library over custom solutions |
| 3 | **Reusability** - Non-PHP-specific code should be written in a PHP-agnostic way |
| 4 | **C++ best practices** - RAII, proper include hygiene (no transitive/missing includes), namespaces, fully qualified identifiers |
| 5 | **Consistency** - Follow existing patterns in the codebase; note when a diff introduces pattern changes |

## Component-Specific Rules

### Compiler (`/compiler/`)

- C++17 standard
- Follow existing AST transformation patterns

### All Runtimes (`/runtime-common/`, `/runtime/`, `/runtime-light/`)

- All functions should be marked `noexcept` without any considerations
- **PHP function naming**: All PHP standard library functions must use the `f$` prefix (e.g., `f$array_merge`)

### Common Runtime (`/runtime-common/`)

- C++17 standard
- **Thread-safe**: All code must be safe for concurrent execution
- **No resource ownership**: Code must not directly own system resources (files, sockets, malloc memory)

### Legacy Runtime (`/runtime/`)

- C++17 standard

### K2 Runtime (`/runtime-light/`)

- C++23 standard
- **Thread-safe**: All code must be safe for concurrent execution
- **No resource ownership**: Code must not directly own system resources; request them through the `k2::` interface instead
- **Fork-aware coroutines**: When calling a coroutine, wrap the call with `kphp::forks::id_managed` unless the coroutine is a KPHP standard library function (prefixed with `f$`), which already handles this internally
- **Braced initialization**: Always use braced initialization (e.g., `T obj{};`) unless parentheses are semantically required (e.g., `std::vector<int> v(10, 0);` for size/value construction)
- **Log messages**: Log messages should start with a lowercase letter
- **Type naming**: Types should begin with a lowercase letter (e.g., `task`, `component`, `buffer`). Exception: `*State` types (e.g., `InstanceState`, `ConfdataImageState`, `RpcServerInstanceState`) are currently permitted (this exception is temporary)
- **KPHP types in ImageState/ComponentState**: KPHP types (`array`, `string`, `mixed`, `class_instance`) that are members of `*ImageState` or `*ComponentState` types must set their reference counter to `ExtraRefCnt::ForGlobalConst` to prevent data races. Prefer `kphp::core::set_reference_counter_recursive` over `obj.set_reference_counter_to`

---

# Output Format

Use this structure for findings:

```
<findings>
### File: `path/to/file.cpp` (lines 45-52)
**Severity:** suggestion

**Issue:** Brief description of the concern

**Rationale:** Why this matters (referencing specific rules above)

**Suggestion:** Concrete alternative or fix
</findings>
```

Severity levels:
- `issue` - Likely bug or violation of hard requirements (thread-safety, resource ownership)
- `suggestion` - Improvement to readability, maintainability, or adherence to best practices
- `nit` - Minor stylistic preference, optional to address
