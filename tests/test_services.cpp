#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>

#include "domain/Errors.hpp"
#include "infrastructure/Database.hpp"
#include "infrastructure/SodiumPasswordHasher.hpp"
#include "infrastructure/SqliteAssignmentRepository.hpp"
#include "infrastructure/SqliteCourseRepository.hpp"
#include "infrastructure/SqliteMarkRepository.hpp"
#include "infrastructure/SqliteSubmissionRepository.hpp"
#include "infrastructure/SqliteUserRepository.hpp"
#include "services/AdminService.hpp"
#include "services/AssignmentService.hpp"
#include "services/AuthService.hpp"
#include "services/MarkingService.hpp"
#include "services/ModerationService.hpp"
#include "services/SubmissionService.hpp"

using namespace lms;
using std::chrono::hours;
using std::chrono::minutes;

namespace {
class FakeClock : public IClock {
public:
    TimePoint t = parseTime("2026-10-20T09:00:00Z");
    TimePoint now() override { return t; }
};
class FakeHasher : public IPasswordHasher {  // fast stand-in so most tests do not pay for Argon2
public:
    std::string hash(const std::string& p) override { return "H:" + p; }
    bool verify(const std::string& p, const std::string& h) override { return h == "H:" + p; }
};

std::vector<std::uint8_t> bytes(const std::string& s) { return {s.begin(), s.end()}; }

struct World : ::testing::Test {
    Database db{":memory:"};
    SqliteUserRepository users{db};
    SqliteCourseRepository courses{db};
    SqliteAssignmentRepository assignments{db};
    SqliteSubmissionRepository submissions{db};
    SqliteMarkRepository marks{db};
    FakeClock clock;
    FakeHasher hasher;
    TenPercentPerDayPolicy penalty;
    AuthService auth{users, hasher, clock};
    AdminService admin{users, courses, hasher, db};
    AssignmentService asgSvc{assignments, courses, submissions, clock};
    SubmissionService subSvc{submissions, assignments, courses, users, marks, clock, db};
    MarkingService marking{submissions, marks, assignments, courses, users, penalty, clock, db};
    ModerationService moderation{submissions, marks, users, assignments, courses, clock, db};

    int adminId = 0, dept = 0, otherDept = 0, hod = 0, otherHod = 0, lec = 0, lec2 = 0, stu = 0, stu2 = 0, course = 0, otherCourse = 0, asg = 0;
    const TimePoint deadline = parseTime("2026-11-01T12:00:00Z");

    void SetUp() override {
        adminId = admin.createFirstAdministrator("admin", "adminpass1");
        dept = admin.createDepartment(adminId, "Computing");
        otherDept = admin.createDepartment(adminId, "Maths");
        hod = admin.createUser(adminId, {Role::HeadOfDepartment, "hod1", "password1", dept});
        otherHod = admin.createUser(adminId, {Role::HeadOfDepartment, "hod2", "password2", otherDept});
        lec = admin.createUser(adminId, {Role::Lecturer, "lec1", "password3", 0});
        lec2 = admin.createUser(adminId, {Role::Lecturer, "lec2", "password4", 0});
        stu = admin.createUser(adminId, {Role::Student, "stu1", "password5", 0});
        stu2 = admin.createUser(adminId, {Role::Student, "stu2", "password6", 0});
        course = admin.createCourse(adminId, {"CS4433", "Software Engineering", dept});
        otherCourse = admin.createCourse(adminId, {"MA1000", "Algebra", otherDept});
        admin.assignLecturer(adminId, course, lec);
        admin.enrol(adminId, course, stu);
        admin.enrol(adminId, course, stu2);
        asg = asgSvc.createAssignment(lec, {course, "Assignment 1", "desc", deadline, 100});
    }
    SubmitResult submitAt(int student, TimePoint when, const std::string& content = "file") {
        clock.t = when;
        return subSvc.submit({student, asg, FileData{"work.txt", bytes(content)}});
    }
    int markOnTime(int student, double mark, const std::string& feedback = "Good") {
        auto r = submitAt(student, deadline - hours(2));
        clock.t = deadline + hours(1);
        marking.recordMark({lec, r.submissionId, mark, feedback});
        return r.submissionId;
    }
};
}  // namespace

