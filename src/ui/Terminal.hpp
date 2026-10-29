#pragma once
// Small platform wrapper: the only place that knows about the operating system's console.
namespace lms::terminal {
bool stdinIsTerminal();
/// Switches echo off for typed secrets; returns true if it was changed (so restore() should be called).
bool hideEcho();
void restoreEcho();
}  // namespace lms::terminal
