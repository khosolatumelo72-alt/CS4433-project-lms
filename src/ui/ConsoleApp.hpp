#pragma once
#include "services/AdminService.hpp"
#include "services/AssignmentService.hpp"
#include "services/AuthService.hpp"
#include "services/MarkingService.hpp"
#include "services/ModerationService.hpp"
#include "services/SubmissionService.hpp"
#include "ui/Console.hpp"

namespace lms {

struct Services {
    AuthService& auth;
    AdminService& admin;
    AssignmentService& assignments;
    SubmissionService& submissions;
    MarkingService& marking;
    ModerationService& moderation;
};

/// Presentation layer: menus per role. It only reads input, calls a service and prints the result.
class ConsoleApp {
public:
    ConsoleApp(Console& io, Services services) : io_(io), s_(services) {}
    /// Login loop. Returns when the user chooses Exit or the input ends.
    void run();
    /// First start: asks for the first administrator account.
    void firstRunSetup();

private:
    void userMenu(const User& user);
    void accountMenu(const User& user);
    void studentMenu(const User& user);
    void lecturerMenu(const User& user);
    void hodMenu(const User& user);
    void adminMenu(const User& user);

    void changePassword(const User& user);
    int pickCourse(const std::vector<Course>& courses);
    int pickAssignment(const User& user, int courseId);
    void showAssignmentList(const User& user);
    void studentSubmit(const User& user);
    void studentViewResult(const User& user);
    void lecturerCreateAssignment(const User& user);
    void lecturerGrantExtension(const User& user);
    void lecturerNotSubmitted(const User& user);
    void lecturerMark(const User& user);
    void hodQueue(const User& user);
    void hodReview(const User& user);
    void hodReleaseAll(const User& user);
    void adminCreateUser(const User& admin);

    Console& io_;
    Services s_;
};

}  // namespace lms
