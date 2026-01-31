#include <iostream>
#include <string>
#include <random>
#include <iomanip>
#include "my_serial.hpp"

// ПОДКЛЮЧЕНИЕ СИСТЕМНОЙ ФУНКЦИИ СНА
#ifdef _WIN32
    #include <windows.h> // Для Sleep()
#else
    #include <unistd.h>  // Для usleep()
#endif

// Кроссплатформенная функция сна (замена std::this_thread::sleep_for)
void sleep_ms(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds); // Windows: аргумент в миллисекундах
#else
    usleep(milliseconds * 1000); // Linux: аргумент в микросекундах
#endif
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port_name>" << std::endl;
        return 1;
    }

    std::string portName = argv[1];
    std::cout << "Starting Emulator on " << portName << "..." << std::endl;

    cplib::SerialPort serial(portName, cplib::SerialPort::BAUDRATE_9600);

    if (!serial.IsOpen()) {
        std::cerr << "Failed to open port! Check if port exists." << std::endl;
        return 1;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> d(22.0, 2.0); 

    while (true) {
        float temp = std::round(d(gen) * 10) / 10.0; 
        
        // Формируем сообщение
        std::string msg = std::to_string(temp);
        msg = msg.substr(0, msg.find(".") + 2) + "\n"; 

        std::cout << "Sending: " << msg; 
        
        if (serial.Write(msg) != cplib::SerialPort::RE_OK) {
            std::cerr << "Error writing to port!" << std::endl;
        }

        sleep_ms(1000); 
    }

    return 0;
}