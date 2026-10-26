#include "infrastructure/SodiumPasswordHasher.hpp"

#include <sodium.h>

#include "domain/Errors.hpp"

namespace lms {

SodiumPasswordHasher::SodiumPasswordHasher() {
    if (sodium_init() < 0) throw LmsError("libsodium could not be started");
}

std::string SodiumPasswordHasher::hash(const std::string& plain) {
    char out[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str_alg(out, plain.c_str(), plain.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
                              crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_ARGON2ID13) != 0)
        throw LmsError("password hashing failed (out of memory)");
    return std::string(out);
}

bool SodiumPasswordHasher::verify(const std::string& plain, const std::string& hash) {
    return crypto_pwhash_str_verify(hash.c_str(), plain.c_str(), plain.size()) == 0;
}

}  // namespace lms
