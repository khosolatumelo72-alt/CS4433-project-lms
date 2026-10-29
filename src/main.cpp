#include <cstdlib>
#include <iostream>
#include <string>

#include "domain/Errors.hpp"
#include "ui/Terminal.hpp"
#include "infrastructure/Database.hpp"
#include "infrastructure/SodiumPasswordHasher.hpp"
#include "infrastructure/SqliteAssignmentRepository.hpp"
#include "infrastructure/SqliteCourseRepository.hpp"
#include "infrastructure/SqliteMarkRepository.hpp"
#include "infrastructure/SqliteSubmissionRepository.hpp"
#include "infrastructure/SqliteUserRepository.hpp"
#include "infrastructure/SystemClock.hpp"
#include "ui/ConsoleApp.hpp"

using namespace lms;

namespace {
void usage() {
    std::cout << "Usage: lms [--db FILE]\n"
                 "  --db FILE   the SQLite database file (default: lms.db, or the LMS_DB variable)\n"
                 "  --help      show this text\n";
}
}  // namespace

// Composition root: the only place that knows the concrete classes (Dependency inversion).
int main(int argc, char** argv) {
    std::string path = std::getenv("LMS_DB") ? std::getenv("LMS_DB") : "lms.db";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") { usage(); return 0; }
        if (a == "--db" && i + 1 < argc) path = argv[++i];
        else { std::cerr << "Unknown option: " << a << "\n"; usage(); return 2; }
    }
    try {
        Database db(path);
        SqliteUserRepository users(db);
        SqliteCourseRepository courses(db);
        SqliteAssignmentRepository assignments(db);
        SqliteSubmissionRepository submissions(db);
        SqliteMarkRepository marks(db);
        SystemClock clock;
        SodiumPasswordHasher hasher;
        TenPercentPerDayPolicy penalty;

        AuthService auth(users, hasher, clock);
        AdminService admin(users, courses, hasher, db);
        AssignmentService assignmentService(assignments, courses, submissions, clock);
        SubmissionService submissionService(submissions, assignments, courses, users, marks, clock, db);
        MarkingService marking(submissions, marks, assignments, courses, users, penalty, clock, db);
        ModerationService moderation(submissions, marks, users, assignments, courses, clock, db);

        Console io(std::cin, std::cout, terminal::stdinIsTerminal());
        ConsoleApp app(io, Services{auth, admin, assignmentService, submissionService, marking, moderation});
        try {
            if (users.listByRole(Role::Administrator).empty()) app.firstRunSetup();
            app.run();
        } catch (const EndOfInput&) {
            std::cout << "\n";
        }
        return 0;
    } catch (const LmsError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }
}
