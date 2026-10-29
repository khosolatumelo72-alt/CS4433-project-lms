#pragma once
#include <iosfwd>
#include <string>
#include <vector>

#include "domain/Types.hpp"

namespace lms {

/// Thrown when the input stream ends (Ctrl-D), so the program can leave cleanly.
struct EndOfInput {};

/// Small helper for reading validated input from a stream and printing to another.
class Console {
public:
    Console(std::istream& in, std::ostream& out, bool hideSecrets) : in_(in), out_(out), hideSecrets_(hideSecrets) {}
    std::ostream& out() { return out_; }
    void println(const std::string& s = "");
    std::string line(const std::string& prompt);
    std::string secret(const std::string& prompt);
    /// Asks until a whole number inside [lo, hi] is typed.
    int integer(const std::string& prompt, int lo, int hi);
    double number(const std::string& prompt);
    /// Asks for "YYYY-MM-DD HH:MM" (UTC) until valid.
    TimePoint time(const std::string& prompt);
    bool confirm(const std::string& prompt);
    /// Prints a numbered menu; returns the chosen number (0 = back/exit).
    int menu(const std::string& title, const std::vector<std::string>& items, const std::string& zero = "Back");
    void pause();

private:
    std::istream& in_;
    std::ostream& out_;
    bool hideSecrets_;
};

}  // namespace lms
