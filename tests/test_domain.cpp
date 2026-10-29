#include <gtest/gtest.h>

#include "domain/Errors.hpp"
#include "domain/Ports.hpp"

using namespace lms;
using std::chrono::hours;
using std::chrono::seconds;

TEST(TimeText, RoundTrip) {
    auto t = parseTime("2026-11-01T12:00:00Z");
    EXPECT_EQ(formatTime(t), "2026-11-01T12:00:00Z");
    EXPECT_EQ(displayTime(t), "2026-11-01 12:00 UTC");
}
TEST(TimeText, RejectsBadStoredText) {
    EXPECT_THROW(parseTime("tomorrow"), ValidationError);
    EXPECT_THROW(parseTime("2026-02-31T10:00:00Z"), ValidationError);
    EXPECT_THROW(parseTime("2026-11-01T12:00:00"), ValidationError);
}
TEST(TimeText, UserInput) {
    EXPECT_TRUE(parseUserTime("2026-11-30 23:59").has_value());
    EXPECT_FALSE(parseUserTime("2026-11-30").has_value());
    EXPECT_FALSE(parseUserTime("2026-13-01 10:00").has_value());
    EXPECT_FALSE(parseUserTime("2026-02-30 10:00").has_value());
    EXPECT_FALSE(parseUserTime("2026-11-30 24:00").has_value());
    EXPECT_FALSE(parseUserTime("2026-11-30 10:00 junk").has_value());
}
TEST(TimeText, LeapYearsAndKnownInstants) {
    EXPECT_TRUE(parseUserTime("2028-02-29 12:00").has_value());
    EXPECT_FALSE(parseUserTime("2027-02-29 12:00").has_value());
    EXPECT_FALSE(parseUserTime("2100-02-29 12:00").has_value());
    EXPECT_TRUE(parseUserTime("2000-02-29 12:00").has_value());
    EXPECT_EQ(formatTime(TimePoint(std::chrono::seconds(0))), "1970-01-01T00:00:00Z");
    EXPECT_EQ(formatTime(TimePoint(std::chrono::seconds(1000000000))), "2001-09-09T01:46:40Z");
    EXPECT_EQ(formatTime(parseTime("2028-02-29T23:59:59Z")), "2028-02-29T23:59:59Z");
}
TEST(Enums, RoundTripAndReject) {
    for (Role r : {Role::Student, Role::Lecturer, Role::HeadOfDepartment, Role::Administrator})
        EXPECT_EQ(roleFromString(toString(r)), r);
    EXPECT_THROW(roleFromString("Dean"), ValidationError);
    EXPECT_THROW(statusFromString("Done"), ValidationError);
    EXPECT_THROW(actionFromString("Delete"), ValidationError);
}

// BR01: boundary values for the late penalty.
struct PenaltyCase { long long lateSeconds; double expected; };
class PenaltyTest : public ::testing::TestWithParam<PenaltyCase> {};
TEST_P(PenaltyTest, TenPercentPerStartedDay) {
    TenPercentPerDayPolicy p;
    EXPECT_DOUBLE_EQ(p.penaltyFor(100.0, seconds(GetParam().lateSeconds)), GetParam().expected);
}
INSTANTIATE_TEST_SUITE_P(Boundaries, PenaltyTest, ::testing::Values(
    PenaltyCase{-3600, 0}, PenaltyCase{0, 0}, PenaltyCase{1, 10}, PenaltyCase{86400, 10},
    PenaltyCase{86401, 20}, PenaltyCase{2 * 86400, 20}, PenaltyCase{2 * 86400 + 1, 30},
    PenaltyCase{3 * 86400, 30}, PenaltyCase{3 * 86400 + 1, 100}, PenaltyCase{30 * 86400, 100}));

