#include "sqlite3.h"
#include <iostream>

class Config {
public:
    static sqlite3 *CreateTables() {
        sqlite3 *db = nullptr;
        char *err = nullptr;

        if (sqlite3_open("log_db.dblite", &db)) {
            fprintf(stderr, "Ошибка открытия/создания БД: %s\n", sqlite3_errmsg(db));
        } else if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS log(id INTEGER PRIMARY KEY, temperature INT, created_at DATETIME);"
                                    "CREATE TABLE IF NOT EXISTS log_h(id INTEGER PRIMARY KEY, temperature INT, created_at DATETIME);"
                                    "CREATE TABLE IF NOT EXISTS log_d(id INTEGER PRIMARY KEY, temperature INT, created_at DATETIME);"
                                    "CREATE TABLE IF NOT EXISTS log_m(id INTEGER PRIMARY KEY, temperature INT, created_at DATETIME);",
                                nullptr, nullptr, &err)) {
            fprintf(stderr, "Ошибка SQL21: %sn", err);
            sqlite3_free(err);
        }
        return db;
    }
};