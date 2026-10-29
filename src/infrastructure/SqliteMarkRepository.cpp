#include "infrastructure/SqliteMarkRepository.hpp"

namespace lms {

std::optional<Mark> SqliteMarkRepository::find(int submissionId) {
    Statement s(db_,
                "SELECT submission_id, awarded_mark, penalty, final_mark, feedback, marked_by, marked_at "
                "FROM mark WHERE submission_id = ?1;");
    s.bind(1, submissionId);
    if (!s.step()) return std::nullopt;
    return Mark(s.getInt(0), s.getDouble(1), s.getDouble(2), s.getDouble(3), s.getText(4), s.getInt(5),
                parseTime(s.getText(6)));
}

void SqliteMarkRepository::save(const Mark& m) {
    Statement s(db_,
                "INSERT INTO mark(submission_id, awarded_mark, penalty, final_mark, feedback, marked_by, marked_at) "
                "VALUES (?1,?2,?3,?4,?5,?6,?7) "
                "ON CONFLICT(submission_id) DO UPDATE SET awarded_mark=excluded.awarded_mark, "
                "penalty=excluded.penalty, final_mark=excluded.final_mark, feedback=excluded.feedback, "
                "marked_by=excluded.marked_by, marked_at=excluded.marked_at;");
    s.bind(1, m.getSubmissionId()).bind(2, m.getAwardedMark()).bind(3, m.getPenalty()).bind(4, m.getFinalMark())
        .bind(5, m.getFeedback()).bind(6, m.getMarkedBy()).bind(7, formatTime(m.getMarkedAt()));
    s.step();
}

int SqliteMarkRepository::addAudit(AuditEntry& e) {
    Statement s(db_,
                "INSERT INTO audit_entry(submission_id, actor_id, at, action, old_value, new_value, reason) "
                "VALUES (?1,?2,?3,?4,?5,?6,?7);");
    s.bind(1, e.getSubmissionId()).bind(2, e.getActorId()).bind(3, formatTime(e.getAt())).bind(4, toString(e.getAction()));
    if (e.getOldValue()) s.bind(5, *e.getOldValue()); else s.bindNull(5);
    if (e.getNewValue()) s.bind(6, *e.getNewValue()); else s.bindNull(6);
    s.bind(7, e.getReason());
    s.step();
    e.setId(static_cast<int>(db_.lastInsertRowId()));
    return e.getId();
}

std::vector<AuditEntry> SqliteMarkRepository::listAudit(int submissionId) {
    Statement s(db_,
                "SELECT id, submission_id, actor_id, at, action, old_value, new_value, reason "
                "FROM audit_entry WHERE submission_id = ?1 ORDER BY id;");
    s.bind(1, submissionId);
    std::vector<AuditEntry> out;
    while (s.step())
        out.emplace_back(s.getInt(0), s.getInt(1), s.getInt(2), parseTime(s.getText(3)), actionFromString(s.getText(4)),
                         s.isNull(5) ? std::nullopt : std::optional<double>(s.getDouble(5)),
                         s.isNull(6) ? std::nullopt : std::optional<double>(s.getDouble(6)), s.getText(7));
    return out;
}
}  // namespace lms
