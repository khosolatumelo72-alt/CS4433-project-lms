#include "ui/ConsoleApp.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "domain/Errors.hpp"

namespace lms {

namespace {

/// Runs one action; an expected error becomes a message instead of a crash.
template <class F>
void guarded(Console& io, F&& f) {
    try {
        f();
    } catch (const EndOfInput&) {
        throw;
    } catch (const LmsError& e) {
        io.println("Error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        io.println("Unexpected problem: " + std::string(e.what()));
    }
}

/// Students must not learn a mark is ready before release (BR2), so Marked and Moderated look alike.
std::string studentStatus(SubmissionStatus s) {
    if (s == SubmissionStatus::Marked || s == SubmissionStatus::Moderated) return "Awaiting release";
    return toString(s);
}

std::string money(double v) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << v;
    return o.str();
}

std::string pad(const std::string& s, std::size_t n) { return s.size() >= n ? s : s + std::string(n - s.size(), ' '); }

std::vector<std::uint8_t> readFile(const std::string& path, std::int64_t maxBytes) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_regular_file(path, ec)) throw ValidationError("No such file: " + path);
    auto size = fs::file_size(path, ec);
    if (ec || size == 0) throw ValidationError("The file is empty or cannot be read");
    if (static_cast<std::int64_t>(size) > maxBytes) throw ValidationError("The file is larger than 10 MB");
    std::ifstream in(path, std::ios::binary);
    if (!in) throw ValidationError("The file cannot be opened");
    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    if (!in) throw ValidationError("The file could not be read completely");
    return data;
}

}  // namespace

void ConsoleApp::firstRunSetup() {
    io_.println("No administrator account exists yet. Let us create the first one.");
    for (;;) {
        std::string name = io_.line("Administrator user name: ");
        std::string pw = io_.secret("Password (at least 8 characters): ");
        std::string pw2 = io_.secret("Type the password again: ");
        if (pw != pw2) { io_.println("The passwords do not match. Try again."); continue; }
        try {
            s_.admin.createFirstAdministrator(name, pw);
            io_.println("Administrator created. You can now log in.");
            return;
        } catch (const LmsError& e) {
            io_.println("Error: " + std::string(e.what()));
        }
    }
}

void ConsoleApp::run() {
    io_.println("=== Moderated Learning Management System ===");
    try {
        for (;;) {
            std::string name = io_.line("\nUser name (just press Enter to exit): ");
            if (name.empty()) return;
            std::string pw = io_.secret("Password: ");
            guarded(io_, [&] {
                UserPtr user = s_.auth.login(name, pw);
                io_.println("Welcome, " + user->getUsername() + " (" + toString(user->role()) + ").");
                userMenu(*user);
                s_.auth.logout();
            });
        }
    } catch (const EndOfInput&) {
        io_.println();
    }
}

void ConsoleApp::userMenu(const User& user) {
    switch (user.role()) {
        case Role::Student: studentMenu(user); break;
        case Role::Lecturer: lecturerMenu(user); break;
        case Role::HeadOfDepartment: hodMenu(user); break;
        case Role::Administrator: adminMenu(user); break;
    }
}

void ConsoleApp::changePassword(const User& user) {
    std::string oldPw = io_.secret("Current password: ");
    std::string newPw = io_.secret("New password (at least 8 characters): ");
    std::string again = io_.secret("Type the new password again: ");
    if (newPw != again) throw ValidationError("The two new passwords do not match");
    s_.auth.changePassword(user.getId(), NewPassword{oldPw, newPw});
    io_.println("Password changed.");
}

int ConsoleApp::pickCourse(const std::vector<Course>& courses) {
    if (courses.empty()) { io_.println("There are no courses to show."); return 0; }
    for (std::size_t i = 0; i < courses.size(); ++i)
        io_.println("  " + std::to_string(i + 1) + ". " + courses[i].getCode() + "  " + courses[i].getTitle());
    int n = io_.integer("Choose a course (0 to cancel): ", 0, static_cast<int>(courses.size()));
    return n == 0 ? 0 : courses[static_cast<std::size_t>(n - 1)].getId();
}

