#include "domain/Entities.hpp"

#include <algorithm>
#include <cctype>

#include "domain/Errors.hpp"

namespace lms {

TimePoint effectiveDeadline(const Assignment& a, const std::optional<Extension>& ext) {
    if (ext) return std::max(a.getDeadline(), ext->getNewDeadline());
    return a.getDeadline();
}

bool Submission::isAllowed(SubmissionStatus from, SubmissionStatus to) {
    using S = SubmissionStatus;
    switch (from) {
        case S::Submitted:
        case S::Late: return to == S::Marked;
        case S::Marked: return to == S::Moderated;
        case S::Moderated: return to == S::Released;
        case S::Released: return false;
    }
    return false;
}

void Submission::advanceTo(SubmissionStatus next) {
    if (!isAllowed(status_, next))
        throw RuleError("a submission cannot go from " + toString(status_) + " to " + toString(next));
    status_ = next;
}

bool Mark::hasFeedback() const {
    return std::any_of(feedback_.begin(), feedback_.end(),
                       [](unsigned char c) { return !std::isspace(c); });
}

}  // namespace lms
