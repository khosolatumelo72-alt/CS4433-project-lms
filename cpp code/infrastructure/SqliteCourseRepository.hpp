#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"

namespace lms {
class SqliteCourseRepository final : public ICourseRepository {
public:
    explicit SqliteCourseRepository(Database& db) : db_(db) {}
    std::optional<Department> findDepartment(int id) override;
    std::vector<Department> listDepartments() override;
    int saveDepartment(Department& d) override;
    std::optional<Course> findById(int id) override;
    std::vector<Course> listByDepartment(int departmentId) override;
    std::vector<Course> listForLecturer(int userId) override;
    std::vector<Course> listForStudent(int userId) override;
    std::vector<int> listStudentIds(int courseId) override;
    bool isTeaching(int courseId, int userId) override;
    bool isEnrolled(int courseId, int userId) override;
    int save(Course& c) override;
    void addLecturer(int courseId, int lecturerId) override;
    void addEnrolment(int courseId, int studentId) override;
private:
    Database& db_;
};
}  // namespace lms
