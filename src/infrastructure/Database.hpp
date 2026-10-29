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

/// One SQL command, prepared once and then filled in with values. Each ? in the SQL text is a
/// placeholder, and bind(1, ...) fills the first ?, bind(2, ...) the second, and so on. Values never
/// become part of the SQL text, so a user cannot inject SQL by typing it into a field.
/// Example:  Statement s(db, "SELECT name FROM course WHERE id = ?"); s.bind(1, 5); s.step();
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

/// A transaction groups several database changes so they are saved all together or not at all.
/// How to use it:
///     Transaction tx(db);   // starts the transaction
///     ...                   // make the changes
///     tx.commit();          // saves them
/// If an exception is thrown before commit() is reached, the destructor undoes ("rolls back") every
/// change. That is what keeps a crash from leaving half a submission or mark behind (NFR6, NFR7).
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