int ConsoleApp::pickAssignment(const User& user, int courseId) {
    auto list = s_.assignments.listForCourse(user, courseId);
    if (list.empty()) { io_.println("This course has no assignments yet."); return 0; }
    for (std::size_t i = 0; i < list.size(); ++i)
        io_.println("  " + std::to_string(i + 1) + ". " + list[i].getTitle() + "  (deadline " +
                    displayTime(list[i].getDeadline()) + ", maximum mark " + money(list[i].getMaxMark()) + ")");
    int n = io_.integer("Choose an assignment (0 to cancel): ", 0, static_cast<int>(list.size()));
    return n == 0 ? 0 : list[static_cast<std::size_t>(n - 1)].getId();
}

void ConsoleApp::showAssignmentList(const User& user) {
    if (user.role() == Role::Student) {
        auto views = s_.assignments.listForStudent(user.getId());
        if (views.empty()) io_.println("You have no assignments yet.");
        for (const auto& v : views)
            io_.println(pad(v.courseCode, 10) + pad(v.assignment.getTitle(), 28) + " deadline " +
                        displayTime(v.assignment.getDeadline()) + "  " +
                        (v.status ? studentStatus(*v.status) : std::string("Not submitted")));
        return;
    }
    auto courses = s_.assignments.myCourses(user);
    if (courses.empty()) io_.println("You have no courses.");
    for (const auto& c : courses) {
        io_.println(c.getCode() + "  " + c.getTitle());
        for (const auto& a : s_.assignments.listForCourse(user, c.getId()))
            io_.println("    [" + std::to_string(a.getId()) + "] " + pad(a.getTitle(), 28) + " deadline " +
                        displayTime(a.getDeadline()) + ", maximum mark " + money(a.getMaxMark()));
    }
}

// ---------------------------------------------------------------- Student

void ConsoleApp::studentMenu(const User& user) {
    for (;;) {
        int c = io_.menu("Student menu", {"My assignments", "Submit or replace a file", "My submission and result",
                                           "Change my password"}, "Log out");
        if (c == 0) return;
        guarded(io_, [&] {
            if (c == 1) showAssignmentList(user);
            else if (c == 2) studentSubmit(user);
            else if (c == 3) studentViewResult(user);
            else changePassword(user);
        });
    }
}

void ConsoleApp::studentSubmit(const User& user) {
    auto views = s_.assignments.listForStudent(user.getId());
    if (views.empty()) { io_.println("You have no assignments yet."); return; }
    for (std::size_t i = 0; i < views.size(); ++i)
        io_.println("  " + std::to_string(i + 1) + ". " + views[i].courseCode + "  " + views[i].assignment.getTitle());
    int n = io_.integer("Choose an assignment (0 to cancel): ", 0, static_cast<int>(views.size()));
    if (n == 0) return;
    const Assignment& a = views[static_cast<std::size_t>(n - 1)].assignment;
    TimePoint eff = s_.submissions.effectiveDeadlineFor(user.getId(), a.getId());
    io_.println("Your deadline for this assignment is " + displayTime(eff) + ".");
    std::string path = io_.line("Path of the file to hand in (Enter to cancel): ");
    if (path.empty()) return;
    auto data = readFile(path, SubmissionService::kMaxFileBytes);
    std::string name = std::filesystem::path(path).filename().string();
    SubmitResult r = s_.submissions.submit(SubmitRequest{user.getId(), a.getId(), FileData{name, std::move(data)}});
    io_.println(std::string(r.isResubmission ? "Replaced. Your file is now version " : "Submitted. Version ") +
                std::to_string(r.versionNo) + " received at " + displayTime(r.submittedAt) + ".");
    if (r.status == SubmissionStatus::Late)
        io_.println("This work is LATE. A penalty will be applied when it is marked. It cannot be replaced.");
}

