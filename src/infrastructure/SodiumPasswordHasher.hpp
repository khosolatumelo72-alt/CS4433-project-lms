#pragma once
#include "domain/Ports.hpp"
namespace lms {
/// Argon2id with a random salt per password, through libsodium (NFR4).
class SodiumPasswordHasher final : public IPasswordHasher {
public:
    SodiumPasswordHasher();
    std::string hash(const std::string& plain) override;
    bool verify(const std::string& plain, const std::string& hash) override;
};
}  // namespace lms