// ---------------------------------------------------------------- submission
TEST_F(World, OnTimeSubmissionIsSubmitted) {
    auto r = submitAt(stu, deadline - hours(1));
    EXPECT_EQ(r.status, SubmissionStatus::Submitted);
    EXPECT_EQ(r.versionNo, 1);
    EXPECT_FALSE(r.isResubmission);
}
TEST_F(World, SubmissionExactlyAtDeadlineIsOnTime) {
    EXPECT_EQ(submitAt(stu, deadline).status, SubmissionStatus::Submitted);
}
TEST_F(World, OneSecondAfterDeadlineIsLateButAccepted) {  // BR5
    auto r = submitAt(stu, deadline + std::chrono::seconds(1));
    EXPECT_EQ(r.status, SubmissionStatus::Late);
}
TEST_F(World, ResubmissionBeforeDeadlineKeepsAllVersions) {  // FR10
    submitAt(stu, deadline - hours(10), "first");
    auto r = submitAt(stu, deadline - hours(5), "second");
    EXPECT_TRUE(r.isResubmission);
    EXPECT_EQ(r.versionNo, 2);
    auto latest = submissions.latestVersion(r.submissionId);
    EXPECT_EQ(latest->getContent(), bytes("second"));
}
TEST_F(World, ResubmissionAfterDeadlineRefused) {
    submitAt(stu, deadline - hours(1));
    EXPECT_THROW(submitAt(stu, deadline + hours(1)), RuleError);
}
TEST_F(World, LateWorkCannotBeReplaced) {
    submitAt(stu, deadline + hours(1));
    EXPECT_THROW(submitAt(stu, deadline + hours(2)), RuleError);
}
TEST_F(World, MarkedWorkCannotBeReplaced) {
    int id = markOnTime(stu, 70);
    (void)id;
    clock.t = deadline - hours(3) + hours(1);
    EXPECT_THROW(subSvc.submit({stu, asg, FileData{"x", bytes("y")}}), RuleError);
}
TEST_F(World, ExtensionTurnsLateIntoOnTime) {  // FR13
    asgSvc.grantExtension(lec, {asg, stu, deadline + hours(48), "ill"});
    clock.t = deadline + hours(24);
    EXPECT_EQ(subSvc.submit({stu, asg, FileData{"w", bytes("a")}}).status, SubmissionStatus::Submitted);
    // the other student has no extension
    EXPECT_EQ(subSvc.submit({stu2, asg, FileData{"w", bytes("a")}}).status, SubmissionStatus::Late);
}
TEST_F(World, ExtensionRules) {
    EXPECT_THROW(asgSvc.grantExtension(lec, {asg, stu, deadline - hours(1), "x"}), ValidationError);
    EXPECT_THROW(asgSvc.grantExtension(lec, {asg, stu, deadline + hours(1), "  "}), ValidationError);
    EXPECT_THROW(asgSvc.grantExtension(lec2, {asg, stu, deadline + hours(1), "x"}), PermissionError);
    int outsider = admin.createUser(adminId, {Role::Student, "stu9", "password9", 0});
    EXPECT_THROW(asgSvc.grantExtension(lec, {asg, outsider, deadline + hours(1), "x"}), ValidationError);
    asgSvc.grantExtension(lec, {asg, stu, deadline + hours(10), "a"});
    asgSvc.grantExtension(lec, {asg, stu, deadline + hours(30), "b"});
    EXPECT_EQ(subSvc.effectiveDeadlineFor(stu, asg), deadline + hours(30));
}
TEST_F(World, FileValidation) {
    clock.t = deadline - hours(1);
    EXPECT_THROW(subSvc.submit({stu, asg, FileData{"a", {}}}), ValidationError);
    EXPECT_THROW(subSvc.submit({stu, asg, FileData{"  ", bytes("x")}}), ValidationError);
    std::vector<std::uint8_t> big(static_cast<std::size_t>(SubmissionService::kMaxFileBytes) + 1, 1);
    EXPECT_THROW(subSvc.submit({stu, asg, FileData{"big", big}}), ValidationError);
    EXPECT_FALSE(submissions.find(asg, stu).has_value());  // nothing stored (NFR6)
    std::vector<std::uint8_t> exact(static_cast<std::size_t>(SubmissionService::kMaxFileBytes), 1);
    EXPECT_NO_THROW(subSvc.submit({stu, asg, FileData{"max", exact}}));
}
TEST_F(World, OnlyEnrolledStudentsMaySubmit) {
    int outsider = admin.createUser(adminId, {Role::Student, "stu9", "password9", 0});
    EXPECT_THROW(subSvc.submit({outsider, asg, FileData{"a", bytes("x")}}), PermissionError);
    EXPECT_THROW(subSvc.submit({stu, 9999, FileData{"a", bytes("x")}}), NotFoundError);
}
TEST_F(World, NotSubmittedList) {  // FR18
    submitAt(stu, deadline - hours(1));
    auto list = subSvc.listNotSubmitted(lec, asg);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->getId(), stu2);
    EXPECT_THROW(subSvc.listNotSubmitted(lec2, asg), PermissionError);
}

