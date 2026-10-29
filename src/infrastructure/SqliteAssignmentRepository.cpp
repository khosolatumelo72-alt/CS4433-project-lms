#include "infrastructure/SqliteAssignmentRepository.hpp"

namespace lms {
namespace {
const char* kAsg = "SELECT id, course_id, title, description, deadline, max_mark FROM assignment ";
Assignment fromRow(Statement& s) {
    return Assignment(s.getInt(0), s.getInt(1), s.getText(2), s.getText(3), parseTime(s.getText(4)), s.getDouble(5));
}
}  // namespace

std::optional<Assignment> SqliteAssignmentRepository::findById(int id) {
    Statement s(db_, std::string(kAsg) + "WHERE id = ?1;");
    s.bind(1, id);
    if (!s.step()) return std::nullopt;
    return fromRow(s);
}

std::vector<Assignment> SqliteAssignmentRepository::listByCourse(int courseId) {
    Statement s(db_, std::string(kAsg) + "WHERE course_id = ?1 ORDER BY deadline, id;");
    s.bind(1, courseId);
    std::vector<Assignment> out;
    while (s.step()) out.push_back(fromRow(s));
    return out;
}

int SqliteAssignmentRepository::save(Assignment& a) {
    Statement s(db_, "INSERT INTO assignment(course_id, title, description, deadline, max_mark) VALUES (?1,?2,?3,?4,?5);");
    s.bind(1, a.getCourseId()).bind(2, a.getTitle()).bind(3, a.getDescription())
        .bind(4, formatTime(a.getDeadline())).bind(5, a.getMaxMark());
    s.step();
    a.setId(static_cast<int>(db_.lastInsertRowId()));
    return a.getId();
}

int SqliteAssignmentRepository::addExtension(Extension& e) {
    Statement s(db_,
                "INSERT INTO extension(assignment_id, student_id, granted_by, new_deadline, reason, granted_at) "
                "VALUES (?1,?2,?3,?4,?5,?6);");
    s.bind(1, e.getAssignmentId()).bind(2, e.getStudentId()).bind(3, e.getGrantedBy())
        .bind(4, formatTime(e.getNewDeadline())).bind(5, e.getReason()).bind(6, formatTime(e.getGrantedAt()));
    s.step();
    e.setId(static_cast<int>(db_.lastInsertRowId()));
    return e.getId();
}

std::optional<Extension> SqliteAssignmentRepository::findExtension(int assignmentId, int studentId) {
    Statement s(db_,
                "SELECT id, assignment_id, student_id, granted_by, new_deadline, reason, granted_at FROM extension "
                "WHERE assignment_id = ?1 AND student_id = ?2 ORDER BY granted_at DESC, id DESC LIMIT 1;");
    s.bind(1, assignmentId).bind(2, studentId);
    if (!s.step()) return std::nullopt;
    return Extension(s.getInt(0), s.getInt(1), s.getInt(2), s.getInt(3), parseTime(s.getText(4)), s.getText(5),
                     parseTime(s.getText(6)));
}
}  // namespace lms
