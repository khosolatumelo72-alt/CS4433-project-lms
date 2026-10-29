#include "services/AssignmentService.hpp"

#include "domain/Errors.hpp"
#include "services/Validation.hpp"

namespace lms {

int AssignmentService::createAssignment(int lecturerId, const NewAssignment& req) {
    if (!courses_.findById(req.courseId)) throw NotFoundError("Course not found");
    if (!courses_.isTeaching(req.courseId, lecturerId)) throw PermissionError("You do not teach this course");
    std::string title = validate::requireText(req.title, "Title", 100);
    std::string desc = validate::trim(req.description);
    if (desc.size() > 2000) throw ValidationError("Description is too long (maximum 2000 characters)");
    if (!(req.maxMark > 0) || req.maxMark > 1000) throw ValidationError("Maximum mark must be more than 0 and at most 1000");
    Assignment a(0, req.courseId, title, desc, req.deadline, req.maxMark);
    return assignments_.save(a);
}

void AssignmentService::grantExtension(int lecturerId, const NewExtension& req) {
    auto asg = assignments_.findById(req.assignmentId);
    if (!asg) throw NotFoundError("Assignment not found");
    if (!courses_.isTeaching(asg->getCourseId(), lecturerId)) throw PermissionError("You do not teach this course");
    if (!courses_.isEnrolled(asg->getCourseId(), req.studentId)) throw ValidationError("That student is not enrolled in this course");
    std::string reason = validate::requireText(req.reason, "Reason", 500);
    auto current = assignments_.findExtension(req.assignmentId, req.studentId);
    if (req.newDeadline <= effectiveDeadline(*asg, current))
        throw ValidationError("The new deadline must be later than the student's current deadline");
    Extension e(0, req.assignmentId, req.studentId, lecturerId, req.newDeadline, reason, clock_.now());
    assignments_.addExtension(e);
}

std::vector<Course> AssignmentService::myCourses(const User& user) {
    switch (user.role()) {
        case Role::Lecturer: return courses_.listForLecturer(user.getId());
        case Role::Student: return courses_.listForStudent(user.getId());
        case Role::HeadOfDepartment:
            return courses_.listByDepartment(static_cast<const HeadOfDepartment&>(user).getDepartmentId());
        case Role::Administrator: break;
    }
    return {};
}

std::vector<Assignment> AssignmentService::listForCourse(const User& user, int courseId) {
    bool ok = false;
    switch (user.role()) {
        case Role::Lecturer: ok = courses_.isTeaching(courseId, user.getId()); break;
        case Role::Student: ok = courses_.isEnrolled(courseId, user.getId()); break;
        case Role::HeadOfDepartment: {
            auto c = courses_.findById(courseId);
            ok = c && c->getDepartmentId() == static_cast<const HeadOfDepartment&>(user).getDepartmentId();
            break;
        }
        case Role::Administrator: break;
    }
    if (!ok) throw PermissionError("You cannot see this course");
    return assignments_.listByCourse(courseId);
}

std::vector<AssignmentView> AssignmentService::listForStudent(int studentId) {
    std::vector<AssignmentView> out;
    for (const auto& c : courses_.listForStudent(studentId))
        for (const auto& a : assignments_.listByCourse(c.getId())) {
            AssignmentView v{a, c.getCode(), std::nullopt};
            if (auto s = submissions_.find(a.getId(), studentId)) v.status = s->getStatus();
            out.push_back(std::move(v));
        }
    return out;
}

}  // namespace lms