void ConsoleApp::studentViewResult(const User& user) {
    auto views = s_.assignments.listForStudent(user.getId());
    if (views.empty()) { io_.println("You have no assignments yet."); return; }
    for (std::size_t i = 0; i < views.size(); ++i)
        io_.println("  " + std::to_string(i + 1) + ". " + views[i].courseCode + "  " + views[i].assignment.getTitle());
    int n = io_.integer("Choose an assignment (0 to cancel): ", 0, static_cast<int>(views.size()));
    if (n == 0) return;
    SubmissionView v = s_.submissions.viewOwn(user.getId(), views[static_cast<std::size_t>(n - 1)].assignment.getId());
    io_.println(v.assignmentTitle + ", deadline " + displayTime(v.effectiveDeadline));
    if (!v.status) { io_.println("You have not submitted anything."); return; }
    io_.println("Status: " + studentStatus(*v.status) + ", file versions: " + std::to_string(v.versions));
    if (v.mark) {
        io_.println("Mark: " + money(v.mark->getFinalMark()) + (v.mark->getPenalty() > 0
                        ? "  (marked " + money(v.mark->getAwardedMark()) + ", late penalty " + money(v.mark->getPenalty()) + ")"
                        : std::string()));
        io_.println("Feedback: " + v.mark->getFeedback());
    }
}

// --------------------------------------------------------------- Lecturer

void ConsoleApp::lecturerMenu(const User& user) {
    for (;;) {
        int c = io_.menu("Lecturer menu", {"My courses and assignments", "Create an assignment", "Grant an extension",
                                            "Students who have not submitted", "Mark submissions", "Change my password"},
                         "Log out");
        if (c == 0) return;
        guarded(io_, [&] {
            if (c == 1) showAssignmentList(user);
            else if (c == 2) lecturerCreateAssignment(user);
            else if (c == 3) lecturerGrantExtension(user);
            else if (c == 4) lecturerNotSubmitted(user);
            else if (c == 5) lecturerMark(user);
            else changePassword(user);
        });
    }
}

void ConsoleApp::lecturerCreateAssignment(const User& user) {
    int course = pickCourse(s_.assignments.myCourses(user));
    if (!course) return;
    NewAssignment a;
    a.courseId = course;
    a.title = io_.line("Title: ");
    a.description = io_.line("Description (may be empty): ");
    a.deadline = io_.time("Deadline (YYYY-MM-DD HH:MM, UTC): ");
    a.maxMark = io_.number("Maximum mark: ");
    int id = s_.assignments.createAssignment(user.getId(), a);
    io_.println("Assignment created (number " + std::to_string(id) + ").");
}

void ConsoleApp::lecturerGrantExtension(const User& user) {
    int course = pickCourse(s_.assignments.myCourses(user));
    if (!course) return;
    int asg = pickAssignment(user, course);
    if (!asg) return;
    NewExtension e;
    e.assignmentId = asg;
    e.studentId = io_.integer("Student number (see 'Students who have not submitted'): ", 1, 1000000000);
    e.newDeadline = io_.time("New deadline (YYYY-MM-DD HH:MM, UTC): ");
    e.reason = io_.line("Reason: ");
    s_.assignments.grantExtension(user.getId(), e);
    io_.println("Extension granted.");
}

void ConsoleApp::lecturerNotSubmitted(const User& user) {
    int course = pickCourse(s_.assignments.myCourses(user));
    if (!course) return;
    int asg = pickAssignment(user, course);
    if (!asg) return;
    auto list = s_.submissions.listNotSubmitted(user.getId(), asg);
    if (list.empty()) io_.println("Every enrolled student has submitted.");
    for (const auto& u : list) io_.println("  student number " + std::to_string(u->getId()) + "  " + u->getUsername());
}

