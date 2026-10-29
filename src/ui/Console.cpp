#include "ui/Console.hpp"

#include "ui/Terminal.hpp"

#include <iostream>

namespace lms {

void Console::println(const std::string& s) { out_ << s << '\n'; }

std::string Console::line(const std::string& prompt) {
    out_ << prompt << std::flush;
    std::string s;
    if (!std::getline(in_, s)) throw EndOfInput{};
    return s;
}

std::string Console::secret(const std::string& prompt) {
    const bool changed = hideSecrets_ && terminal::hideEcho();
    std::string s;
    try {
        s = line(prompt);
    } catch (...) {
        if (changed) terminal::restoreEcho();
        throw;
    }
    if (changed) { terminal::restoreEcho(); out_ << '\n'; }
    return s;
}

int Console::integer(const std::string& prompt, int lo, int hi) {
    for (;;) {
        std::string s = line(prompt);
        try {
            std::size_t used = 0;
            int v = std::stoi(s, &used);
            while (used < s.size() && s[used] == ' ') ++used;
            if (used == s.size() && v >= lo && v <= hi) return v;
        } catch (const std::exception&) {
        }
        println("Please type a whole number from " + std::to_string(lo) + " to " + std::to_string(hi) + ".");
    }
}

double Console::number(const std::string& prompt) {
    for (;;) {
        std::string s = line(prompt);
        try {
            std::size_t used = 0;
            double v = std::stod(s, &used);
            while (used < s.size() && s[used] == ' ') ++used;
            if (used == s.size()) return v;
        } catch (const std::exception&) {
        }
        println("Please type a number, for example 72.5.");
    }
}

TimePoint Console::time(const std::string& prompt) {
    for (;;) {
        if (auto t = parseUserTime(line(prompt))) return *t;
        println("Please type the date and time as YYYY-MM-DD HH:MM (UTC), for example 2026-11-30 23:59.");
    }
}

bool Console::confirm(const std::string& prompt) {
    for (;;) {
        std::string s = line(prompt + " (y/n): ");
        if (s == "y" || s == "Y" || s == "yes") return true;
        if (s == "n" || s == "N" || s == "no") return false;
    }
}

int Console::menu(const std::string& title, const std::vector<std::string>& items, const std::string& zero) {
    println();
    println("== " + title + " ==");
    for (std::size_t i = 0; i < items.size(); ++i) println("  " + std::to_string(i + 1) + ". " + items[i]);
    println("  0. " + zero);
    return integer("Choose: ", 0, static_cast<int>(items.size()));
}

void Console::pause() { line("Press Enter to continue..."); }

}  // namespace lms
