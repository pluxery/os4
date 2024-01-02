#pragma once

#include <format>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include "help_utils.hpp"

#if !defined (WIN32)
#	include <unistd.h>
#	include <time.h>
#endif

#include "db/sqlite3.h"
#include "db/config.hpp"
#include <cstdlib>

class Repository {
public:

    Repository() {
        this->_db = Config::CreateTables();
    }

    void insert(const std::string &table, const std::string &temperature) {

        if (!HelpUtils::isNumber(temperature)) {
            std::cerr << "Error: temperature is not number: " << temperature << std::endl;
            return;
        }

        std::string datetime = HelpUtils::convertTimePointToString(std::chrono::system_clock::now());
        insertQuery(table, temperature, datetime);
    }


    int GetAvgTemperature(
            const std::string &table,
            const std::chrono::system_clock::time_point &start_date_time,
            const std::chrono::system_clock::time_point &end_date_time) {

        std::string start_t = HelpUtils::convertTimePointToString(start_date_time);
        std::string end_t = HelpUtils::convertTimePointToString(end_date_time);

        std::ostringstream query;
        sqlite3_stmt *stmt;
        int result;

        query << "SELECT AVG(temperature) AS avg_temperature FROM " << table
              << " WHERE created_at >= '" << start_t << "' AND created_at <= '" << end_t << "';";

        sqlite3_prepare_v2(_db, query.str().c_str(), -1, &stmt, nullptr);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            result = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        return result;

    }

private:
    sqlite3 *_db = nullptr;

    void insertQuery(const std::string &table, const std::string &temperature, const std::string &datetime) {
        char *err = nullptr;
        std::ostringstream query;
        query << "INSERT INTO " << table << "(temperature, created_at) VALUES" "(" << temperature << ", " << "'"
              << datetime << "'"
              << ");";

        if (sqlite3_exec(_db, query.str().c_str(), nullptr, nullptr, &err)) {
            fprintf(stderr, "Ошибка SQL: %sn", err);
            sqlite3_free(err);
        }
    }
};