// ------------------------------------------------------------------- marking
TEST_F(World, OnTimeMarkHasNoPenaltyAndIsAudited) {
    int id = markOnTime(stu, 80);
    auto m = marks.find(id);
    EXPECT_DOUBLE_EQ(m->getPenalty(), 0);
    EXPECT_DOUBLE_EQ(m->getFinalMark(), 80);
    EXPECT_EQ(submissions.findById(id)->getStatus(), SubmissionStatus::Marked);
    auto log = marks.listAudit(id);
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0].getAction(), AuditAction::MarkSet);
    EXPECT_EQ(log[0].getActorId(), lec);
    EXPECT_FALSE(log[0].getOldValue().has_value());
    EXPECT_DOUBLE_EQ(*log[0].getNewValue(), 80);
}
TEST_F(World, LatePenaltyAppliedAtMarking) {  // BR01
    auto r = submitAt(stu, deadline + hours(30));  // 2 started days late
    marking.recordMark({lec, r.submissionId, 80, "ok"});
    auto m = marks.find(r.submissionId);
    EXPECT_DOUBLE_EQ(m->getAwardedMark(), 80);
    EXPECT_DOUBLE_EQ(m->getPenalty(), 20);
    EXPECT_DOUBLE_EQ(m->getFinalMark(), 60);
}
TEST_F(World, MoreThanThreeDaysLateGetsZero) {
    auto r = submitAt(stu, deadline + hours(73));
    marking.recordMark({lec, r.submissionId, 90, "ok"});
    EXPECT_DOUBLE_EQ(marks.find(r.submissionId)->getFinalMark(), 0);
    EXPECT_DOUBLE_EQ(marks.find(r.submissionId)->getAwardedMark(), 90);
}
TEST_F(World, ExtensionReducesPenalty) {
    asgSvc.grantExtension(lec, {asg, stu, deadline + hours(24), "reason"});
    auto r = submitAt(stu, deadline + hours(30));  // 6 hours after the extended deadline
    marking.recordMark({lec, r.submissionId, 100, ""});
    EXPECT_DOUBLE_EQ(marks.find(r.submissionId)->getPenalty(), 10);
}
TEST_F(World, MarkingValidationAndAccess) {
    auto r = submitAt(stu, deadline - hours(1));
    EXPECT_THROW(marking.recordMark({lec, r.submissionId, 101, "x"}), ValidationError);
    EXPECT_THROW(marking.recordMark({lec, r.submissionId, -1, "x"}), ValidationError);
    EXPECT_THROW(marking.recordMark({lec2, r.submissionId, 50, "x"}), PermissionError);
    EXPECT_THROW(marking.recordMark({stu, r.submissionId, 50, "x"}), PermissionError);
    EXPECT_THROW(marking.recordMark({lec, 9999, 50, "x"}), NotFoundError);
    EXPECT_NO_THROW(marking.recordMark({lec, r.submissionId, 100, "x"}));  // boundary: full marks
    EXPECT_THROW(marking.recordMark({lec, r.submissionId, 60, "x"}), RuleError);  // already marked
    EXPECT_TRUE(marks.listAudit(r.submissionId).size() == 1);
}

