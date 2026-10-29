#pragma once
#include <memory>
#include <optional>
#include <vector>

#include "domain/Entities.hpp"
#include "domain/User.hpp"

namespace lms {

// Interfaces (ports) from Class diagram, Figure C. Services use only these.

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    /// Returns nullptr when there is no such user.
    virtual UserPtr findById(int id) = 0;
    virtual UserPtr findByUsername(const std::string& name) = 0;
    /// Inserts when user.getId() == 0 (and sets the new id), otherwise updates. Returns the id.
    virtual int save(User& user) = 0;
    virtual std::vector<UserPtr> listByRole(Role role) = 0;
};

class ICourseRepository {
public:
    virtual ~ICourseRepository() = default;
    virtual std::optional<Department> findDepartment(int id) = 0;
    virtual std::vector<Department> listDepartments() = 0;
    virtual int saveDepartment(Department& d) = 0;
    virtual std::optional<Course> findById(int id) = 0;
    virtual std::vector<Course> listByDepartment(int departmentId) = 0;
    virtual std::vector<Course> listForLecturer(int userId) = 0;
    virtual std::vector<Course> listForStudent(int userId) = 0;
    virtual std::vector<int> listStudentIds(int courseId) = 0;
    virtual bool isTeaching(int courseId, int userId) = 0;
    virtual bool isEnrolled(int courseId, int userId) = 0;
    virtual int save(Course& c) = 0;
    virtual void addLecturer(int courseId, int lecturerId) = 0;
    virtual void addEnrolment(int courseId, int studentId) = 0;
};

class IAssignmentRepository {
public:
    virtual ~IAssignmentRepository() = default;
    virtual std::optional<Assignment> findById(int id) = 0;
    virtual std::vector<Assignment> listByCourse(int courseId) = 0;
    virtual int save(Assignment& a) = 0;
    virtual int addExtension(Extension& e) = 0;
    /// The latest extension granted to this student for this assignment, if any.
    virtual std::optional<Extension> findExtension(int assignmentId, int studentId) = 0;
};

class ISubmissionRepository {
public:
    virtual ~ISubmissionRepository() = default;
    virtual std::optional<Submission> findById(int id) = 0;
    virtual std::optional<Submission> find(int assignmentId, int studentId) = 0;
    virtual std::vector<Submission> listByAssignment(int assignmentId) = 0;
    virtual int save(Submission& s) = 0;
    virtual int addVersion(SubmissionVersion& v) = 0;
    virtual std::optional<SubmissionVersion> latestVersion(int submissionId) = 0;
};

class IMarkRepository {
public:
    virtual ~IMarkRepository() = default;
    virtual std::optional<Mark> find(int submissionId) = 0;
    /// Inserts or replaces the mark of that submission.
    virtual void save(const Mark& m) = 0;
    virtual int addAudit(AuditEntry& e) = 0;
    virtual std::vector<AuditEntry> listAudit(int submissionId) = 0;
};

class IClock {
public:
    virtual ~IClock() = default;
    virtual TimePoint now() = 0;
};

class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;
    virtual std::string hash(const std::string& plain) = 0;
    virtual bool verify(const std::string& plain, const std::string& hash) = 0;
};

/// Strategy pattern: the late-penalty rule (BR01).
class IPenaltyPolicy {
public:
    virtual ~IPenaltyPolicy() = default;
    /// Marks to take off, from 0 up to maxMark. lateBy <= 0 means on time.
    virtual double penaltyFor(double maxMark, Duration lateBy) = 0;
};

class NoPenaltyPolicy final : public IPenaltyPolicy {
public:
    double penaltyFor(double, Duration) override { return 0.0; }
};

/// BR01: 10% of the maximum mark for each started 24 hours late, up to 3 days; after that 0 marks.
class TenPercentPerDayPolicy final : public IPenaltyPolicy {
public:
    double penaltyFor(double maxMark, Duration lateBy) override;
};

}  // namespace lms
