#pragma once
#include "domain/Ports.hpp"
namespace lms {
class SystemClock final : public IClock {
public:
    TimePoint now() override;
};
}  // namespace lms
