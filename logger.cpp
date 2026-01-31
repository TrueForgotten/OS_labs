#define NOMINMAX
// ВАЖНО: Сначала подключаем winsock2 (через httplib или напрямую), 
// чтобы избежать конфликтов с windows.h в других файлах
#ifdef _WIN32
    #include <winsock2.h>
#endif

#include "httplib/httplib.h" // Сервер (включает winsock2)

#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>

// Библиотеки проекта (могут включать windows.h, поэтому идут ПОСЛЕ httplib)
#include "my_serial.hpp"
#include "db_manager.hpp"

// Глобальный указатель на БД
std::unique_ptr<DBManager> db;

// Превращаем данные в JSON
std::string measurementToJson(const Measurement& m) {
    std::stringstream ss;
    ss << "{\"timestamp\": " << m.timestamp << ", \"value\": " << m.value << "}";
    return ss.str();
}

std::string historyToJson(const std::vector<Measurement>& vec) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        ss << measurementToJson(vec[i]);
        if (i < vec.size() - 1) ss << ",";
    }
    ss << "]";
    return ss.str();
}

// Функция веб-сервера (работает в отдельном потоке)
void runHttpServer() {
    httplib::Server svr;

    // Раздаем папку web_client как статический сайт
    auto ret = svr.set_mount_point("/", "./web_client");
    if (!ret) {
        std::cout << "Note: web_client folder not found nearby (GUI might not load)." << std::endl;
    }

    // API: Текущее значение
    svr.Get("/api/current", [](const httplib::Request&, httplib::Response& res) {
        Measurement m = db->getLastMeasurement();
        res.set_content(measurementToJson(m), "application/json");
    });

    // API: История
    svr.Get("/api/history", [](const httplib::Request& req, httplib::Response& res) {
        long duration = 3600;
        if (req.has_param("duration")) {
            try {
                duration = std::stol(req.get_param_value("duration"));
            } catch(...) {}
        }
        
        long long now = std::time(nullptr);
        auto history = db->getHistory(now - duration);
        res.set_content(historyToJson(history), "application/json");
    });

    std::cout << "HTTP Server listening on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}

int main(int argc, char* argv[]) {
    // Настройка кодировки консоли для Windows
    #ifdef _WIN32
    SetConsoleOutputCP(1251);
    #endif

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port_name>" << std::endl;
        return 1;
    }

    try {
        // 1. Инициализируем БД
        db = std::make_unique<DBManager>("sensor_data.sqlite");
        
        // 2. Запускаем сервер в фоновом потоке
        std::thread serverThread(runHttpServer);
        serverThread.detach();

        // 3. Подключаемся к порту
        std::string portName = argv[1];
        std::cout << "Connecting to " << portName << "..." << std::endl;
        
        cplib::SerialPort serial(portName, cplib::SerialPort::BAUDRATE_9600);
        serial.SetTimeout(0.1);

        if (!serial.IsOpen()) {
            std::cerr << "Failed to open port!" << std::endl;
            return 1;
        }
        serial.Flush();

        char readBuf[1024];
        std::string lineBuffer;

        // Основной цикл чтения данных
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
                            std::cout << "RX: " << temp << std::endl;
                            
                            // Пишем в БД (потокобезопасно)
                            db->insertMeasurement(now, temp);
                        } catch (...) {}
                    }
                }
            } else {
                // Спим немного, чтобы не грузить процессор
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}