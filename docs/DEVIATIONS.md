# Deviations from the Phase 3 design (to record in the report)
1. Admin and lecturer service methods take the acting user's id first, so authorisation is checked in the service.
2. `ICourseRepository` gained departments, `listByDepartment`, `listStudentIds`, `addLecturer`, `addEnrolment`.
3. `AdminService` gained `createDepartment` and `createFirstAdministrator`.
4. `SubmissionService` also depends on `IUserRepository` and `IMarkRepository`; `ModerationService` on `IAssignmentRepository` and `ICourseRepository`; `MarkingService` on `IUserRepository`.
5. Polymorphic `findById` returns a `unique_ptr<User>` (null if absent).
6. `submit` takes a `SubmitRequest`; refusals throw `RuleError`.
7. User subclasses have explicit public constructors; the base constructor is protected so `User` stays abstract.
