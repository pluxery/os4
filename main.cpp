#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib>

#if !defined (WIN32)
#	include <unistd.h>
#	include <time.h>
#endif
#include <locale>
#include "my_serial.hpp"
#include "repository.hpp"
#include "help_utils.hpp"

#define TABLE_SEC "log"
#define TABLE_H "log_h"
#define TABLE_D "log_d"
#define TABLE_M "log_m"

#define HOUR 1
#define HOURS_IN_DAY (HOUR * 24)
#define HOURS_IN_MONTH (HOURS_IN_DAY * 30)

int getTemperatureFromPort() {
    srand(static_cast<unsigned int>(time(nullptr)));
    int random_temperature = 2 + rand() % 21;

    return random_temperature;
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");
    if (argc < 3) {
        std::cout << "Usage: sertest [port] [regime], where [regime] can be 'read' or 'write'" << std::endl;
        return -1;
    }

    bool read = true;

    if (!strcmp(argv[2], "write"))
        read = false;

    cplib::SerialPort smport(std::string(argv[1]), cplib::SerialPort::BAUDRATE_115200);

    if (!smport.IsOpen()) {
        std::cout << "Failed to open port '" << argv[1] << "'! Terminating..." << std::endl;
        return -2;
    }

    Repository repository;

    auto last_updated_h = std::chrono::system_clock::now();
    auto last_updated_d = std::chrono::system_clock::now();
    auto last_updated_m = std::chrono::system_clock::now();

    std::string temperature_from_port;
    if (read) {
        smport.SetTimeout(1.0);
        for (;;) {
            auto now_h = std::chrono::system_clock::now();
            auto now_d = std::chrono::system_clock::now();
            auto now_m = std::chrono::system_clock::now();

            smport >> temperature_from_port;

            std::cout << "Got temperature: " << (temperature_from_port.empty() ? "nothing" : temperature_from_port)
                      << std::endl;

            if (temperature_from_port.empty()) {
                continue;
            }

            repository.insert(TABLE_SEC, temperature_from_port);

            /* если прошел час */
            if (now_h - last_updated_h >= std::chrono::seconds (HOUR)) {
                int average_temperature = repository.GetAvgTemperature(TABLE_SEC, last_updated_h, now_h);
                repository.insert(TABLE_H, HelpUtils::baseTypeToString(average_temperature));
                last_updated_h = std::chrono::system_clock::now();
            }
            /* если прошел день */
            if (now_d - last_updated_d >= std::chrono::hours (HOURS_IN_DAY)) {
                int average_temperature = repository.GetAvgTemperature(TABLE_H, last_updated_d, now_d);
                repository.insert(TABLE_D, HelpUtils::baseTypeToString(average_temperature));
                last_updated_d = std::chrono::system_clock::now();
            }
            /* если прошел месяц */
            if (now_d - last_updated_m >= std::chrono::hours(HOURS_IN_MONTH)) {
                int average_temperature = repository.GetAvgTemperature(TABLE_D, last_updated_m, now_m);
                repository.insert(TABLE_M, HelpUtils::baseTypeToString(average_temperature));
                last_updated_m = std::chrono::system_clock::now();
            }
        }
    } else {
        for (;;) {
            temperature_from_port = HelpUtils::baseTypeToString(getTemperatureFromPort());
            smport << temperature_from_port;
            HelpUtils::sleep(1.0);
        }
    }
    return 0;
}