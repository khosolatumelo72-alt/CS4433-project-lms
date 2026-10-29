#include "infrastructure/SqliteCourseRepository.hpp"

namespace lms {
namespace {
Course courseFromRow(Statement& s) { return Course(s.getInt(0), s.getText(1), s.getText(2), s.getInt(3)); }
const char* kCourse = "SELECT c.id, c.code, c.title, c.department_id FROM course c ";
}  // namespace

std::optional<Department> SqliteCourseRepository::findDepartment(int id) {
    Statement s(db_, "SELECT id, name FROM department WHERE id = ?1;");
    s.bind(1, id);
    if (!s.step()) return std::nullopt;
    return Department(s.getInt(0), s.getText(1));
}

std::vector<Department> SqliteCourseRepository::listDepartments() {
    Statement s(db_, "SELECT id, name FROM department ORDER BY name;");
    std::vector<Department> out;
    while (s.step()) out.emplace_back(s.getInt(0), s.getText(1));
    return out;
}

int SqliteCourseRepository::saveDepartment(Department& d) {
    Statement s(db_, "INSERT INTO department(name) VALUES (?1);");
    s.bind(1, d.getName());
    s.step();
    d.setId(static_cast<int>(db_.lastInsertRowId()));
    return d.getId();
}

std::optional<Course> SqliteCourseRepository::findById(int id) {
    Statement s(db_, std::string(kCourse) + "WHERE c.id = ?1;");
    s.bind(1, id);
    if (!s.step()) return std::nullopt;
    return courseFromRow(s);
}

std::vector<Course> SqliteCourseRepository::listByDepartment(int departmentId) {
    Statement s(db_, std::string(kCourse) + "WHERE c.department_id = ?1 ORDER BY c.code;");
    s.bind(1, departmentId);
    std::vector<Course> out;
    while (s.step()) out.push_back(courseFromRow(s));
    return out;
}

std::vector<Course> SqliteCourseRepository::listForLecturer(int userId) {
    Statement s(db_, std::string(kCourse) + "JOIN course_lecturer cl ON cl.course_id = c.id WHERE cl.lecturer_id = ?1 ORDER BY c.code;");
    s.bind(1, userId);
    std::vector<Course> out;
    while (s.step()) out.push_back(courseFromRow(s));
    return out;
}

std::vector<Course> SqliteCourseRepository::listForStudent(int userId) {
    Statement s(db_, std::string(kCourse) + "JOIN enrolment e ON e.course_id = c.id WHERE e.student_id = ?1 ORDER BY c.code;");
    s.bind(1, userId);
    std::vector<Course> out;
    while (s.step()) out.push_back(courseFromRow(s));
    return out;
}

std::vector<int> SqliteCourseRepository::listStudentIds(int courseId) {
    Statement s(db_, "SELECT student_id FROM enrolment WHERE course_id = ?1 ORDER BY student_id;");
    s.bind(1, courseId);
    std::vector<int> out;
    while (s.step()) out.push_back(s.getInt(0));
    return out;
}

bool SqliteCourseRepository::isTeaching(int courseId, int userId) {
    Statement s(db_, "SELECT 1 FROM course_lecturer WHERE course_id = ?1 AND lecturer_id = ?2;");
    s.bind(1, courseId).bind(2, userId);
    return s.step();
}

bool SqliteCourseRepository::isEnrolled(int courseId, int userId) {
    Statement s(db_, "SELECT 1 FROM enrolment WHERE course_id = ?1 AND student_id = ?2;");
    s.bind(1, courseId).bind(2, userId);
    return s.step();
}

int SqliteCourseRepository::save(Course& c) {
    Statement s(db_, "INSERT INTO course(code, title, department_id) VALUES (?1,?2,?3);");
    s.bind(1, c.getCode()).bind(2, c.getTitle()).bind(3, c.getDepartmentId());
    s.step();
    c.setId(static_cast<int>(db_.lastInsertRowId()));
    return c.getId();
}

void SqliteCourseRepository::addLecturer(int courseId, int lecturerId) {
    Statement s(db_, "INSERT INTO course_lecturer(course_id, lecturer_id) VALUES (?1,?2);");
    s.bind(1, courseId).bind(2, lecturerId);
    s.step();
}

void SqliteCourseRepository::addEnrolment(int courseId, int studentId) {
    Statement s(db_, "INSERT INTO enrolment(course_id, student_id) VALUES (?1,?2);");
    s.bind(1, courseId).bind(2, studentId);
    s.step();
}
}  // namespace lms