void ConsoleApp::lecturerMark(const User& user) {
    int course = pickCourse(s_.assignments.myCourses(user));
    if (!course) return;
    int asg = pickAssignment(user, course);
    if (!asg) return;
    for (;;) {
        auto rows = s_.marking.listToMark(user.getId(), asg);
        if (rows.empty()) { io_.println("No submissions yet."); return; }
        io_.println("\n  No.  Student           Status      Mark");
        for (const auto& r : rows)
            io_.println("  " + pad(std::to_string(r.submission.getId()), 5) + pad(r.studentName, 18) +
                        pad(toString(r.submission.getStatus()), 12) + (r.finalMark ? money(*r.finalMark) : "-"));
        int id = io_.integer("Submission number to open (0 to go back): ", 0, 1000000000);
        if (id == 0) return;
        guarded(io_, [&] {
            int a = io_.menu("Submission " + std::to_string(id), {"Save a copy of the student's file", "Enter mark and feedback"});
            if (a == 1) {
                SubmissionVersion v = s_.marking.latestFile(user.getId(), id);
                std::string out = io_.line("Save as (path, default " + v.getFileName() + "): ");
                if (out.empty()) out = v.getFileName();
                if (std::filesystem::exists(out) && !io_.confirm("That file exists. Overwrite it?")) return;
                std::ofstream f(out, std::ios::binary);
                f.write(reinterpret_cast<const char*>(v.getContent().data()), static_cast<std::streamsize>(v.getContent().size()));
                if (!f) throw ValidationError("The file could not be written");
                io_.println("Saved " + std::to_string(v.getFileSize()) + " bytes (version " + std::to_string(v.getVersionNo()) + ").");
            } else if (a == 2) {
                MarkRequest m;
                m.lecturerId = user.getId();
                m.submissionId = id;
                m.awardedMark = io_.number("Mark: ");
                m.feedback = io_.line("Written feedback: ");
                if (m.feedback.find_first_not_of(" \t") == std::string::npos) {
                    io_.println("Warning: the Head of Department cannot release a mark without feedback.");
                    if (!io_.confirm("Save without feedback?")) return;
                }
                s_.marking.recordMark(m);
                io_.println("Mark saved. It now waits for the Head of Department.");
            }
        });
    }
}

// ----------------------------------------------------------------- HoD

void ConsoleApp::hodMenu(const User& user) {
    for (;;) {
        int c = io_.menu("Head of Department menu", {"Courses and assignments", "Moderation queue",
                                                      "Review one submission", "Release all marks of an assignment",
                                                      "Change my password"}, "Log out");
        if (c == 0) return;
        guarded(io_, [&] {
            if (c == 1) showAssignmentList(user);
            else if (c == 2) hodQueue(user);
            else if (c == 3) hodReview(user);
            else if (c == 4) hodReleaseAll(user);
            else changePassword(user);
        });
    }
}

void ConsoleApp::hodQueue(const User& user) {
    auto rows = s_.moderation.queue(user.getId());
    if (rows.empty()) { io_.println("Nothing is waiting for moderation."); return; }
    io_.println("  No.  Student           Status      Mark   Feedback");
    for (const auto& r : rows)
        io_.println("  " + pad(std::to_string(r.submission.getId()), 5) + pad(r.studentName, 18) +
                    pad(toString(r.submission.getStatus()), 12) + pad(r.finalMark ? money(*r.finalMark) : "-", 7) +
                    (r.hasFeedback ? "yes" : "NO"));
}

void ConsoleApp::hodReview(const User& user) {
    hodQueue(user);
    int id = io_.integer("Submission number to review (0 to cancel): ", 0, 1000000000);
    if (id == 0) return;
    for (;;) {
        auto mark = s_.moderation.viewMark(user.getId(), id);
        if (!mark) { io_.println("This submission has no mark yet."); return; }
        io_.println("\nAwarded " + money(mark->getAwardedMark()) + ", penalty " + money(mark->getPenalty()) +
                    ", final mark " + money(mark->getFinalMark()));
        io_.println("Feedback: " + (mark->hasFeedback() ? mark->getFeedback() : std::string("(none)")));
        int a = io_.menu("Submission " + std::to_string(id), {"Approve", "Change the final mark", "Release", "Show change log"});
        if (a == 0) return;
        guarded(io_, [&] {
            if (a == 1) { s_.moderation.approve(user.getId(), id); io_.println("Approved."); }
            else if (a == 2) {
                MarkChange ch;
                ch.submissionId = id;
                ch.newFinalMark = io_.number("New final mark: ");
                ch.reason = io_.line("Reason: ");
                s_.moderation.changeMark(user.getId(), ch);
                io_.println("Mark changed and logged.");
            } else if (a == 3) {
                s_.moderation.release(user.getId(), id);
                io_.println("Released. The student can now see the mark and feedback.");
            } else {
                for (const auto& e : s_.moderation.viewAudit(user.getId(), id))
                    io_.println("  " + displayTime(e.getAt()) + "  user " + std::to_string(e.getActorId()) + "  " +
                                toString(e.getAction()) + "  old " + (e.getOldValue() ? money(*e.getOldValue()) : "-") +
                                "  new " + (e.getNewValue() ? money(*e.getNewValue()) : "-") + "  " + e.getReason());
            }
        });
    }
}

