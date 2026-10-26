# Moderated Learning Management System (CS4433)
Console LMS in C++17 with SQLite. Roles: Student, Lecturer, Head of Department, Administrator.
Marks are moderated by the Head of Department and released to students only after approval (BR2, BR3, BR6),
late work is penalised by policy (BR01), and every mark change is audited (BR4).

See `BUILD.md` to build and run, and `CODING_STANDARD.md` for conventions.

## Layout
- `src/domain` entities, abstract `User` and subclasses, `UserFactory`, penalty strategies, repository interfaces
- `src/services` application rules (auth, admin, assignments, submissions, marking, moderation)
- `src/infrastructure` SQLite repositories, password hashing, clock
- `src/ui` console menus; `src/main.cpp` composition root
- `sql/schema.sql` database schema; `tests/` GoogleTest suites
