#define NOMINMAX 
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "my_serial.hpp"

// ПОДКЛЮЧЕНИЕ СИСТЕМНОЙ ФУНКЦИИ СНА
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// Функция сна (замена std::this_thread)
void sleep_ms(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

// Константы времени в секундах (РЕАЛЬНЫЕ)
// const long SEC_IN_HOUR = 3600;          // 1 час
// const long SEC_IN_DAY = 86400;          // 24 часа
// const long SEC_IN_MONTH = SEC_IN_DAY * 30;   // 30 дней (условно месяц)
// const long SEC_IN_YEAR = SEC_IN_DAY * 365;   // 365 дней (год)

//Для показания работоспособности приложения уменьшим значения времени
const long SEC_IN_HOUR = 5;          
const long SEC_IN_DAY = 10;          
const long SEC_IN_MONTH = 20;   
const long SEC_IN_YEAR = 40; 

const std::string RAW_LOG = "temp_raw.log";
const std::string HOURLY_LOG = "temp_hourly.log";
const std::string DAILY_LOG = "temp_daily.log";

struct LogEntry {
    time_t timestamp;
    float value;
};

std::string timeToString(time_t t) {
    std::tm* tm_ptr = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void writeFormattedLine(std::ostream& out, time_t ts, float val) {
    out << ts << " " 
        << std::fixed << std::setprecision(2) << std::setw(6) << std::right << val 
        << "   # " << timeToString(ts) << "\n";
}

void clearAllLogs() {
    std::cout << "Clearing all log files..." << std::endl;
    std::ofstream(RAW_LOG, std::ios::trunc);
    std::ofstream(HOURLY_LOG, std::ios::trunc);
    std::ofstream(DAILY_LOG, std::ios::trunc);
}

void appendToLog(const std::string& filename, float value, time_t timestamp) {
    std::ofstream outfile;
    outfile.open(filename, std::ios_base::app);
    if (!outfile.is_open()) {
        std::cerr << "[Warning] Cannot write to " << filename << std::endl;
        return;
    }
    writeFormattedLine(outfile, timestamp, value);
}

std::vector<LogEntry> cleanAndReadLog(const std::string& filename, long maxAgeSeconds) {
    std::vector<LogEntry> entries;
    std::ifstream infile(filename);
    time_t now = std::time(nullptr);
    std::vector<LogEntry> keptEntries;

    if (infile.is_open()) {
        std::string line;
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            time_t ts;
            float val;
            if (ss >> ts >> val) {
                if (difftime(now, ts) <= maxAgeSeconds) {
                    keptEntries.push_back({ts, val});
                }
            }
        }
        infile.close();
    }
    
    std::ofstream outfile(filename, std::ios_base::trunc);
    if (!outfile.is_open()) return keptEntries;

    for (const auto& entry : keptEntries) {
        writeFormattedLine(outfile, entry.timestamp, entry.value);
    }
    return keptEntries; 
}

float calculateAverage(const std::vector<LogEntry>& entries, time_t startTime, time_t endTime) {
    double sum = 0;
    int count = 0;
    for (const auto& entry : entries) {
        if (entry.timestamp >= startTime && entry.timestamp < endTime) {
            sum += entry.value;
            count++;
        }
    }
    return (count > 0) ? (float)(sum / count) : -1000.0f;
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetConsoleOutputCP(1251); 
    #endif

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port_name>" << std::endl;
        return 1;
    }

    clearAllLogs(); 

    std::string portName = argv[1];
    std::cout << "Connecting to sensor on " << portName << "..." << std::endl;

    cplib::SerialPort serial(portName, cplib::SerialPort::BAUDRATE_9600);
    serial.SetTimeout(0.1); 

    if (!serial.IsOpen()) {
        std::cerr << "Failed to open port!" << std::endl;
        return 1;
    }

    serial.Flush(); 

    time_t lastHourlyCalc = std::time(nullptr);
    time_t lastDailyCalc = std::time(nullptr);
    
    std::string lineBuffer; 
    char readBuf[1024];

    while (true) {
        size_t bytesRead = 0;
        int res = serial.Read(readBuf, sizeof(readBuf), &bytesRead); 

        if (res == cplib::SerialPort::RE_OK && bytesRead > 0) {
            std::string chunk(readBuf, bytesRead);
            lineBuffer += chunk;

            size_t pos = 0;
            while ((pos = lineBuffer.find('\n')) != std::string::npos) {
                std::string line = lineBuffer.substr(0, pos);
                lineBuffer.erase(0, pos + 1);

                if (!line.empty() && line != "\r") {
                    try {
                        float temp = std::stof(line);
                        time_t now = std::time(nullptr);
                        
                        std::cout << "RX: " << std::fixed << std::setprecision(2) << std::setw(6) << temp 
                                  << " | " << timeToString(now) << std::endl;

                        appendToLog(RAW_LOG, temp, now);
                        cleanAndReadLog(RAW_LOG, SEC_IN_DAY);

                        if (difftime(now, lastHourlyCalc) >= SEC_IN_HOUR) {
                            std::cout << "--- Hourly Avg ---" << std::endl;
                            auto rawData = cleanAndReadLog(RAW_LOG, SEC_IN_DAY);
                            float avgHour = calculateAverage(rawData, now - SEC_IN_HOUR, now);
                            
                            if (avgHour > -900) {
                                appendToLog(HOURLY_LOG, avgHour, now);
                            }
                            cleanAndReadLog(HOURLY_LOG, SEC_IN_MONTH);
                            lastHourlyCalc = now;
                        }

                        if (difftime(now, lastDailyCalc) >= SEC_IN_DAY) {
                            std::cout << "--- Daily Avg ---" << std::endl;
                            auto hourlyData = cleanAndReadLog(HOURLY_LOG, SEC_IN_MONTH);
                            float avgDay = calculateAverage(hourlyData, now - SEC_IN_DAY, now);
                            
                            if (avgDay > -900) {
                                appendToLog(DAILY_LOG, avgDay, now);
                            }
                            cleanAndReadLog(DAILY_LOG, SEC_IN_YEAR);
                            lastDailyCalc = now;
                        }

                    } catch (...) {}
                }
            }
        } else {
            // Используем sleep_ms вместо std::this_thread
            sleep_ms(50);
        }
    }

    return 0;
}