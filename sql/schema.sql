PRAGMA foreign_keys = ON;

CREATE TABLE department (
    id    INTEGER PRIMARY KEY,
    name  TEXT NOT NULL UNIQUE
);

CREATE TABLE user_account (
    id               INTEGER PRIMARY KEY,
    username         TEXT NOT NULL UNIQUE,
    password_hash    TEXT NOT NULL,
    role             TEXT NOT NULL
                     CHECK (role IN ('Student','Lecturer','HeadOfDepartment','Administrator')),
    department_id    INTEGER REFERENCES department(id),
    active           INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0,1)),
    failed_attempts  INTEGER NOT NULL DEFAULT 0 CHECK (failed_attempts >= 0),
    locked_until     TEXT,
    CHECK ((role = 'HeadOfDepartment' AND department_id IS NOT NULL)
        OR (role <> 'HeadOfDepartment' AND department_id IS NULL))
);

CREATE TABLE course (
    id             INTEGER PRIMARY KEY,
    code           TEXT NOT NULL UNIQUE,
    title          TEXT NOT NULL,
    department_id  INTEGER NOT NULL REFERENCES department(id)
);

CREATE TABLE course_lecturer (
    course_id    INTEGER NOT NULL REFERENCES course(id),
    lecturer_id  INTEGER NOT NULL REFERENCES user_account(id),
    PRIMARY KEY (course_id, lecturer_id)
);

CREATE TABLE enrolment (
    course_id   INTEGER NOT NULL REFERENCES course(id),
    student_id  INTEGER NOT NULL REFERENCES user_account(id),
    PRIMARY KEY (course_id, student_id)
);

CREATE TABLE assignment (
    id           INTEGER PRIMARY KEY,
    course_id    INTEGER NOT NULL REFERENCES course(id),
    title        TEXT NOT NULL,
    description  TEXT NOT NULL DEFAULT '',
    deadline     TEXT NOT NULL,
    max_mark     REAL NOT NULL CHECK (max_mark > 0)
);

CREATE TABLE extension (
    id            INTEGER PRIMARY KEY,
    assignment_id INTEGER NOT NULL REFERENCES assignment(id),
    student_id    INTEGER NOT NULL REFERENCES user_account(id),
    granted_by    INTEGER NOT NULL REFERENCES user_account(id),
    new_deadline  TEXT NOT NULL,
    reason        TEXT NOT NULL CHECK (length(trim(reason)) > 0),
    granted_at    TEXT NOT NULL
);

CREATE TABLE submission (
    id                  INTEGER PRIMARY KEY,
    assignment_id       INTEGER NOT NULL REFERENCES assignment(id),
    student_id          INTEGER NOT NULL REFERENCES user_account(id),
    status              TEXT NOT NULL
                        CHECK (status IN ('Submitted','Late','Marked','Moderated','Released')),
    first_submitted_at  TEXT NOT NULL,
    UNIQUE (assignment_id, student_id)
);

CREATE TABLE submission_version (
    id             INTEGER PRIMARY KEY,
    submission_id  INTEGER NOT NULL REFERENCES submission(id),
    version_no     INTEGER NOT NULL CHECK (version_no >= 1),
    file_name      TEXT NOT NULL,
    file_size      INTEGER NOT NULL CHECK (file_size BETWEEN 1 AND 10485760),
    file_data      BLOB NOT NULL,
    uploaded_at    TEXT NOT NULL,
    UNIQUE (submission_id, version_no)
);

CREATE TABLE mark (
    submission_id  INTEGER PRIMARY KEY REFERENCES submission(id),
    awarded_mark   REAL NOT NULL CHECK (awarded_mark >= 0),
    penalty        REAL NOT NULL DEFAULT 0 CHECK (penalty >= 0),
    final_mark     REAL NOT NULL CHECK (final_mark >= 0),
    feedback       TEXT NOT NULL DEFAULT '',
    marked_by      INTEGER NOT NULL REFERENCES user_account(id),
    marked_at      TEXT NOT NULL
);

CREATE TABLE audit_entry (
    id             INTEGER PRIMARY KEY,
    submission_id  INTEGER NOT NULL REFERENCES submission(id),
    actor_id       INTEGER NOT NULL REFERENCES user_account(id),
    at             TEXT NOT NULL,
    action         TEXT NOT NULL CHECK (action IN ('MarkSet','MarkChanged','Approved')),
    old_value      REAL,
    new_value      REAL,
    reason         TEXT NOT NULL DEFAULT '',
    CHECK (action <> 'MarkChanged' OR length(trim(reason)) > 0)
);

CREATE INDEX idx_course_department     ON course(department_id);
CREATE INDEX idx_enrolment_student     ON enrolment(student_id);
CREATE INDEX idx_assignment_course     ON assignment(course_id);
CREATE INDEX idx_extension_lookup      ON extension(assignment_id, student_id);
CREATE INDEX idx_submission_assignment ON submission(assignment_id);
CREATE INDEX idx_audit_submission      ON audit_entry(submission_id);

-- BR6: a submission cannot be Released unless its mark has feedback.
CREATE TRIGGER trg_release_needs_feedback
BEFORE UPDATE OF status ON submission
WHEN NEW.status = 'Released'
 AND NOT EXISTS (SELECT 1 FROM mark
                 WHERE submission_id = NEW.id AND length(trim(feedback)) > 0)
BEGIN
    SELECT RAISE(ABORT, 'BR6: feedback is required before release');
END;

-- BR4: the audit log can only grow.
CREATE TRIGGER trg_audit_no_update BEFORE UPDATE ON audit_entry
BEGIN SELECT RAISE(ABORT, 'audit_entry is append-only'); END;
CREATE TRIGGER trg_audit_no_delete BEFORE DELETE ON audit_entry
BEGIN SELECT RAISE(ABORT, 'audit_entry is append-only'); END;
