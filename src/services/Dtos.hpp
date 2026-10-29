#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "domain/Entities.hpp"

namespace lms {

// Plain request/result structs (not stored in the database).
struct NewUserRequest { Role role = Role::Student; std::string username; std::string password; int departmentId = 0; };
struct NewCourse { std::string code; std::string title; int departmentId = 0; };
struct NewAssignment { int courseId = 0; std::string title; std::string description; TimePoint deadline{}; double maxMark = 0; };
struct NewExtension { int assignmentId = 0; int studentId = 0; TimePoint newDeadline{}; std::string reason; };
struct NewPassword { std::string oldPassword; std::string newPassword; };

struct FileData { std::string fileName; std::vector<std::uint8_t> content; };
struct SubmitRequest { int studentId = 0; int assignmentId = 0; FileData file; };
struct SubmitResult {
    int submissionId = 0;
    int versionNo = 0;
    SubmissionStatus status = SubmissionStatus::Submitted;
    TimePoint effectiveDeadline{};
    TimePoint submittedAt{};
    bool isResubmission = false;
};

struct AssignmentView {
    Assignment assignment;
    std::string courseCode;
    std::optional<SubmissionStatus> status;  // empty = not submitted yet
};

struct SubmissionView {
    std::string assignmentTitle;
    TimePoint effectiveDeadline{};
    std::optional<SubmissionStatus> status;     // empty = nothing submitted
    int versions = 0;
    std::optional<Mark> mark;                   // filled only when the status is Released (BR2)
};

struct MarkRequest { int lecturerId = 0; int submissionId = 0; double awardedMark = 0; std::string feedback; };
struct MarkChange { int submissionId = 0; double newFinalMark = 0; std::string reason; };

struct ReleaseReport {
    std::vector<int> released;
    std::vector<std::pair<int, std::string>> skipped;  // submission id and the reason
};

/// One row of a lecturer's or the HoD's list.
struct SubmissionRow {
    Submission submission;
    std::string studentName;
    std::optional<double> finalMark;
    bool hasFeedback = false;
};

}  // namespace lms
