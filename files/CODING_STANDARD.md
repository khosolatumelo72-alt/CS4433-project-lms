# Coding standard
1. **Language**: C++17, no compiler extensions. Warnings `-Wall -Wextra -Wpedantic -Wshadow` must be clean.
2. **Naming**: classes `PascalCase`; functions and variables `camelCase`; private data members end with `_`; constants `kPascalCase`; files match the class name; namespace `lms`.
3. **Layout**: 4 spaces, no tabs, braces on the same line, lines under 110 characters, `#pragma once` in headers.
4. **Layers**: `ui` → `services` → `domain` ← `infrastructure`. Only `infrastructure` includes SQLite or libsodium. Dependencies go through the interfaces in `domain/Ports.hpp` and are passed in by constructor (see `main.cpp`).
5. **Ownership**: no raw `new`/`delete`; use `std::unique_ptr`, values and references. Private data with getters.
6. **Errors**: throw the types in `domain/Errors.hpp` (`ValidationError`, `AuthError`, `PermissionError`, `NotFoundError`, `RuleError`, `DatabaseError`). Catch only in the UI. Multi-step changes run in a `Transaction`.
7. **Input**: every external value is validated in `services/Validation` before use. SQL uses bound parameters only, never string concatenation.
8. **Comments**: explain why, and cite the rule id (BR/FR/NFR) a block implements.
9. **Tests**: every rule has a GoogleTest case, with boundary values for marks, dates and sizes.
10. **Commits**: one logical change each, imperative subject under 72 characters, work on `feature/*` branches merged into `develop`, releases tagged on `main`.
