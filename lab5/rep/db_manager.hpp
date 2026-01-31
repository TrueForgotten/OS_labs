#ifndef DB_MANAGER_HPP
#define DB_MANAGER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <mutex> // Работает в w64devkit
#include "sqlite/sqlite3.h"

struct Measurement {
    long long timestamp;
    double value;
};

class DBManager {
private:
    sqlite3* db;
    std::mutex dbMutex; // Стандартный мьютекс C++

public:
    DBManager(const std::string& dbPath) {
        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(db)));
        }
        createTable();
    }

    ~DBManager() {
        sqlite3_close(db);
    }

    void createTable() {
        const char* sql = 
            "CREATE TABLE IF NOT EXISTS measurements ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "timestamp INTEGER NOT NULL, "
            "value REAL NOT NULL);";
        
        char* errMsg = 0;
        if (sqlite3_exec(db, sql, 0, 0, &errMsg) != SQLITE_OK) {
            std::string err = errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error("SQL error: " + err);
        }
    }

    void insertMeasurement(long long timestamp, double value) {
        std::lock_guard<std::mutex> lock(dbMutex); // Авто-блокировка
        
        std::string sql = "INSERT INTO measurements (timestamp, value) VALUES (" + 
                          std::to_string(timestamp) + ", " + std::to_string(value) + ");";
        
        char* errMsg = 0;
        if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
            std::cerr << "Insert error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }

    Measurement getLastMeasurement() {
        std::lock_guard<std::mutex> lock(dbMutex);
        sqlite3_stmt* stmt;
        const char* sql = "SELECT timestamp, value FROM measurements ORDER BY timestamp DESC LIMIT 1;";
        
        Measurement m = {0, 0.0};
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                m.timestamp = sqlite3_column_int64(stmt, 0);
                m.value = sqlite3_column_double(stmt, 1);
            }
        }
        sqlite3_finalize(stmt);
        return m;
    }

    std::vector<Measurement> getHistory(long long startTime) {
        std::lock_guard<std::mutex> lock(dbMutex);
        std::vector<Measurement> result;
        sqlite3_stmt* stmt;
        
        std::string sql = "SELECT timestamp, value FROM measurements WHERE timestamp >= " + 
                          std::to_string(startTime) + " ORDER BY timestamp ASC;";

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                Measurement m;
                m.timestamp = sqlite3_column_int64(stmt, 0);
                m.value = sqlite3_column_double(stmt, 1);
                result.push_back(m);
            }
        }
        sqlite3_finalize(stmt);
        return result;
    }
};

#endif