// ---------------------------------------------------------------- moderation
TEST_F(World, StudentCannotSeeMarkBeforeRelease) {  // BR2
    int id = markOnTime(stu, 70);
    EXPECT_FALSE(subSvc.viewOwn(stu, asg).mark.has_value());
    moderation.approve(hod, id);
    EXPECT_FALSE(subSvc.viewOwn(stu, asg).mark.has_value());
    moderation.release(hod, id);
    auto v = subSvc.viewOwn(stu, asg);
    ASSERT_TRUE(v.mark.has_value());
    EXPECT_DOUBLE_EQ(v.mark->getFinalMark(), 70);
    EXPECT_EQ(v.mark->getFeedback(), "Good");
}
TEST_F(World, ReleaseNeedsFeedback) {  // BR6
    int id = markOnTime(stu, 70, "");
    EXPECT_THROW(moderation.release(hod, id), RuleError);
    EXPECT_EQ(submissions.findById(id)->getStatus(), SubmissionStatus::Marked);
    EXPECT_EQ(marks.listAudit(id).size(), 1u);  // no approval was left behind (rolled back)
}
TEST_F(World, ReleasingMarkedSubmissionRecordsApproval) {  // BR3
    int id = markOnTime(stu, 70);
    moderation.release(hod, id);
    EXPECT_EQ(submissions.findById(id)->getStatus(), SubmissionStatus::Released);
    auto log = marks.listAudit(id);
    ASSERT_EQ(log.size(), 2u);
    EXPECT_EQ(log[1].getAction(), AuditAction::Approved);
    EXPECT_EQ(log[1].getActorId(), hod);
}
TEST_F(World, ApproveThenReleaseDoesNotApproveTwice) {
    int id = markOnTime(stu, 70);
    moderation.approve(hod, id);
    EXPECT_THROW(moderation.approve(hod, id), RuleError);
    moderation.release(hod, id);
    EXPECT_EQ(marks.listAudit(id).size(), 2u);
    EXPECT_THROW(moderation.release(hod, id), RuleError);  // already released
}
TEST_F(World, ChangeMarkIsLoggedWithReason) {  // BR4, NFR7
    int id = markOnTime(stu, 70);
    EXPECT_THROW(moderation.changeMark(hod, {id, 75, ""}), ValidationError);
    EXPECT_THROW(moderation.changeMark(hod, {id, 101, "x"}), ValidationError);
    EXPECT_THROW(moderation.changeMark(hod, {id, 70, "same"}), ValidationError);
    EXPECT_THROW(moderation.changeMark(lec, {id, 75, "x"}), PermissionError);  // lecturers cannot change after marking
    clock.t += minutes(5);
    moderation.changeMark(hod, {id, 75, "marking error"});
    EXPECT_DOUBLE_EQ(marks.find(id)->getFinalMark(), 75);
    auto log = marks.listAudit(id);
    ASSERT_EQ(log.size(), 2u);
    EXPECT_EQ(log[1].getAction(), AuditAction::MarkChanged);
    EXPECT_DOUBLE_EQ(*log[1].getOldValue(), 70);
    EXPECT_DOUBLE_EQ(*log[1].getNewValue(), 75);
    EXPECT_EQ(log[1].getReason(), "marking error");
    EXPECT_EQ(log[1].getAt(), clock.t);
}
TEST_F(World, ChangeAfterReleaseRefused) {
    int id = markOnTime(stu, 70);
    moderation.release(hod, id);
    EXPECT_THROW(moderation.changeMark(hod, {id, 60, "late change"}), RuleError);
}
TEST_F(World, HeadOfOtherDepartmentHasNoAccess) {
    int id = markOnTime(stu, 70);
    EXPECT_THROW(moderation.release(otherHod, id), PermissionError);
    EXPECT_THROW(moderation.approve(otherHod, id), PermissionError);
    EXPECT_THROW(moderation.viewAudit(otherHod, id), PermissionError);
    EXPECT_THROW(moderation.releaseAll(otherHod, asg), PermissionError);
    EXPECT_TRUE(moderation.queue(otherHod).empty());
    EXPECT_EQ(moderation.queue(hod).size(), 1u);
}
TEST_F(World, OnlyHeadOfDepartmentCanModerate) {
    int id = markOnTime(stu, 70);
    EXPECT_THROW(moderation.release(lec, id), PermissionError);
    EXPECT_THROW(moderation.release(stu, id), PermissionError);
    EXPECT_THROW(moderation.release(adminId, id), PermissionError);
    EXPECT_THROW(moderation.queue(9999), PermissionError);
}
TEST_F(World, ReleaseAllSkipsMissingFeedbackAndReportsIt) {  // UC21 3b
    int a = markOnTime(stu, 70, "Good");
    int b = markOnTime(stu2, 60, "");
    auto report = moderation.releaseAll(hod, asg);
    ASSERT_EQ(report.released.size(), 1u);
    EXPECT_EQ(report.released[0], a);
    ASSERT_EQ(report.skipped.size(), 1u);
    EXPECT_EQ(report.skipped[0].first, b);
    EXPECT_EQ(submissions.findById(b)->getStatus(), SubmissionStatus::Marked);
}

