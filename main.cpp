#include "my_serial.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>

#if !defined (WIN32)
#	include <unistd.h>          // pause()
#	include <time.h>            // nanosleep()
#endif

#include <cstdlib>

#define LOG_FILE "log.txt" //логируюм все данные теспературы и даты
#define LOG_FILE_H "log_h.txt"//логируюм раз в час среднюю данные теспературы и даты
#define LOG_FILE_D "log_d.txt"//логируюм раз в день среднюю данные теспературы и даты
#define LOG_FILE_M "log_m.txt"//логируюм раз в месяц среднюю данные теспературы и даты

int getTemperature() {
    srand(static_cast<unsigned int>(time(0)));
    int randomNumber = rand() % 21;

    return randomNumber;
}

bool isNumber(const std::string &str) {
    try {
        std::size_t pos = 0;
        std::stoi(str, &pos);
        return pos == str.length(); // Make sure the entire string was consumed
    } catch (const std::exception &) {
        return false; // Failed to convert to int or exception occurred
    }
}

void writeToLog(const std::string &logfilename, const std::string &message) {
    if (!isNumber(message)) {
        return;
    }
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // Преобразуем текущее время в строку
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime), "%Y-%m-%d %H:%M:%S");
    std::string dateTimeStr = ss.str();

    std::ofstream logfile;
    logfile.open(logfilename,
                 std::ios::app); // Открыть файл для записи, режим std::ios::app добавляет текст в конец файла

    if (logfile.is_open()) {
        logfile << message << "|" << dateTimeStr << std::endl; // Записать сообщение в файл
        logfile.close(); // Закрыть файл
    } else {
        std::cout << "Не удалось открыть лог-файл!" << std::endl;
    }
}

std::tm stringToTm(const std::string &dateTimeStr) {
    std::tm resultTimeInfo = {};

    std::istringstream ss(dateTimeStr);
    ss >> std::get_time(&resultTimeInfo, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        std::cerr << "Date parsing failed!" << std::endl;
    }
    std::put_time(&resultTimeInfo, "%Y-%m-%d %H:%M:%S");

    return resultTimeInfo;
}

int readDataFromLogFileAndComputeAvg(const std::string &logFilePath,
                                     const std::chrono::system_clock::time_point &startTime,
                                     const std::chrono::system_clock::time_point &endTime) {
    int sumTemperatures = 0;
    int cnt_finded_records = 1;
    std::ifstream logFile(logFilePath);
    if (!logFile) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        return -1;
    }

    std::string line;

    while (std::getline(logFile, line)) {
        // Пропускаем пустые строки
        if (line.empty()) {
            continue;
        }
        // Преобразуем строку с датой и временем в объект std::tm
        std::istringstream datetime_ss(line.substr(line.find('|') + 1));
        std::string datetime_str = datetime_ss.str();

        std::tm timeInfo = stringToTm(datetime_str);
        auto dataTimestamp = std::chrono::system_clock::from_time_t(std::mktime(&timeInfo));

        // Проверяем, находится ли время записи внутри заданного интервала
        if (dataTimestamp >= startTime && dataTimestamp <= endTime) {
            cnt_finded_records++;
            std::string temperature_str = line.substr(0, line.find('|'));
            sumTemperatures += std::stoi(temperature_str);
        }
    }
    std::cout << "AVG_TEMPERATURES:" << sumTemperatures / cnt_finded_records << std::endl;
    logFile.close();
    return sumTemperatures / cnt_finded_records;
}

// Сконвертировать любой базовый тип в строку
template<class T>
std::string to_string(const T &v) {
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

void csleep(double timeout) {
#if defined (WIN32)
    if (timeout <= 0.0)
        ::Sleep(INFINITE);
    else
        ::Sleep((DWORD) (timeout * 1e3));
#else
    if (timeout <= 0.0)
        pause();
    else {
        struct timespec t;
        t.tv_sec = (int)timeout;
        t.tv_nsec = (int)((timeout - t.tv_sec)*1e9);
        nanosleep(&t, NULL);
    }
#endif
}

int main(int argc, char **argv) {
    auto fixed_now_h = std::chrono::system_clock::now();
    auto fixed_now_d = std::chrono::system_clock::now();
    auto fixed_now_m = std::chrono::system_clock::now();

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
    std::string mystr;
    if (read) {
        smport.SetTimeout(1.0);
        for (;;) {
            auto cur_now_h = std::chrono::system_clock::now();
            auto cur_now_d = std::chrono::system_clock::now();
            auto cur_now_m = std::chrono::system_clock::now();
            smport >> mystr;
            std::cout << "Got: " << (mystr.empty() ? "nothing" : mystr) << std::endl;

            writeToLog(LOG_FILE, mystr.empty() ? "0" : mystr);

            if (cur_now_h - fixed_now_h >= std::chrono::seconds(5)) {
                int avg = readDataFromLogFileAndComputeAvg(LOG_FILE, fixed_now_h, cur_now_h);
                writeToLog(LOG_FILE_H, to_string(avg));
                fixed_now_h = std::chrono::system_clock::now();
            }

            if (cur_now_d - fixed_now_d >= std::chrono::seconds(15)) {
                int avg = readDataFromLogFileAndComputeAvg(LOG_FILE, fixed_now_d, cur_now_d);
                writeToLog(LOG_FILE_D, to_string(avg));
                fixed_now_d = std::chrono::system_clock::now();
            }

            if (cur_now_d - fixed_now_m >= std::chrono::seconds(15 * 3)) {
                int avg = readDataFromLogFileAndComputeAvg(LOG_FILE, fixed_now_m, cur_now_m);
                writeToLog(LOG_FILE_M, to_string(avg));
                fixed_now_m = std::chrono::system_clock::now();
            }
        }
    } else {
        for (;;) {
            mystr = to_string(getTemperature());
            smport << mystr;
            csleep(1.0);
        }
    }
    return 0;
}