TEST(Penalty, ScalesWithMaximumMark) {
    TenPercentPerDayPolicy p;
    EXPECT_DOUBLE_EQ(p.penaltyFor(50.0, hours(30)), 10.0);
}
TEST(Penalty, NoPenaltyPolicyAlwaysZero) {
    NoPenaltyPolicy p;
    EXPECT_DOUBLE_EQ(p.penaltyFor(100.0, hours(500)), 0.0);
}

TEST(Submission, AllowedTransitionsFollowStateDiagram) {
    using S = SubmissionStatus;
    TimePoint t = parseTime("2026-11-01T12:00:00Z");
    Submission a(1, 1, 1, S::Submitted, t);
    EXPECT_THROW(a.advanceTo(S::Released), RuleError);
    EXPECT_THROW(a.advanceTo(S::Moderated), RuleError);
    a.advanceTo(S::Marked);
    EXPECT_THROW(a.advanceTo(S::Released), RuleError);  // must be Moderated first (BR3)
    a.advanceTo(S::Moderated);
    EXPECT_FALSE(a.isReleased());
    a.advanceTo(S::Released);
    EXPECT_TRUE(a.isReleased());
    EXPECT_THROW(a.advanceTo(S::Marked), RuleError);
    Submission late(2, 1, 2, S::Late, t);
    late.advanceTo(S::Marked);
    EXPECT_EQ(late.getStatus(), S::Marked);
}

TEST(EffectiveDeadline, ExtensionOnlyHelpsNeverShortens) {
    TimePoint d = parseTime("2026-11-01T12:00:00Z");
    Assignment a(1, 1, "A", "", d, 100);
    EXPECT_EQ(effectiveDeadline(a, std::nullopt), d);
    Extension later(1, 1, 1, 2, d + hours(48), "ill", d);
    EXPECT_EQ(effectiveDeadline(a, later), d + hours(48));
    Extension earlier(2, 1, 1, 2, d - hours(5), "x", d);
    EXPECT_EQ(effectiveDeadline(a, earlier), d);
}

TEST(UserModel, PolymorphicRolesAndFactory) {
    UserSpec s; s.username = "x1y"; s.passwordHash = "h";
    for (Role r : {Role::Student, Role::Lecturer, Role::Administrator}) {
        s.role = r;
        EXPECT_EQ(UserFactory::create(s)->role(), r);
    }
    s.role = Role::HeadOfDepartment;
    EXPECT_THROW(UserFactory::create(s), ValidationError);
    s.departmentId = 3;
    auto hod = UserFactory::create(s);
    EXPECT_EQ(hod->role(), Role::HeadOfDepartment);
    EXPECT_EQ(static_cast<HeadOfDepartment&>(*hod).getDepartmentId(), 3);
    s.username = "";
    EXPECT_THROW(UserFactory::create(s), ValidationError);
}

TEST(UserModel, LocksAfterFiveFailures) {
    UserSpec s; s.username = "stu1"; s.passwordHash = "h"; s.role = Role::Student;
    auto u = UserFactory::create(s);
    TimePoint now = parseTime("2026-11-01T12:00:00Z");
    for (int i = 0; i < User::kMaxFailedAttempts - 1; ++i) u->registerFailedLogin(now);
    EXPECT_FALSE(u->isLocked(now));
    u->registerFailedLogin(now);
    EXPECT_TRUE(u->isLocked(now));
    EXPECT_TRUE(u->isLocked(now + std::chrono::minutes(User::kLockMinutes) - seconds(1)));
    EXPECT_FALSE(u->isLocked(now + std::chrono::minutes(User::kLockMinutes)));
    u->resetFailedLogins();
    EXPECT_FALSE(u->isLocked(now));
}

TEST(MarkModel, FeedbackMustHaveVisibleText) {
    Mark m(1, 50, 0, 50, "  \t\n", 2, parseTime("2026-11-01T12:00:00Z"));
    EXPECT_FALSE(m.hasFeedback());
    Mark n(1, 50, 0, 50, "Good work", 2, parseTime("2026-11-01T12:00:00Z"));
    EXPECT_TRUE(n.hasFeedback());
}
