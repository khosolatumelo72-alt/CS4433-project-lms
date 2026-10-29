#include "infrastructure/SqliteSubmissionRepository.hpp"

namespace lms {
namespace {
const char* kSub = "SELECT id, assignment_id, student_id, status, first_submitted_at FROM submission ";
Submission fromRow(Statement& s) {
    return Submission(s.getInt(0), s.getInt(1), s.getInt(2), statusFromString(s.getText(3)), parseTime(s.getText(4)));
}
}  // namespace

std::optional<Submission> SqliteSubmissionRepository::findById(int id) {
    Statement s(db_, std::string(kSub) + "WHERE id = ?1;");
    s.bind(1, id);
    if (!s.step()) return std::nullopt;
    return fromRow(s);
}

std::optional<Submission> SqliteSubmissionRepository::find(int assignmentId, int studentId) {
    Statement s(db_, std::string(kSub) + "WHERE assignment_id = ?1 AND student_id = ?2;");
    s.bind(1, assignmentId).bind(2, studentId);
    if (!s.step()) return std::nullopt;
    return fromRow(s);
}

std::vector<Submission> SqliteSubmissionRepository::listByAssignment(int assignmentId) {
    Statement s(db_, std::string(kSub) + "WHERE assignment_id = ?1 ORDER BY id;");
    s.bind(1, assignmentId);
    std::vector<Submission> out;
    while (s.step()) out.push_back(fromRow(s));
    return out;
}

int SqliteSubmissionRepository::save(Submission& sub) {
    if (sub.getId() == 0) {
        Statement s(db_, "INSERT INTO submission(assignment_id, student_id, status, first_submitted_at) VALUES (?1,?2,?3,?4);");
        s.bind(1, sub.getAssignmentId()).bind(2, sub.getStudentId()).bind(3, toString(sub.getStatus()))
            .bind(4, formatTime(sub.getFirstSubmittedAt()));
        s.step();
        sub.setId(static_cast<int>(db_.lastInsertRowId()));
    } else {
        Statement s(db_, "UPDATE submission SET status = ?2 WHERE id = ?1;");
        s.bind(1, sub.getId()).bind(2, toString(sub.getStatus()));
        s.step();
    }
    return sub.getId();
}

int SqliteSubmissionRepository::addVersion(SubmissionVersion& v) {
    Statement s(db_,
                "INSERT INTO submission_version(submission_id, version_no, file_name, file_size, file_data, uploaded_at) "
                "VALUES (?1,?2,?3,?4,?5,?6);");
    s.bind(1, v.getSubmissionId()).bind(2, v.getVersionNo()).bind(3, v.getFileName()).bind(4, v.getFileSize())
        .bind(5, v.getContent()).bind(6, formatTime(v.getUploadedAt()));
    s.step();
    v.setId(static_cast<int>(db_.lastInsertRowId()));
    return v.getId();
}

std::optional<SubmissionVersion> SqliteSubmissionRepository::latestVersion(int submissionId) {
    Statement s(db_,
                "SELECT id, submission_id, version_no, file_name, file_size, file_data, uploaded_at "
                "FROM submission_version WHERE submission_id = ?1 ORDER BY version_no DESC LIMIT 1;");
    s.bind(1, submissionId);
    if (!s.step()) return std::nullopt;
    return SubmissionVersion(s.getInt(0), s.getInt(1), s.getInt(2), s.getText(3), s.getInt64(4), s.getBlob(5),
                             parseTime(s.getText(6)));
}
}  // namespace lms