// ---------------------------------------------------------------------- auth
TEST_F(World, LoginSuccessAndFailure) {
    EXPECT_EQ(auth.login("stu1", "password5")->getId(), stu);
    EXPECT_THROW(auth.login("stu1", "wrong"), AuthError);
    EXPECT_THROW(auth.login("nobody", "password5"), AuthError);
    try { auth.login("nobody", "x"); } catch (const AuthError& e) { EXPECT_STREQ(e.what(), "Invalid user name or password"); }
}
TEST_F(World, AccountLocksAfterFiveWrongPasswords) {
    for (int i = 0; i < 5; ++i) EXPECT_THROW(auth.login("stu1", "bad"), AuthError);
    EXPECT_THROW(auth.login("stu1", "password5"), AuthError);  // locked even with the right password
    clock.t += minutes(User::kLockMinutes) + std::chrono::seconds(1);
    EXPECT_NO_THROW(auth.login("stu1", "password5"));
}
TEST_F(World, SuccessfulLoginResetsFailureCount) {
    for (int i = 0; i < 4; ++i) EXPECT_THROW(auth.login("stu1", "bad"), AuthError);
    auth.login("stu1", "password5");
    for (int i = 0; i < 4; ++i) EXPECT_THROW(auth.login("stu1", "bad"), AuthError);
    EXPECT_NO_THROW(auth.login("stu1", "password5"));
}
TEST_F(World, DisabledAccountCannotLogIn) {
    admin.setActive(adminId, stu, false);
    EXPECT_THROW(auth.login("stu1", "password5"), AuthError);
    admin.setActive(adminId, stu, true);
    EXPECT_NO_THROW(auth.login("stu1", "password5"));
}
TEST_F(World, ChangePassword) {
    EXPECT_THROW(auth.changePassword(stu, {"wrong", "newpassword1"}), AuthError);
    EXPECT_THROW(auth.changePassword(stu, {"password5", "short"}), ValidationError);
    EXPECT_THROW(auth.changePassword(stu, {"password5", "stu1"}), ValidationError);
    auth.changePassword(stu, {"password5", "newpassword1"});
    EXPECT_THROW(auth.login("stu1", "password5"), AuthError);
    EXPECT_NO_THROW(auth.login("stu1", "newpassword1"));
}

