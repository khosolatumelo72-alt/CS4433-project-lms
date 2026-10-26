#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace lms {

/// Owns the SQLite connection to lms.db. Foreign keys are switched on for every connection.
class Database {
public:
    /// Opens (or creates) the file. Use ":memory:" for tests. Creates the schema on a new file.
    explicit Database(const std::string& path);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void exec(const std::string& sql);
    sqlite3* handle() const { return db_; }
    std::int64_t lastInsertRowId() const;

private:
    sqlite3* db_ = nullptr;
};

/// A prepared statement. All values are bound as parameters, never joined into the SQL text.
class Statement {
public:
    Statement(Database& db, const std::string& sql);
    ~Statement();
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement& bind(int index, int value);
    Statement& bind(int index, std::int64_t value);
    Statement& bind(int index, double value);
    Statement& bind(int index, const std::string& value);
    Statement& bind(int index, const std::vector<std::uint8_t>& value);
    Statement& bindNull(int index);

    /// True while a row is available, false when finished.
    bool step();

    bool isNull(int col) const;
    int getInt(int col) const;
    std::int64_t getInt64(int col) const;
    double getDouble(int col) const;
    std::string getText(int col) const;
    std::vector<std::uint8_t> getBlob(int col) const;

private:
    Database& db_;
    sqlite3_stmt* stmt_ = nullptr;
};

/// RAII transaction: rolls back in the destructor unless commit() was called (NFR6, NFR7).
class Transaction {
public:
    explicit Transaction(Database& db);
    ~Transaction();
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    void commit();

private:
    Database& db_;
    bool done_ = false;
};

}  // namespace lms
