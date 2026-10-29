#pragma once
#include "domain/Ports.hpp"
#include "services/Dtos.hpp"

namespace lms {
class AssignmentService {
public:
    AssignmentService(IAssignmentRepository& assignments, ICourseRepository& courses,
                      ISubmissionRepository& submissions, IClock& clock)
        : assignments_(assignments), courses_(courses), submissions_(submissions), clock_(clock) {}
    int createAssignment(int lecturerId, const NewAssignment& req);   // FR06
    void grantExtension(int lecturerId, const NewExtension& req);     // FR13
    std::vector<Course> myCourses(const User& user);                  // FR07
    std::vector<Assignment> listForCourse(const User& user, int courseId);
    std::vector<AssignmentView> listForStudent(int studentId);        // FR07
private:
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    ISubmissionRepository& submissions_;
    IClock& clock_;
};
}  // namespace lms