// --------------------------------------------------------------------- admin
TEST_F(World, AdminRulesAndValidation) {
    EXPECT_THROW(admin.createUser(lec, {Role::Student, "newone", "password1", 0}), PermissionError);
    EXPECT_THROW(admin.createUser(stu, {Role::Student, "newone", "password1", 0}), PermissionError);
    EXPECT_THROW(admin.createUser(adminId, {Role::Student, "stu1", "password1", 0}), RuleError);   // duplicate
    EXPECT_THROW(admin.createUser(adminId, {Role::Student, "ab", "password1", 0}), ValidationError);
    EXPECT_THROW(admin.createUser(adminId, {Role::Student, "bad name!", "password1", 0}), ValidationError);
    EXPECT_THROW(admin.createUser(adminId, {Role::Student, "okname", "short", 0}), ValidationError);
    EXPECT_THROW(admin.createUser(adminId, {Role::HeadOfDepartment, "hodx", "password1", 0}), NotFoundError);
    EXPECT_THROW(admin.createFirstAdministrator("again", "password1"), RuleError);
    EXPECT_THROW(admin.setActive(adminId, adminId, false), RuleError);
    EXPECT_THROW(admin.assignLecturer(adminId, course, stu), ValidationError);
    EXPECT_THROW(admin.enrol(adminId, course, lec), ValidationError);
    EXPECT_THROW(admin.enrol(adminId, course, stu), RuleError);
    EXPECT_THROW(admin.enrol(adminId, 9999, stu), NotFoundError);
}
TEST_F(World, PasswordsAreStoredHashedNotPlain) {
    auto u = users.findById(stu);
    EXPECT_NE(u->getPasswordHash(), "password5");
}
TEST_F(World, AssignmentRulesAndAccess) {
    EXPECT_THROW(asgSvc.createAssignment(lec2, {course, "T", "", deadline, 10}), PermissionError);
    EXPECT_THROW(asgSvc.createAssignment(stu, {course, "T", "", deadline, 10}), PermissionError);
    EXPECT_THROW(asgSvc.createAssignment(lec, {course, "", "", deadline, 10}), ValidationError);
    EXPECT_THROW(asgSvc.createAssignment(lec, {course, "T", "", deadline, 0}), ValidationError);
    EXPECT_THROW(asgSvc.createAssignment(lec, {9999, "T", "", deadline, 10}), NotFoundError);
    auto list = asgSvc.listForStudent(stu);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_FALSE(list[0].status.has_value());
    auto u = users.findById(stu);
    EXPECT_THROW(asgSvc.listForCourse(*u, otherCourse), PermissionError);
}

// ----------------------------------------------------------------- database
TEST_F(World, AuditLogCannotBeEditedOrDeleted) {  // BR4
    int id = markOnTime(stu, 70);
    (void)id;
    EXPECT_THROW(db.exec("UPDATE audit_entry SET new_value = 99;"), DatabaseError);
    EXPECT_THROW(db.exec("DELETE FROM audit_entry;"), DatabaseError);
}
TEST_F(World, DatabaseRefusesReleaseWithoutFeedback) {
    int id = markOnTime(stu, 70, "");
    EXPECT_THROW(db.exec("UPDATE submission SET status='Released' WHERE id=" + std::to_string(id) + ";"), DatabaseError);
}
TEST_F(World, SqlInjectionTextIsJustText) {
    std::string evil = "x'); DROP TABLE user_account;--";
    EXPECT_EQ(users.findByUsername(evil), nullptr);
    EXPECT_NO_THROW(users.listByRole(Role::Student));
    EXPECT_THROW(admin.createUser(adminId, {Role::Student, evil, "password1", 0}), ValidationError);
}
TEST(DatabaseFile, DataSurvivesRestart) {  // persistence
    auto path = (std::filesystem::temp_directory_path() / "lms_test_persist.db").string();
    std::remove(path.c_str());
    {
        Database db(path);
        SqliteUserRepository users(db);
        FakeHasher h;
        UserSpec s; s.username = "keepme"; s.passwordHash = h.hash("password1"); s.role = Role::Lecturer;
        auto u = UserFactory::create(s);
        users.save(*u);
    }
    {
        Database db(path);
        SqliteUserRepository users(db);
        auto u = users.findByUsername("keepme");
        ASSERT_NE(u, nullptr);
        EXPECT_EQ(u->role(), Role::Lecturer);
    }
    std::remove(path.c_str());
}
TEST(DatabaseFile, BadPathGivesCleanError) {
    EXPECT_THROW(Database("/nonexistent_dir/x/lms.db"), DatabaseError);
}
TEST(PasswordHasher, SaltedArgon2id) {
    SodiumPasswordHasher h;
    auto a = h.hash("correct horse");
    auto b = h.hash("correct horse");
    EXPECT_NE(a, b);                       // random salt
    EXPECT_EQ(a.rfind("$argon2id$", 0), 0u);
    EXPECT_TRUE(h.verify("correct horse", a));
    EXPECT_FALSE(h.verify("wrong", a));
    EXPECT_FALSE(h.verify("correct horse", "garbage"));
}