void ConsoleApp::hodReleaseAll(const User& user) {
    int course = pickCourse(s_.assignments.myCourses(user));
    if (!course) return;
    int asg = pickAssignment(user, course);
    if (!asg) return;
    if (!io_.confirm("Release every Marked or Moderated submission of this assignment that has feedback?")) return;
    ReleaseReport r = s_.moderation.releaseAll(user.getId(), asg);
    io_.println(std::to_string(r.released.size()) + " submission(s) released.");
    for (const auto& [id, why] : r.skipped) io_.println("  skipped " + std::to_string(id) + ": " + why);
}

// ---------------------------------------------------------------- Admin

void ConsoleApp::adminMenu(const User& user) {
    for (;;) {
        int c = io_.menu("Administrator menu",
                         {"Create a department", "Create a course", "Create a user", "Disable or enable a user",
                          "Assign a lecturer to a course", "Enrol a student in a course", "List users",
                          "List departments and courses", "Change my password"}, "Log out");
        if (c == 0) return;
        guarded(io_, [&] {
            const int me = user.getId();
            if (c == 1) {
                int id = s_.admin.createDepartment(me, io_.line("Department name: "));
                io_.println("Department created (number " + std::to_string(id) + ").");
            } else if (c == 2) {
                NewCourse n;
                for (const auto& d : s_.admin.listDepartments(me)) io_.println("  department " + std::to_string(d.getId()) + "  " + d.getName());
                n.departmentId = io_.integer("Department number: ", 1, 1000000000);
                n.code = io_.line("Course code: ");
                n.title = io_.line("Course title: ");
                io_.println("Course created (number " + std::to_string(s_.admin.createCourse(me, n)) + ").");
            } else if (c == 3) adminCreateUser(user);
            else if (c == 4) {
                int id = io_.integer("User number: ", 1, 1000000000);
                bool on = io_.confirm("Enable this account? (n = disable)");
                s_.admin.setActive(me, id, on);
                io_.println(on ? "Account enabled." : "Account disabled.");
            } else if (c == 5) {
                int course = io_.integer("Course number: ", 1, 1000000000);
                int lec = io_.integer("Lecturer's user number: ", 1, 1000000000);
                s_.admin.assignLecturer(me, course, lec);
                io_.println("Lecturer assigned.");
            } else if (c == 6) {
                int course = io_.integer("Course number: ", 1, 1000000000);
                int stu = io_.integer("Student's user number: ", 1, 1000000000);
                s_.admin.enrol(me, course, stu);
                io_.println("Student enrolled.");
            } else if (c == 7) {
                for (const auto& u : s_.admin.listUsers(me))
                    io_.println("  " + pad(std::to_string(u->getId()), 5) + pad(u->getUsername(), 20) + pad(toString(u->role()), 18) +
                                (u->isActive() ? "active" : "DISABLED"));
            } else if (c == 8) {
                for (const auto& d : s_.admin.listDepartments(me)) io_.println("  department " + std::to_string(d.getId()) + "  " + d.getName());
                for (const auto& co : s_.admin.listCourses(me))
                    io_.println("  course " + pad(std::to_string(co.getId()), 4) + pad(co.getCode(), 10) + pad(co.getTitle(), 30) + "department " + std::to_string(co.getDepartmentId()));
            } else changePassword(user);
        });
    }
}

void ConsoleApp::adminCreateUser(const User& admin) {
    int r = io_.menu("Role of the new user", {"Student", "Lecturer", "Head of Department", "Administrator"}, "Cancel");
    if (r == 0) return;
    NewUserRequest req;
    req.role = std::vector<Role>{Role::Student, Role::Lecturer, Role::HeadOfDepartment, Role::Administrator}[static_cast<std::size_t>(r - 1)];
    req.username = io_.line("User name (3-32 letters, digits, . _ -): ");
    req.password = io_.secret("Start password (at least 8 characters): ");
    if (req.role == Role::HeadOfDepartment) {
        for (const auto& d : s_.admin.listDepartments(admin.getId())) io_.println("  department " + std::to_string(d.getId()) + "  " + d.getName());
        req.departmentId = io_.integer("Department number: ", 1, 1000000000);
    }
    int id = s_.admin.createUser(admin.getId(), req);
    io_.println("User created (number " + std::to_string(id) + ").");
}

}  // namespace lms
