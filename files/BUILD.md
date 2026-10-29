# Build guide (CS4433 Moderated LMS, v1.0)

## Requirements
| Tool | Version | Purpose |
|---|---|---|
| C++17 compiler | g++ 9+ or clang 10+ | build |
| CMake | 3.16+ | build system |
| SQLite3 dev files | 3.31+ | database (`libsqlite3-dev`) |
| libsodium dev files | 1.0.18+ | Argon2id password hashing (`libsodium-dev`) |
| pkg-config | any | locates libsodium |
| GoogleTest | optional | unit tests; fetched automatically if not installed |

Ubuntu/Debian: `sudo apt install build-essential cmake pkg-config libsqlite3-dev libsodium-dev`

## Windows (MSYS2, no WSL needed)
1. Install MSYS2 from https://www.msys2.org and open the **UCRT64** terminal.
2. `pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-sqlite3 mingw-w64-ucrt-x86_64-libsodium mingw-w64-ucrt-x86_64-pkgconf git unzip`
3. Go to the project folder (your C: drive is `/c/...`) and run the commands below, adding `-G Ninja` to the first one.
The code uses no POSIX-only time functions; the only platform code is `src/ui/Terminal.cpp`.
Note: this Windows route was written but not run by the author; the Linux build is the verified one.

## Build, test, run
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/lms --db lms.db
```
On first run, with no administrator in the database, the program asks you to create one.
The database file is created and the schema (`sql/schema.sql`) applied automatically.

Options: `--db FILE`, or the `LMS_DB` environment variable. Default file: `lms.db`.

## Coverage (optional)
`cmake -S . -B build-cov -DLMS_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug && cmake --build build-cov && ctest --test-dir build-cov` then run `gcov` on the objects.

## Why Argon2id from libsodium
Passwords are never stored. Each is hashed with Argon2id (the winner of the Password Hashing Competition and the OWASP first choice), with a random per-password salt, through `crypto_pwhash_str`. Libsodium is a widely audited library with a hard-to-misuse API, so we do not write any cryptography ourselves. Memory-hard hashing makes offline guessing expensive. Interactive cost limits are used so login stays fast.

## Security notes
- All SQL is parameterised and lives only in the `infrastructure` layer.
- Accounts lock for 5 minutes after 5 wrong passwords; login errors never reveal whether the user exists.
- The audit log is append-only (database triggers refuse UPDATE and DELETE).
- Times are stored and entered as UTC.
