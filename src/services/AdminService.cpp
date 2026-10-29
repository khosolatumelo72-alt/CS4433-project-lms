#include "services/AdminService.hpp"

#include "domain/Errors.hpp"
#include "services/Validation.hpp"

namespace lms {

void AdminService::requireAdmin(int adminId) {
    UserPtr u = users_.findById(adminId);
    if (!u || u->role() != Role::Administrator || !u->isActive())
        throw PermissionError("Only an administrator can do this");
}

int AdminService::createFirstAdministrator(const std::string& username, const std::string& password) {
    if (!users_.listByRole(Role::Administrator).empty()) throw RuleError("An administrator already exists");
    std::string name = validate::requireUsername(username);
    validate::requirePassword(password, name);
    UserSpec spec;
    spec.role = Role::Administrator;
    spec.username = name;
    spec.passwordHash = hasher_.hash(password);
    UserPtr u = UserFactory::create(spec);
    return users_.save(*u);
}

int AdminService::createDepartment(int adminId, const std::string& name) {
    requireAdmin(adminId);
    Department d(0, validate::requireText(name, "Department name", 100));
    return courses_.saveDepartment(d);
}

int AdminService::createUser(int adminId, const NewUserRequest& req) {
    requireAdmin(adminId);
    std::string name = validate::requireUsername(req.username);
    validate::requirePassword(req.password, name);
    if (users_.findByUsername(name)) throw RuleError("That user name is already taken");
    UserSpec spec;
    spec.role = req.role;
    spec.username = name;
    if (req.role == Role::HeadOfDepartment) {
        if (!courses_.findDepartment(req.departmentId)) throw NotFoundError("Department not found");
        spec.departmentId = req.departmentId;
    }
    spec.passwordHash = hasher_.hash(req.password);
    UserPtr u = UserFactory::create(spec);
    return users_.save(*u);
}

void AdminService::setActive(int adminId, int userId, bool active) {
    requireAdmin(adminId);
    if (adminId == userId && !active) throw RuleError("You cannot disable your own account");
    UserPtr u = users_.findById(userId);
    if (!u) throw NotFoundError("User not found");
    u->setActive(active);
    if (active) u->resetFailedLogins();
    users_.save(*u);
}

int AdminService::createCourse(int adminId, const NewCourse& req) {
    requireAdmin(adminId);
    std::string code = validate::requireText(req.code, "Course code", 20);
    std::string title = validate::requireText(req.title, "Course title", 100);
    if (!courses_.findDepartment(req.departmentId)) throw NotFoundError("Department not found");
    Course c(0, code, title, req.departmentId);
    return courses_.save(c);
}

void AdminService::assignLecturer(int adminId, int courseId, int lecturerId) {
    requireAdmin(adminId);
    if (!courses_.findById(courseId)) throw NotFoundError("Course not found");
    UserPtr u = users_.findById(lecturerId);
    if (!u || u->role() != Role::Lecturer) throw ValidationError("That user is not a lecturer");
    if (courses_.isTeaching(courseId, lecturerId)) throw RuleError("That lecturer already teaches this course");
    courses_.addLecturer(courseId, lecturerId);
}

void AdminService::enrol(int adminId, int courseId, int studentId) {
    requireAdmin(adminId);
    if (!courses_.findById(courseId)) throw NotFoundError("Course not found");
    UserPtr u = users_.findById(studentId);
    if (!u || u->role() != Role::Student) throw ValidationError("That user is not a student");
    if (courses_.isEnrolled(courseId, studentId)) throw RuleError("That student is already enrolled");
    courses_.addEnrolment(courseId, studentId);
}

std::vector<UserPtr> AdminService::listUsers(int adminId) {
    requireAdmin(adminId);
    std::vector<UserPtr> all;
    for (Role r : {Role::Administrator, Role::HeadOfDepartment, Role::Lecturer, Role::Student})
        for (auto& u : users_.listByRole(r)) all.push_back(std::move(u));
    return all;
}

std::vector<Department> AdminService::listDepartments(int adminId) {
    requireAdmin(adminId);
    return courses_.listDepartments();
}

std::vector<Course> AdminService::listCourses(int adminId) {
    requireAdmin(adminId);
    std::vector<Course> out;
    for (const auto& d : courses_.listDepartments())
        for (auto& c : courses_.listByDepartment(d.getId())) out.push_back(std::move(c));
    return out;
}

}  // namespace lms
