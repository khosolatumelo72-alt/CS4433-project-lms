#include "ui/Terminal.hpp"

#ifdef _WIN32
#include <cstdio>
#include <io.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace lms::terminal {
namespace {
#ifdef _WIN32
DWORD savedMode = 0;
#else
termios saved{};
#endif
}  // namespace

bool stdinIsTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

bool hideEcho() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE || !GetConsoleMode(h, &savedMode)) return false;
    return SetConsoleMode(h, savedMode & ~static_cast<DWORD>(ENABLE_ECHO_INPUT)) != 0;
#else
    if (tcgetattr(STDIN_FILENO, &saved) != 0) return false;
    termios t = saved;
    t.c_lflag &= static_cast<tcflag_t>(~ECHO);
    return tcsetattr(STDIN_FILENO, TCSANOW, &t) == 0;
#endif
}

void restoreEcho() {
#ifdef _WIN32
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), savedMode);
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
#endif
}
}  // namespace lms::terminal
