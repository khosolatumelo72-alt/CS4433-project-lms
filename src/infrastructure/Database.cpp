#include "infrastructure/Database.hpp"

#include <sqlite3.h>

#include "domain/Errors.hpp"
#include "infrastructure/Schema.hpp"

namespace lms {

namespace {
[[noreturn]] void fail(sqlite3* db, const std::string& what) {
    throw DatabaseError(what + ": " + (db ? sqlite3_errmsg(db) : "no connection"));
}
}  // namespace

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        sqlite3_close(db_);
        db_ = nullptr;
        throw DatabaseError("cannot open database '" + path + "': " + msg);
    }
    sqlite3_busy_timeout(db_, 5000);
    exec("PRAGMA foreign_keys = ON;");
    Statement ver(*this, "PRAGMA user_version;");
    ver.step();
    if (ver.getInt(0) == 0) {  // new file: create tables, constraints and triggers
        Transaction tx(*this);
        exec(schemaSql());
        exec("PRAGMA user_version = 1;");
        tx.commit();
    }
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw DatabaseError("SQL error: " + msg);
    }
}

std::int64_t Database::lastInsertRowId() const { return sqlite3_last_insert_rowid(db_); }

Statement::Statement(Database& db, const std::string& sql) : db_(db) {
    if (sqlite3_prepare_v2(db.handle(), sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK)
        fail(db.handle(), "cannot prepare statement");
}

Statement::~Statement() { sqlite3_finalize(stmt_); }

Statement& Statement::bind(int i, int v) {
    if (sqlite3_bind_int(stmt_, i, v) != SQLITE_OK) fail(db_.handle(), "bind failed");
    return *this;
}
Statement& Statement::bind(int i, std::int64_t v) {
    if (sqlite3_bind_int64(stmt_, i, v) != SQLITE_OK) fail(db_.handle(), "bind failed");
    return *this;
}
Statement& Statement::bind(int i, double v) {
    if (sqlite3_bind_double(stmt_, i, v) != SQLITE_OK) fail(db_.handle(), "bind failed");
    return *this;
}
Statement& Statement::bind(int i, const std::string& v) {
    if (sqlite3_bind_text(stmt_, i, v.c_str(), static_cast<int>(v.size()), SQLITE_TRANSIENT) != SQLITE_OK)
        fail(db_.handle(), "bind failed");
    return *this;
}
Statement& Statement::bind(int i, const std::vector<std::uint8_t>& v) {
    const void* p = v.empty() ? "" : static_cast<const void*>(v.data());
    if (sqlite3_bind_blob(stmt_, i, p, static_cast<int>(v.size()), SQLITE_TRANSIENT) != SQLITE_OK)
        fail(db_.handle(), "bind failed");
    return *this;
}
Statement& Statement::bindNull(int i) {
    if (sqlite3_bind_null(stmt_, i) != SQLITE_OK) fail(db_.handle(), "bind failed");
    return *this;
}

bool Statement::step() {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;
    fail(db_.handle(), "statement failed");
}

bool Statement::isNull(int c) const { return sqlite3_column_type(stmt_, c) == SQLITE_NULL; }
int Statement::getInt(int c) const { return sqlite3_column_int(stmt_, c); }
std::int64_t Statement::getInt64(int c) const { return sqlite3_column_int64(stmt_, c); }
double Statement::getDouble(int c) const { return sqlite3_column_double(stmt_, c); }
std::string Statement::getText(int c) const {
    const unsigned char* t = sqlite3_column_text(stmt_, c);
    return t ? std::string(reinterpret_cast<const char*>(t), static_cast<std::size_t>(sqlite3_column_bytes(stmt_, c)))
             : std::string();
}
std::vector<std::uint8_t> Statement::getBlob(int c) const {
    const auto* p = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt_, c));
    int n = sqlite3_column_bytes(stmt_, c);
    return p ? std::vector<std::uint8_t>(p, p + n) : std::vector<std::uint8_t>();
}

Transaction::Transaction(Database& db) : db_(db) { db_.exec("BEGIN IMMEDIATE;"); }

Transaction::~Transaction() {
    if (!done_) {
        try {
            db_.exec("ROLLBACK;");
        } catch (...) {
        }
    }
}

void Transaction::commit() {
    db_.exec("COMMIT;");
    done_ = true;
}

}  // namespace lms
