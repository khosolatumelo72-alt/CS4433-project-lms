#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"
#include "services/Dtos.hpp"

namespace lms {
class AdminService {
public:
    AdminService(IUserRepository& users, ICourseRepository& courses, IPasswordHasher& hasher, Database& db)
        : users_(users), courses_(courses), hasher_(hasher), db_(db) {}
    /// First-run set-up: creates an Administrator when none exists yet. Throws RuleError otherwise.
    int createFirstAdministrator(const std::string& username, const std::string& password);

    int createDepartment(int adminId, const std::string& name);
    int createUser(int adminId, const NewUserRequest& req);       // FR04
    void setActive(int adminId, int userId, bool active);         // FR04
    int createCourse(int adminId, const NewCourse& req);
    void assignLecturer(int adminId, int courseId, int lecturerId);  // FR05
    void enrol(int adminId, int courseId, int studentId);            // FR05
    std::vector<UserPtr> listUsers(int adminId);
    std::vector<Department> listDepartments(int adminId);
    std::vector<Course> listCourses(int adminId);
private:
    void requireAdmin(int adminId);
    IUserRepository& users_;
    ICourseRepository& courses_;
    IPasswordHasher& hasher_;
    Database& db_;
};
}  // namespace lms
